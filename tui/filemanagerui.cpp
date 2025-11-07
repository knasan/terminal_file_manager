/**
 * @file filemanagerui.cpp
 * @brief Implementation of the FileManagerUI class
 *
 * This file contains the complete implementation of the FileManagerUI class,
 * including UI rendering, event handling, file operations, filtering, and
 * asynchronous directory loading.
 *
 * Key implementation areas:
 * - Destructor and resource cleanup
 * - File filtering (duplicates, zero-byte files)
 * - Asynchronous directory loading with progress tracking
 * - Virtualized view management for large file lists
 * - UI setup and layout configuration
 * - File/directory navigation and selection
 * - Delete operations with safety checks
 * - Animation thread management
 *
 * @see FileManagerUI
 * @see filemanagerui.hpp
 */

#include "filemanagerui.hpp"
#include "duplicatefinder.hpp"
#include "fileprocessoradapter.hpp"
#include "filesafety.hpp"
#include "utils.hpp"

FileManagerUI::~FileManagerUI() {
  stopAnimation();

  // Wait for background task to complete
  if (m_load_future.valid()) {
    m_load_future.wait();
  }

  // FTXUI bug workaround: Terminal cleanup requires output to properly restore
  // state This ensures the terminal is left in a clean state even if the
  // program terminates unexpectedly (e.g., via exception or early exit)
  std::cout << "FileManager terminated. Final status: " << m_current_status
            << std::endl;
}

// ============================================================================
// FILTERING FUNCTIONS
// ============================================================================

/**
 * @brief Filters file list to show only duplicate files
 *
 * Implementation details:
 * 1. Toggle behavior: If duplicate filter is already active, clears it
 * 2. State preservation: Clears any other active filter first
 * 3. Backs up current file list to m_all_files
 * 4. Uses DuplicateFinder to identify duplicates
 * 5. Extracts only files marked as duplicates
 * 6. Updates UI to show full paths and resets selection
 * 7. Calculates and displays wasted space from duplicates
 *
 * If no duplicates are found, updates status message and returns without
 * applying filter.
 *
 * @see DuplicateFinder::findDuplicates()
 * @see DuplicateFinder::calculateWastedSpace()
 * @see clearFilter()
 */
void FileManagerUI::showDuplicates() {
  // 1. Switching logic: Deactivate if already active.
  if (m_current_filter_state == FilterState::DuplicatesOnly) {
    clearFilter();
    return;
  }

  // 2. State preservation: If ANOTHER filter is active, delete it first,
  //    so that m_file_infos is reset to its original state.
  if (m_current_filter_state != FilterState::None) {
    clearFilter();
  }

  // 3. Filter preparation (backup of the original state, since clearFilter()
  // deleted m_all_files)
  m_all_files = m_file_infos;
  auto groups = DuplicateFinder::findDuplicates(m_file_infos);

  if (groups.empty()) {
    m_current_status = "No duplicates found.";
    return;
  }

  m_store_files.clear();
  for (const auto &info : m_file_infos) {
    if (info.isDuplicate()) {
      m_store_files.push_back(info);
    }
  }

  m_file_infos = m_store_files;
  m_show_full_paths = true;
  m_current_filter_state = FilterState::DuplicatesOnly;

  updateMenuStrings(m_file_infos, m_panel_files);
  updateVirtualizedView();
  m_selected = 0;

  long long wasted = DuplicateFinder::calculateWastedSpace(groups);
  m_current_status = "Showing " + std::to_string(m_store_files.size()) +
                     " duplicates (" + formatBytes(wasted) +
                     " wasted). Press 'c' to clear filter.";
}

/**
 * @brief Filters file list to show only zero-byte files
 *
 * Implementation details:
 * 1. Toggle behavior: If zero-byte filter is already active, clears it
 * 2. State preservation: Clears any other active filter first
 * 3. Backs up current file list to m_all_files
 * 4. Filters for files with size == 0 (excludes directories and parent dir)
 * 5. Updates UI to show full paths and resets selection
 * 6. If no zero-byte files found, clears backup and returns
 *
 * @see clearFilter()
 */
