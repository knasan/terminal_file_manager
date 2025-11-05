#include "filemanagerui.hpp"

/**
 * @brief Populate the UI menu entries from the action map.
 *
 * Extracts the menu text from each entry in ActionMap and replaces
 * the FileManagerUI::m_menu_entries vector with those strings.
 *
 * Details:
 * - Iterates over all key/value pairs in ActionMap and copies
 *   pair.second.menu_text into m_menu_entries.
 * - Existing contents of m_menu_entries are overwritten.
 * - The order of entries in m_menu_entries follows the iteration
 *   order of ActionMap (which may be unspecified depending on its type).
 *
 * Thread-safety:
 * - Not thread-safe. Callers must ensure exclusive access to ActionMap
 *   and m_menu_entries if concurrent access is possible.
 *
 * Complexity: O(N) time and O(N) additional space, where N is the number
 * of entries in ActionMap.
 *
 * @note Assumes each mapped value has a valid menu_text member.
 */
void FileManagerUI::getMenuEntries() {
  std::vector<std::string> entries;
  for (const auto &pair : ActionMap) {
    entries.push_back(pair.second.menu_text);
  }
  m_menu_entries = entries;
}

/**
 * @brief Configure and install the top (horizontal) menu for the file manager
 * UI.
 *
 * This function populates the internal menu entry list, constructs the
 * top-level Menu widget (using a horizontal layout), and attaches an event
 * handler that responds to activation (Return) events. The handler resolves the
 * currently selected menu entry to an ActionID via getActionIdByIndex and
 * dispatches the corresponding action:
 *
 * - If the resolved ActionID == ActionID::Quit, the application screen's Exit()
 *   method is invoked to terminate the UI.
 * - Otherwise, the current status text (m_current_status) is updated to reflect
 *   the executed menu action using the text from ActionMap.
 *
 * The attached handler returns true when it handles the Return event, allowing
 * the event to be consumed by the menu. Other events are not handled by this
 * lambda and will return false so normal event propagation can continue.
 *
 * Side effects / state modified:
 * - Initializes/assigns m_top_menu (first with a Menu object, then augmented
 *   with the event handler).
 * - Reads/writes m_menu_entries and m_top_menu_selected.
 * - May call m_screen.Exit() on Quit action.
 * - Updates m_current_status for non-quit actions.
 *
 * Notes:
 * - The event handler captures required context by reference ([&]) so member
 *   variables and helper functions (for example, getActionIdByIndex and
 *   ActionMap) are accessed from the enclosing object.
 * - The function does not return a value.
 */
void FileManagerUI::setupTopMenu() {
  getMenuEntries();
  m_top_menu =
      Menu(&m_menu_entries, &m_top_menu_selected, MenuOption::Horizontal());

  m_top_menu = m_top_menu | CatchEvent([&](Event event) {
                 if (event == Event::Return) {
                   ActionID action_id = getActionIdByIndex(m_top_menu_selected);
                   if (action_id == ActionID::Quit) {
                     m_screen.Exit();
                   } else {
                     m_current_status =
                         "Menu action: " + ActionMap.at(action_id).menu_text +
                         " executed.";
                   }
                   return true;
                 }
                 return false;
               });
}

/*void FileManagerUI::setupLeftPanelEvents() {
  m_left_menu = m_left_menu | CatchEvent([&](Event event) {
                  // Beispiel: Enter-Taste
                  if (event == Event::Return && !m_left_file_infos.empty()) {
                    if (auto *selected_info =
                            safe_at(m_left_file_infos, m_left_selected)) {
                      std::cout << "selected_info: " << selected_info
                                << std::endl;
                      std::cout << "[DEBUG] left_selected out-of-range: "
                                << m_left_selected
                                << ", vector size: " << m_left_file_infos.size()
                                << std::endl;
                      return handleFileSelection(*selected_info);
                    } else {
                      // Optional: debug output
                      std::cout << "[DEBUG] left_selected out-of-range: "
                                << m_left_selected
                                << ", vector size: " << m_left_file_infos.size()
                                << std::endl;
                    }
                  }

                  // andere Events global behandeln
                  if (event.is_character()) {
                    return handleGlobalShortcut(event.character()[0]);
                  }

                  return false;
                });
}*/

/**
 * @brief Sets up the file panels in the file manager UI.
 *
 * Initializes and configures two menu panels (left and right) for file
 * navigation. Each panel consists of:
 * - A menu component to display files
 * - Event handling for file selection (Enter key)
 * - Event handling for global shortcuts
 * - A panel layout with path display
 *
 * The panels are arranged in a resizable split view where the left panel's size
 * can be adjusted.
 */
void FileManagerUI::setupFilePanels() {
  m_left_menu = Menu(&m_left_panel_files, &m_left_selected);
  m_right_menu = Menu(&m_right_panel_files, &m_right_selected);

  m_left_menu = m_left_menu | CatchEvent([&](Event event) {
                  if (event == Event::Return && !m_left_file_infos.empty()) {
                    if (auto *selected_info =
                            safe_at(m_left_file_infos, m_left_selected)) {
                      return handleFileSelection(*selected_info);
                    }
                  }

                  if (event.is_character()) {
                    return handleGlobalShortcut(event.character()[0]);
                  }

                  return false;
                });

  // 3. Panels rendern
  auto left_panel = createPanel(m_left_menu, m_left_panel_path);
  auto right_panel = createPanel(m_right_menu, m_right_panel_path);

  m_file_split_view =
      ResizableSplitLeft(left_panel, right_panel, &m_left_panel_size);
}

