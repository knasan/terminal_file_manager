#include "filemanagerui.hpp"
#include "duplicatefinder.hpp"
#include "fileprocessoradapter.hpp"
#include "filesafety.hpp"

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

// Show duplicates
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
  m_current_status = "Filter: " + std::to_string(m_file_infos.size()) + " Zero file(s) found.";
  m_screen.RequestAnimationFrame();
}

// Reset filter
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

void FileManagerUI::initialize() {
  setupTopMenu();
  setupFilePanels();
  setupMainLayout();

  // Start async loading
  loadDirectoryAsync(m_current_dir);
}

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

      size_element = text(info->getSizeFormatted()) | color(Color::GrayLight);
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

bool FileManagerUI::handleDirectoryChange(const FileInfo &selected_file_info) {
  m_panel_path = selected_file_info.getPath();

  // Start async loading (non-blocking!)
  loadDirectoryAsync(m_panel_path);

  return true;
}

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
// REST
// ============================================================================

void FileManagerUI::getMenuEntries() {
  m_menu_entries.clear();
  m_menu_entries.reserve(ActionMap.size()); // Pre-allocate
  for (const auto &pair : ActionMap) {
    m_menu_entries.push_back(pair.second.m_menu_title);
  }
}

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

void FileManagerUI::setupMainLayout() {
  m_document =
      Container::Vertical({m_top_menu, Renderer([] { return separator(); }),
                           m_main_view | flex, Renderer([this] {
                             return text("STATUS: " + m_current_status) |
                                    color(Color::GrayLight) | hcenter;
                           })});
}

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

ActionID FileManagerUI::getActionIdByIndex(int index) {
  if (index < 0 || index >= static_cast<int>(ActionMap.size())) {
    return ActionID::Quit; // Fallback in case of invalid index
  }

  auto it = ActionMap.begin();
  std::advance(it, index);
  return it->first;
}

// Hashes, Duplicates, Filter, Delete - unchanged from before

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
        text("Size: " + file.getSizeFormatted())};

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

void FileManagerUI::stopAnimation() {
  m_animating = false;
  if (m_animation_thread.joinable()) {
    m_animation_thread.join();
  }
  m_screen.RequestAnimationFrame();
}