void FileManagerUI::showZeroByteFiles() {
  // 1. Switching logic: Deactivate if already active.
  if (m_current_filter_state == FilterState::DuplicatesOnly) {
    clearFilter();
    return;
  }

  // 2. State preservation: If ANOTHER filter is active, delete it first,
  //    so that m_file_infos is reset to its original state.
  if (m_current_filter_state != FilterState::None) {
    clearFilter();
  }

  m_all_files = m_file_infos; // Saving the current file list (backup)

  for (const auto &info : m_file_infos) {
    if (info.getFileSize() == 0 && !info.isDirectory() && !info.isParentDir()) {
      m_store_files.push_back(info);
    }
  }

  if (m_store_files.empty()) {
    m_current_status = "No 0-byte files found.";
    m_all_files.clear(); // Delete the backup because no filter was applied.
    clearFilter();       // ensures that the status is consistent.
    return;
  }

  m_file_infos = m_store_files;
  m_show_full_paths = true;
  m_current_filter_state = FilterState::ZeroBytesOnly;

  updateMenuStrings(m_file_infos, m_panel_files);
  updateVirtualizedView();
  m_selected = 0;
  m_current_status =
      "Filter: " + std::to_string(m_file_infos.size()) + " Zero file(s) found.";
  m_screen.RequestAnimationFrame();
}

/**
 * @brief Clears active filter and restores original file list
 *
 * Restores m_file_infos from the backup stored in m_all_files, clears the
 * backup, resets filter state to None, disables full path display, updates
 * the UI strings and virtualized view, and resets selection to first item.
 *
 * If no backup exists (m_all_files is empty), only resets the filter state.
 *
 * @see showDuplicates()
 * @see showZeroByteFiles()
 */
void FileManagerUI::clearFilter() {
  if (m_all_files.empty()) {
    m_current_filter_state = FilterState::None;
    return;
  }

  // Restore original files
  m_file_infos = m_all_files;
  m_all_files.clear();

  // IMPORTANT: back
  m_current_filter_state = FilterState::None;
  m_show_full_paths = false;

  updateMenuStrings(m_file_infos, m_panel_files);
  updateVirtualizedView();
  m_selected = 0;
  m_current_status = "Filter cleared. Showing " +
                     std::to_string(m_file_infos.size()) + " entries.";
  m_screen.RequestAnimationFrame();
}

// ============================================================================
// ASYNC LOADING
// ============================================================================

/**
 * @brief Loads directory contents asynchronously in a background thread
 *
 * Implementation flow:
 * 1. Waits for any existing async operation to complete
 * 2. Sets loading flags and clears current file lists
 * 3. Starts animation thread for visual feedback
 * 4. Launches async task using std::async:
 *    - Creates FileProcessorAdapter for the path
 *    - Scans directory with progress callback (updates m_loaded_count)
 *    - Posts results back to UI thread via m_screen.Post()
 *    - Calls updateUIAfterLoad() and stops animation
 * 5. Handles exceptions by displaying error status
 *
 * The operation is fully asynchronous - the UI remains responsive during
 * directory scanning.
 *
 * @param path The directory path to scan asynchronously
 *
 * @see updateUIAfterLoad()
 * @see startAnimation()
 * @see stopAnimation()
 */
void FileManagerUI::loadDirectoryAsync(const std::filesystem::path &path) {
  if (m_load_future.valid()) {
    m_load_future.wait();
  }

  m_loading = true;
  m_loaded_count = 0;
  m_loading_message = "Scanning directory...";

  m_file_infos.clear();
  m_panel_files.clear();
  m_visible_files.clear();

  startAnimation();

  m_load_future = std::async(std::launch::async, [this, path]() {
    try {
      FileProcessorAdapter fp(path);

      auto progress_callback = [this](int count) { m_loaded_count = count; };

      // FIXIT include_parent_dir, recursive, ?
      auto files = fp.scanDirectory(true, false, progress_callback);

      m_screen.Post([this, files = std::move(files)]() mutable {
        m_file_infos = std::move(files);

        updateUIAfterLoad();
        m_loading = false;

        stopAnimation();
      });

      return files;

    } catch (const std::exception &e) {
      m_screen.Post([this]() {
        m_current_status = "Error loading directory";
        m_loading = false;
        stopAnimation();
      });

      return std::vector<FileInfo>();
    }
  });
}

