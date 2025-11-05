#include "filemanagerui.hpp"

// UI
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

void FileManagerUI::setupLeftPanelEvents() {
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
}

void FileManagerUI::setupFilePanels() {
  // 1. Menüs erstellen
  m_left_menu = Menu(&m_left_panel_files, &m_left_selected);
  m_right_menu = Menu(&m_right_panel_files, &m_right_selected);

  // 2. Event-Logik direkt anhängen
  m_left_menu = m_left_menu | CatchEvent([&](Event event) {
                  // Beispiel: Enter-Taste
                  if (event == Event::Return && !m_left_file_infos.empty()) {
                    if (auto *selected_info =
                            safe_at(m_left_file_infos, m_left_selected)) {
                      // Debug-Ausgaben entfernen, wenn nicht mehr benötigt
                      return handleFileSelection(*selected_info);
                    }
                  }

                  // globale Events
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

Component FileManagerUI::createPanel(Component menu, const std::string &path) {
  return Renderer(menu, [&, path] {
    return vbox({text(path) | bold | color(Color::Green), separator(),
                 menu->Render() | vscroll_indicator | frame}) |
           border;
  });
}

ActionID FileManagerUI::getActionIdByIndex(int index) {
  auto it = ActionMap.begin();
  std::advance(it, index);
  std::cout << "ActionID index: " << index << "\n";
  return (it != ActionMap.end()) ? it->first : ActionID::Quit;
}

void FileManagerUI::run() {
  auto global_handler = CatchEvent(m_document, [&](Event event) {
    if (event.is_character()) {
      std::cout << "Event Char: " << event.character() << "\n";
      return handleGlobalShortcut(event.character()[0]);
    }
    return false;
  });

  m_screen.Loop(global_handler);
  std::cout << "Program ended. Last status: " << m_current_status << std::endl;
}

void FileManagerUI::getMenuEntries() {
  std::vector<std::string> entries;
  for (const auto &pair : ActionMap) {
    std::cout << "Debug Vectory pair.second.menu_text = "
              << pair.second.menu_text << "\n";
    entries.push_back(pair.second.menu_text);
  }
  m_menu_entries = entries;
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
bool FileManagerUI::handleDirectoryChange(const FileInfo& selected_info) {
  std::cout << "[DEBUG] Vor Verzeichniswechsel: selected=" << m_left_selected
            << ", old_size=" << m_left_panel_files.size() << "\n";

  m_left_panel_path = selected_info.path.string();
  FileProcessor fp(m_left_panel_path);
  m_left_file_infos = fp.scanDirectory();
  m_left_panel_files = fileInfoToMenuStrings(m_left_file_infos);

  // <<< Hier die entscheidende Korrektur: Selected-Index absichern!
  if (m_left_file_infos.empty()) {
      m_left_selected = 0; // Bleibt 0, aber zur Sicherheit
  } else {
      // Stellen Sie sicher, dass der Index nicht außerhalb des neuen Bereichs liegt.
      // Da wir in diesem Fall neu laden, setzen wir einfach auf 0.
      m_left_selected = 0;
  }

  std::cout << "[DEBUG] Nach Verzeichniswechsel: selected=" << m_left_selected
             << ", new_size=" << m_left_panel_files.size() << "\n";

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