/**
 * @brief Initialize the UI state and build the primary interface.
 *
 * Performs an initial scan of the current directory, prepares the left-panel
 * file model and its display strings, and constructs the top menu, file
 * panels and main layout for the file manager UI.
 *
 * @details
 * - Scans the directory referenced by the object's current-directory member
 *   and stores the resulting file information in the left-panel model.
 * - Converts the file information into display strings used by the left
 *   panel's menu/view.
 * - Calls internal setup routines to create the top menu, file panels and
 *   the main layout so the UI is ready to be presented.
 *
 * @pre m_current_dir must be set to a valid directory path before calling.
 * @post m_left_file_infos and m_left_panel_files are populated and the UI
 *       layout and menus are constructed.
 *
 * @note This operation performs synchronous file-system access and may block
 *       the calling thread for large directories. Call from the main/UI
 *       thread only if blocking is acceptable; otherwise run asynchronously.
 *
 * @thread-safety Not thread-safe — must be invoked from the UI/main thread.
 * @exceptions May propagate exceptions or errors from the underlying file
 *             scanning/processing code; callers should handle or guard
 *             against such errors as appropriate.
 */
void FileManagerUI::initialize() {
  FileProcessor fp(m_current_dir);
  m_left_file_infos = fp.scanDirectory();
  m_left_panel_files = fileInfoToMenuStrings(m_left_file_infos);

  setupTopMenu();
  setupFilePanels();
  setupMainLayout();
}

void FileManagerUI::setupMainLayout() {
  m_document =
      Container::Vertical({m_top_menu, Renderer([] { return separator(); }),
                           m_file_split_view | flex, Renderer([&] {
                             return text("STATUS: " + m_current_status) |
                                    color(Color::GrayLight) | hcenter;
                           })});
}

Component FileManagerUI::createPanel(Component menu,
                                     const std::string &path_ref) {
  return Renderer(
      menu, [menu, &path_ref] { // menu by value, path by ref zu member
        return vbox({text(path_ref) | bold | color(Color::Green), separator(),
                     menu->Render() | vscroll_indicator | frame}) |
               border;
      });
}

ActionID FileManagerUI::getActionIdByIndex(int index) {
  auto it = ActionMap.begin();
  std::advance(it, index);
  return (it != ActionMap.end()) ? it->first : ActionID::Quit;
}

/**
 * @brief Runs the main event loop of the file manager UI.
 *
 * Sets up a global event handler that processes character input events
 * through handleGlobalShortcut(). Starts the screen loop with the
 * configured event handler. When the loop exits, prints final status
 * message.
 *
 * @note A long cout string is required at the end for proper FXTUI operation
 */
void FileManagerUI::run() {
  auto global_handler = CatchEvent(m_document, [&](Event event) {
    if (event.is_character()) {
      return handleGlobalShortcut(event.character()[0]);
    }
    return false;
  });

  m_screen.Loop(global_handler);
  // WICHTIG: ohne einen langen cout string arbeitet fxtui nicht richitg.
  std::cout << "Program ended. Last status: " << m_current_status << std::endl;
}

std::vector<std::string>
FileManagerUI::fileInfoToMenuStrings(const std::vector<FileInfo> &infos) {
  std::vector<std::string> strings;
  for (const auto &info : infos) {
    strings.push_back(info.getDisplayName());
  }
  return strings;
}

// Helper methods
bool FileManagerUI::handleDirectoryChange(const FileInfo &selected_info) {
  m_left_panel_path = selected_info.path.string();
  FileProcessor fp(m_left_panel_path);
  m_left_file_infos = fp.scanDirectory();
  m_left_panel_files = fileInfoToMenuStrings(m_left_file_infos);

  if (m_left_file_infos.empty()) {
    m_left_selected = 0; // Bleibt 0, aber zur Sicherheit
  } else {
    m_left_selected = 0;
  }

  m_current_status = "Directory changed: " + m_left_panel_path;
  return true;
}

bool FileManagerUI::handleFileSelection(const FileInfo &selected_info) {
  if (selected_info.is_directory) {
    return handleDirectoryChange(selected_info);
  } else {
    m_current_status =
        "File marked for processing: " + selected_info.getDisplayName();
    return true;
  }
}

bool FileManagerUI::handleGlobalShortcut(char key_pressed) {
  for (const auto &pair : ActionMap) {
    if (key_pressed == pair.second.shortcut) {
      if (pair.first == ActionID::Quit) {
        m_screen.Exit();
      } else {
        m_current_status = "Global shortcut: '" + std::string(1, key_pressed) +
                           "' -> " + pair.second.menu_text;
      }
      return true;
    }
  }
  return false;
}