/**
 * @brief Updates UI components after async directory load completes
 *
 * Called from the UI thread after the async directory scan finishes.
 * Updates menu strings from file info objects, refreshes the virtualized
 * view window, resets selection to first item, updates status message with
 * item count, and clears the loading message.
 *
 * @see loadDirectoryAsync()
 * @see updateMenuStrings()
 * @see updateVirtualizedView()
 */
void FileManagerUI::updateUIAfterLoad() {
  updateMenuStrings(m_file_infos, m_panel_files);
  updateVirtualizedView();
  m_selected = 0;
  m_current_status = "Loaded " + std::to_string(m_file_infos.size()) + " items";
  m_loading_message = "";
}

// ============================================================================
// VIRTUALIZATION
// ============================================================================

/**
 * @brief Updates the virtualized view window for large file lists
 *
 * Implements virtual scrolling to handle large file lists efficiently by only
 * rendering a subset of items (VISIBLE_ITEMS = 100) at a time.
 *
 * Algorithm:
 * 1. If panel is empty, clears visible files and resets offset
 * 2. Calculates window: centers selection with VISIBLE_ITEMS/2 buffer
 * 3. Adjusts for boundaries (start >= 0, end <= total_items)
 * 4. Handles end-of-list case by shifting window backwards
 * 5. Sets m_virtual_offset to track window position
 * 6. Copies visible window subset to m_visible_files for rendering
 *
 * This ensures constant-time rendering regardless of total file count.
 *
 * @see VISIBLE_ITEMS
 * @see m_virtual_offset
 * @see m_visible_files
 */
void FileManagerUI::updateVirtualizedView() {
  if (m_panel_files.empty()) {
    m_visible_files.clear();
    m_virtual_offset = 0;
    return;
  }

  // Calculate visible window
  int total_items = m_panel_files.size();

  // Center selection in visible window
  int start = std::max(0, m_selected - VISIBLE_ITEMS / 2);
  int end = std::min(start + VISIBLE_ITEMS, total_items);

  // Adjust if we're at the end
  if (end == total_items) {
    start = std::max(0, end - VISIBLE_ITEMS);
  }

  m_virtual_offset = start;

  // Copy visible items
  m_visible_files.clear();
  m_visible_files.reserve(end - start);
  for (int i = start; i < end; ++i) {
    m_visible_files.push_back(m_panel_files[i]);
  }
}

// ============================================================================
// UI SETUP
// ============================================================================

/**
 * @brief Initializes the file manager UI components
 *
 * Sets up the complete UI by calling setup functions in order:
 * 1. setupTopMenu() - Creates top menu bar with actions
 * 2. setupFilePanels() - Creates file browsing panel with table
 * 3. setupMainLayout() - Arranges all components in layout
 * 4. loadDirectoryAsync() - Starts initial directory scan
 *
 * Must be called before run().
 *
 * @see setupTopMenu()
 * @see setupFilePanels()
 * @see setupMainLayout()
 * @see loadDirectoryAsync()
 */
void FileManagerUI::initialize() {
  setupTopMenu();
  setupFilePanels();
  setupMainLayout();

  // Start async loading
  loadDirectoryAsync(m_current_dir);
}

/**
 * @brief Initializes and configures the file panel components
 *
 * Creates the main file browsing panel with custom rendering and event
 * handling:
 *
 * Rendering features:
 * - Displays file name and size in two columns
 * - Color-codes files by type (red=1, green=2, yellow=3, blue=4)
 * - Highlights selected item with inverted colors and bold
 * - Supports both filename and full path display modes
 * - Right-aligns file sizes with gray color
 *
 * Event handling:
 * - Enter key: Handles file selection (opens directories, selects files)
 * - Character keys: Passes to global shortcut handler
 * - Automatically updates virtualized view after navigation
 *
 * Uses m_visible_files (virtualized subset) for rendering efficiency.
 *
 * @see createPanelWithTable()
 * @see handleFileSelection()
 * @see updateVirtualizedView()
 */
void FileManagerUI::setupFilePanels() {
  auto menu_option = MenuOption::Vertical();
  menu_option.entries_option.transform = [this](EntryState state) {
    const FileInfo *info = nullptr;

    for (const auto &file_info : m_file_infos) {
      std::string label_to_compare =
          m_show_full_paths ? file_info.getPath() : file_info.getDisplayName();

      if (label_to_compare == state.label) {
        info = &file_info;
        break;
      }
    }

    auto name_element = text(state.label);
    Element size_element = text("");

    if (info) {
      switch (info->getColorCode()) {
      case 1:
        name_element = name_element | color(Color::Red);
        break;
      case 2:
        name_element = name_element | color(Color::Green);
        break;
      case 3:
        name_element = name_element | color(Color::Yellow);
        break;
      case 4:
        name_element = name_element | color(Color::Blue);
        break;
      default:
        break;
      }

      size_element =
          text(formatBytes(info->getFileSize())) | color(Color::GrayLight);
    }

    auto row = hbox({name_element | size(WIDTH, EQUAL, 60), filler(),
                     size_element | align_right});

    if (state.focused) {
      row = row | inverted | bold;
    }

    return row;
  };

  // Use m_visible_files!
  m_menu = Menu(&m_visible_files, &m_selected, menu_option);

  m_menu = m_menu | CatchEvent([this](Event event) {
             bool needs_update = false;

             if (event == Event::Return && !m_file_infos.empty()) {
               if (auto *selected_info = safe_at(m_file_infos, m_selected)) {
                 needs_update = handleFileSelection(*selected_info);
               }
             } else if (event.is_character()) {
               needs_update = (event.character()[0]);
             }

             if (needs_update) {
               updateVirtualizedView();
             }

             return false;
           });

  m_main_view = createPanelWithTable();
}

/**
 * @brief Creates a panel component with table layout for file display
 *
 * Constructs the main file panel renderer with two display modes:
 *
 * Loading State:
 * - Shows animated spinner (10 frames cycling)
 * - Displays loading message and progress count
 * - Updates automatically via animation thread
 *
 * Normal State:
 * - Shows path with pagination info (e.g., "[2/5]" for large directories)
 * - Renders table header with "Name" (or "Full Path") and "Size" columns
 * - Displays menu with vertical scroll indicator
 * - Adapts height based on terminal size (minimum 5 lines)
 * - Shows only VISIBLE_ITEMS at a time (virtualized)
 *
 * @return Component FTXUI component for the file panel with border
 *
 * @see setupFilePanels()
 * @see m_visible_files
 * @see VISIBLE_ITEMS
 */
Component FileManagerUI::createPanelWithTable() {
  return Renderer(m_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = std::max(5, terminal_height - 9);

    // LOADING STATE
    if (m_loading) {
      static const std::vector<std::string> spinner = {"⠋", "⠙", "⠹", "⠸", "⠼",
                                                       "⠴", "⠦", "⠧", "⠇", "⠏"};
      static size_t frame = 0;
      frame = (frame + 1) % spinner.size();

      return vbox({text(m_panel_path) | bold | color(Color::Green), separator(),
                   vbox({text("") | flex,
                         hbox({text(spinner[frame]) | color(Color::Cyan) |
                                   bold | size(WIDTH, EQUAL, 2),
                               text(m_loading_message) |
                                   color(Color::GrayLight)}) |
                             center,
                         text("") | size(HEIGHT, EQUAL, 1),
                         text("Items found: " +
                              std::to_string(m_loaded_count.load())) |
                             color(Color::Yellow) | center,
                         text("") | flex}) |
                       flex}) |
             border;
    }

    // NORMAL STATE
    std::string header_name = m_show_full_paths ? "Full Path" : "Name";

    // Show paging info if needed
    std::string path_display = m_panel_path;
    if (m_file_infos.size() > VISIBLE_ITEMS) {
      int current_page = m_selected / VISIBLE_ITEMS + 1;
      int total_pages =
          (m_file_infos.size() + VISIBLE_ITEMS - 1) / VISIBLE_ITEMS;
      path_display += " [" + std::to_string(current_page) + "/" +
                      std::to_string(total_pages) + "]";
    }

    auto header = hbox({text(header_name) | bold | size(WIDTH, EQUAL, 60),
                        filler(), text("Size") | bold | align_right}) |
                  color(Color::Cyan);

    return vbox({text(path_display) | bold | color(Color::Green), separator(),
                 header, separator(),
                 m_menu->Render() | vscroll_indicator | frame |
                     size(HEIGHT, EQUAL, available_height)}) |
           border;
  });
}

// ============================================================================
// NAVIGATION
// ============================================================================

/**
 * @brief Handles directory navigation when user selects a directory
 *
 * Updates the panel path to the selected directory and initiates an
 * asynchronous load of the new directory contents. The UI remains
 * responsive during the load operation.
 *
 * @param selected_file_info The FileInfo object for the selected directory
 * @return true Always returns true to indicate the navigation was initiated
 *
 * @see loadDirectoryAsync()
 * @see handleFileSelection()
 */
bool FileManagerUI::handleDirectoryChange(const FileInfo &selected_file_info) {
  m_panel_path = selected_file_info.getPath();

  // Start async loading (non-blocking!)
  loadDirectoryAsync(m_panel_path);

  return true;
}

/**
 * @brief Handles file selection/activation events
 *
 * Processes user selection of a file entry. If the entry is a directory,
 * delegates to handleDirectoryChange() to navigate into it. Otherwise,
 * updates the status bar to show the selected file path.
 *
 * @param selected_info The FileInfo object for the selected entry
 * @return true Always returns true to indicate the selection was handled
 *
 * @see handleDirectoryChange()
 */
bool FileManagerUI::handleFileSelection(const FileInfo &selected_info) {
  if (selected_info.isDirectory()) {
    return handleDirectoryChange(selected_info);
  } else {
    m_current_status = "File: " + selected_info.getPath();
    return true;
  }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

/**
 * @brief Starts the main UI event loop
 *
 * Wraps the main document component with a global event handler that
 * intercepts character key presses and delegates them to
 * handleGlobalShortcut(). This allows keyboard shortcuts (like 'q' for quit,
 * 'd' for duplicates) to work from anywhere in the UI.
 *
 * Starts the FTXUI screen loop - this is a blocking call that runs until
 * the user quits the application.
 *
 * @see handleGlobalShortcut()
 * @see m_screen
 * @see m_document
 */
void FileManagerUI::run() {
  auto global_handler = CatchEvent(m_document, [this](Event event) {
    if (event.is_character()) {
      return handleGlobalShortcut(event.character()[0]);
    }
    return false;
  });

  m_screen.Loop(global_handler);
}

// ============================================================================
// HELPER METHODS
// ============================================================================

/**
 * @brief Retrieves and populates menu entry strings from ActionMap
 *
 * Clears m_menu_entries and repopulates it with menu titles from the
 * global ActionMap. Pre-allocates memory using reserve() for efficiency.
 *
 * @see ActionMap
 * @see m_menu_entries
 */
void FileManagerUI::getMenuEntries() {
  m_menu_entries.clear();
  m_menu_entries.reserve(ActionMap.size()); // Pre-allocate
  for (const auto &pair : ActionMap) {
    m_menu_entries.push_back(pair.second.m_menu_title);
  }
}

/**
 * @brief Initializes and configures the top menu bar component
 *
 * Creates the horizontal top menu using menu entries from ActionMap,
 * and sets up event handling for menu item selection. When Enter is
 * pressed on a menu item:
 * - Quit action: Exits the application
 * - Other actions: Updates status bar with action name
 *
 * @see getMenuEntries()
 * @see m_top_menu
 * @see ActionMap
 */
void FileManagerUI::setupTopMenu() {
  m_menu_entries = ::getMenuEntries(); // from uicontrol.hpp
  m_top_menu =
      Menu(&m_menu_entries, &m_top_menu_selected, MenuOption::Horizontal());

  m_top_menu = m_top_menu | CatchEvent([this](Event event) {
                 if (event == Event::Return) {
                   ActionID action_id = getActionIdByIndex(m_top_menu_selected);
                   if (action_id == ActionID::Quit) {
                     m_screen.Exit();
                   } else {
                     m_current_status = "Menu action: " +
                                        ActionMap.at(action_id).m_menu_title +
                                        " executed.";
                   }
                   return true;
                 }
                 return false;
               });
}

/**
 * @brief Sets up the main UI layout structure
 *
 * Constructs the overall vertical layout by combining:
 * 1. Top menu bar (m_top_menu)
 * 2. Horizontal separator
 * 3. Main file panel view (m_main_view) with flex sizing
 * 4. Status bar at bottom showing m_current_status
 *
 * Components are arranged in a vertical container that fills the terminal.
 *
 * @see m_document
 * @see m_top_menu
 * @see m_main_view
 */
void FileManagerUI::setupMainLayout() {
  m_document =
      Container::Vertical({m_top_menu, Renderer([] { return separator(); }),
                           m_main_view | flex, Renderer([this] {
                             return text("STATUS: " + m_current_status) |
                                    color(Color::GrayLight) | hcenter;
                           })});
}

/**
 * @brief Updates menu string representations from FileInfo objects
 *
 * Converts a vector of FileInfo objects to formatted strings for display
 * in menus and lists. Clears the target vector and repopulates it with
 * either full paths or display names depending on m_show_full_paths flag.
 *
 * @param infos Source vector of FileInfo objects to convert
 * @param target Target vector to fill with formatted strings (cleared first)
 *
 * @see FileInfo::getPath()
 * @see FileInfo::getDisplayName()
 * @see m_show_full_paths
 */
void FileManagerUI::updateMenuStrings(const std::vector<FileInfo> &infos,
                                      std::vector<std::string> &target) {
  target.clear();
  target.reserve(infos.size());

  for (const auto &info : infos) {
    if (m_show_full_paths) {
      target.push_back(info.getPath()); // fullPath
    } else {
      target.push_back(info.getDisplayName()); // file name
    }
  }
}

/**
 * @brief Maps menu index to action identifier
 *
 * Converts a numeric menu selection index to the corresponding ActionID
 * enum value by iterating through the ActionMap. Returns ActionID::Quit
 * as a safe fallback if the index is out of bounds.
 *
 * @param index The menu item index to look up
 * @return ActionID The action identifier for the menu item, or Quit if invalid
 *
 * @see ActionMap
 * @see ActionID
 */
ActionID FileManagerUI::getActionIdByIndex(int index) {
  if (index < 0 || index >= static_cast<int>(ActionMap.size())) {
    return ActionID::Quit; // Fallback in case of invalid index
  }

  auto it = ActionMap.begin();
  std::advance(it, index);
  return it->first;
}

// ============================================================================
// KEYBOARD SHORTCUTS AND ACTIONS
// ============================================================================

/**
 * @brief Handles global keyboard shortcuts
 *
 * Processes keyboard input for global shortcuts by looking up the key in
 * ActionMap and executing the corresponding action:
 *
 * Supported shortcuts:
 * - 'q': Quit application
 * - 'd': Show/toggle duplicate files filter
 * - 'c': Clear active filter
 * - '0': Show/toggle zero-byte files filter
 * - 'D': Delete marked/selected file with safety checks
 *
 * For delete operations:
 * 1. Validates file selection
 * 2. Shows confirmation dialog with FileSafety checks
 * 3. Calls deleteFile() or deleteDirectory() based on type
 * 4. Refreshes directory asynchronously on success
 *
 * @param key_pressed The character key that was pressed
 * @return true if the shortcut was recognized and handled, false otherwise
 *
 * @see ActionMap
 * @see showDeleteConfirmation()
 * @see deleteFile()
 * @see deleteDirectory()
 */
bool FileManagerUI::handleGlobalShortcut(char key_pressed) {
  for (const auto &pair : ActionMap) {
    if (key_pressed == pair.second.m_shortcut) {
      switch (pair.first) {

      case ActionID::Quit:
        m_screen.Exit();
        return true;

      case ActionID::FindDuplicates:
        showDuplicates();
        return true;

      case ActionID::ClearFilter:
        clearFilter();
        return true;

      case ActionID::FindZeroBytes:
        showZeroByteFiles();
        return true;

      // ========================================
      // DELETE FUNCTION
      // ========================================
      case ActionID::DeleteMarkedFiles: { // ← This is where the case begins
        if (m_file_infos.empty() || m_selected < 0 ||
            m_selected >= static_cast<int>(m_file_infos.size())) {
          m_current_status = "No file selected.";
          return true;
        }

        const FileInfo &selected = m_file_infos[m_selected];

        if (showDeleteConfirmation(selected)) {
          bool success;

          if (selected.isDirectory()) {
            success = deleteDirectory(selected, true);
          } else {
            success = deleteFile(selected);
          }

          if (success) {
            // Async refresh (instead of blocking!)
            loadDirectoryAsync(m_panel_path);
          }

        } else {
          m_current_status = "Delete cancelled.";
        }
        return true;
      } // ← This is where the case ends.

      default:
        m_current_status = "Global shortcut: '" + std::string(1, key_pressed) +
                           "' -> " + pair.second.m_menu_title;
        return true;
      }
    }
  }
  return false;
}

// ============================================================================
// DELETE OPERATIONS
// ============================================================================

/**
 * @brief Deletes a regular file from the filesystem
 *
 * Attempts to delete the file at the path specified in the FileInfo object.
 * Updates the status bar with success or error message.
 *
 * @param file The FileInfo object representing the file to delete
 * @return true if deletion succeeded, false if an error occurred
 *
 * @note Does not perform safety checks - caller must call
 * showDeleteConfirmation() first
 * @see showDeleteConfirmation()
 * @see FileSafety
 */
bool FileManagerUI::deleteFile(const FileInfo &file) {
  try {
    std::filesystem::remove(file.getPath());
    m_current_status = "✓ Deleted: " + file.getPath();
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    m_current_status = "✗ Error deleting file: " + std::string(e.what());
    return false;
  }
}

/**
 * @brief Deletes a directory from the filesystem
 *
 * Deletes the directory specified in the FileInfo object. Can operate in
 * two modes:
 *
 * Recursive mode (recursive=true):
 * - Counts total items in directory tree
 * - Shows progress in status bar
 * - Uses std::filesystem::remove_all() to delete all contents
 * - Displays total deleted item count on success
 *
 * Non-recursive mode (recursive=false):
 * - Only deletes empty directories
 * - Uses std::filesystem::remove()
 * - Fails if directory is not empty
 *
 * @param dir The FileInfo object representing the directory to delete
 * @param recursive If true, recursively delete all contents; if false, only
 * delete if empty
 * @return true if deletion succeeded, false if an error occurred
 *
 * @note Does not perform safety checks - caller must call
 * showDeleteConfirmation() first
 * @see showDeleteConfirmation()
 * @see FileSafety
 */
bool FileManagerUI::deleteDirectory(const FileInfo &dir, bool recursive) {
  try {
    if (recursive) {
      // Count items
      uintmax_t total = 0;
      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(dir.getPath())) {
        total++;
      }

      // Update status during deletion (simplified)
      m_current_status = "Deleting " + std::to_string(total) + " items...";

      std::filesystem::remove_all(dir.getPath());
      m_current_status = "✓ Deleted directory (recursive, " +
                         std::to_string(total) + " items): " + dir.getPath();
    } else {
      std::filesystem::remove(dir.getPath());
      m_current_status = "✓ Deleted empty directory: " + dir.getPath();
    }
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    m_current_status = "✗ Error: " + std::string(e.what());
    return false;
  }
}

/**
 * @brief Displays confirmation dialog before file deletion
 *
 * Shows an interactive confirmation dialog with safety checks before allowing
 * file deletion. The dialog includes:
 *
 * Safety checks:
 * 1. Uses FileSafety::checkDeletion() to validate path
 * 2. Blocks deletion if status is not Allowed or WarningRemovableMedia
 * 3. Shows special warning for removable media
 *
 * Dialog contents:
 * - Warning text (red): "DELETE FILE?" or "DELETE DIRECTORY? (RECURSIVE)"
 * - File path (yellow)
 * - File size
 * - Removable media warning (magenta) if applicable
 * - Instructions: 'y' to confirm, 'n' or ESC to cancel
 *
 * User input handling:
 * - 'y'/'Y': Confirms deletion
 * - 'n'/'N'/ESC: Cancels deletion
 *
 * @param file The FileInfo object for the file to be deleted
 * @return true if user confirmed deletion and safety checks passed, false
 * otherwise
 *
 * @see FileSafety::checkDeletion()
 * @see FileSafety::getStatusMessage()
 * @see deleteFile()
 * @see deleteDirectory()
 */
bool FileManagerUI::showDeleteConfirmation(const FileInfo &file) {
  // Safety check via FileSafety class
  auto status = FileSafety::checkDeletion(file.getPath());

  // Block if not allowed
  if (status != FileSafety::DeletionStatus::Allowed &&
      status != FileSafety::DeletionStatus::WarningRemovableMedia) {
    m_current_status = FileSafety::getStatusMessage(status, file.getPath());
    return false;
  }

  // Show dialog
  m_dialog_active = true;
  bool confirmed = false;
  auto dialog_screen = ScreenInteractive::TerminalOutput();

  // Extra warning for removable media
  bool is_removable =
      (status == FileSafety::DeletionStatus::WarningRemovableMedia);

  auto dialog_renderer = Renderer([&] {
    std::string warning =
        file.isDirectory() ? "DELETE DIRECTORY? (RECURSIVE)" : "DELETE FILE?";

    std::vector<Element> content = {
        text(warning) | bold | color(Color::Red) | hcenter, separator(),
        text("Path: " + file.getPath()) | color(Color::Yellow),
        text("Size: " + formatBytes(file.getFileSize()))};

    if (is_removable) {
      content.push_back(separator());
      content.push_back(text("This is on REMOVABLE MEDIA") |
                        color(Color::Magenta) | bold);
    }

    content.push_back(separator());
    content.push_back(text("") | size(HEIGHT, EQUAL, 1));
    content.push_back(
        hbox({text("Press ") | color(Color::GrayLight),
              text("'y'") | bold | color(Color::Green),
              text(" to confirm, ") | color(Color::GrayLight),
              text("'n'") | bold | color(Color::Red),
              text(" or ") | color(Color::GrayLight), text("ESC") | bold,
              text(" to cancel") | color(Color::GrayLight)}) |
        hcenter);

    return vbox(content) | border | center;
  });

  auto dialog_handler = CatchEvent(dialog_renderer, [&](Event event) {
    if (event == Event::Character('y') || event == Event::Character('Y')) {
      confirmed = true;
      dialog_screen.Exit();
      return true;
    }
    if (event == Event::Character('n') || event == Event::Character('N') ||
        event == Event::Escape) {
      confirmed = false;
      dialog_screen.Exit();
      return true;
    }
    return false;
  });

  dialog_screen.Loop(dialog_handler);
  m_dialog_active = false;

  return confirmed;
}

// ============================================================================
// ANIMATION THREAD
// ============================================================================

/**
 * @brief Starts the animation thread for visual feedback
 *
 * Launches a background thread that continuously requests UI animation frames
 * to keep the display updated during long-running operations (like directory
 * loading with spinner animation).
 *
 * The thread runs a tight loop that:
 * 1. Sleeps for 10ms between frames (~100 FPS)
 * 2. Requests animation frame from FTXUI screen
 * 3. Continues until m_animating flag is set to false
 *
 * If animation is already running, this function returns immediately without
 * starting a new thread.
 *
 * @see stopAnimation()
 * @see m_animating
 * @see m_animation_thread
 * @see loadDirectoryAsync()
 */
void FileManagerUI::startAnimation() {
  if (m_animating)
    return;

  m_animating = true;
  m_animation_thread = std::thread([this]() {
    while (m_animating) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (m_animating) {
        m_screen.RequestAnimationFrame(); // Force UI refresh
      }
    }
  });
}

/**
 * @brief Stops the animation thread
 *
 * Signals the animation thread to terminate by setting m_animating to false,
 * then waits for the thread to finish using join(). Requests one final
 * animation frame to ensure the UI is updated after stopping.
 *
 * Safe to call even if animation is not running.
 *
 * @see startAnimation()
 * @see m_animating
 * @see m_animation_thread
 */
void FileManagerUI::stopAnimation() {
  m_animating = false;
  if (m_animation_thread.joinable()) {
    m_animation_thread.join();
  }

  // CRITICAL: Request final frame redraw to clear spinner
  // Without this, the last spinner frame stays visible on screen
  m_screen.RequestAnimationFrame();
}
