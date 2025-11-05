#include "filemanagerui.hpp"
#include "fileprocessoradapter.hpp"

FileManagerUI::~FileManagerUI() {
  // FTXUI bug workaround: Terminal cleanup requires output to properly restore state
  // This ensures the terminal is left in a clean state even if the program
  // terminates unexpectedly (e.g., via exception or early exit)
  std::cout << "FileManager terminated. Final status: " << m_current_status 
            << std::endl;
}

void FileManagerUI::getMenuEntries() {
  m_menu_entries.clear();
  m_menu_entries.reserve(ActionMap.size()); // Pre-allocate
  for (const auto &pair : ActionMap) {
    m_menu_entries.push_back(pair.second.menu_text);
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
                     m_current_status =
                         "Menu action: " + ActionMap.at(action_id).menu_text +
                         " executed.";
                   }
                   return true;
                 }
                 return false;
               });
}

void FileManagerUI::setupFilePanels() {
  m_left_menu = Menu(&m_left_panel_files, &m_left_selected);
  m_right_menu = Menu(&m_right_panel_files, &m_right_selected);

  m_left_menu = m_left_menu | CatchEvent([this](Event event) {
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
  auto left_panel =
      FileManagerUI::createLeftPanel(); // createPanel(m_left_menu,
                                        // m_left_panel_path);
  auto right_panel =
      FileManagerUI::createRightPanel(); // createPanel(m_right_menu,
                                         // m_right_panel_path);

  m_file_split_view =
      ResizableSplitLeft(left_panel, right_panel, &m_left_panel_size);
}

void FileManagerUI::initialize() {
    FileProcessorAdapter fp(m_current_dir);
    m_left_file_infos = fp.scanDirectory(true);  // mit . und ..
    
    updateMenuStrings(m_left_file_infos, m_left_panel_files);
    
    setupTopMenu();
    setupFilePanels();
    setupMainLayout();
}
void FileManagerUI::setupMainLayout() {
  m_document =
      Container::Vertical({m_top_menu, Renderer([] { return separator(); }),
                           m_file_split_view | flex, Renderer([this] {
                             return text("STATUS: " + m_current_status) |
                                    color(Color::GrayLight) | hcenter;
                           })});
}

Component FileManagerUI::createLeftPanel() {
  return Renderer(m_left_menu, [this] {
    return vbox({text(m_left_panel_path) | bold | color(Color::Green),
                 separator(),
                 m_left_menu->Render() | vscroll_indicator | frame}) |
           border;
  });
}

Component FileManagerUI::createRightPanel() {
  return Renderer(m_right_menu, [this] {
    return vbox({text(m_right_panel_path) | bold | color(Color::Green),
                 separator(),
                 m_right_menu->Render() | vscroll_indicator | frame}) |
           border;
  });
}

ActionID FileManagerUI::getActionIdByIndex(int index) {
  if (index < 0 || index >= static_cast<int>(ActionMap.size())) {
    return ActionID::Quit; // Fallback in case of invalid index
  }

  auto it = ActionMap.begin();
  std::advance(it, index);
  return it->first;
}

void FileManagerUI::run() {
  auto global_handler = CatchEvent(m_document, [this](Event event) {
    if (event.is_character()) {
      return handleGlobalShortcut(event.character()[0]);
    }
    return false;
  });

  m_screen.Loop(global_handler);
}

void FileManagerUI::updateMenuStrings(const std::vector<FileInfo> &infos,
                                      std::vector<std::string> &target) {
  target.clear();
  target.reserve(infos.size());
  for (const auto &info : infos) {
    target.push_back(info.getDisplayName());
  }
}

// Helper methods
bool FileManagerUI::handleDirectoryChange(const FileInfo &selected_info) {
  m_left_panel_path = selected_info.getPath();

  FileProcessor fp(m_left_panel_path);
  m_left_file_infos = fp.scanDirectory();

  updateMenuStrings(m_left_file_infos, m_left_panel_files);

  // First, make sure it is within the valid range.
  if (m_left_file_infos.empty()) {
    m_left_selected = 0;
  } else {
    m_left_selected = std::min(m_left_selected,
                               static_cast<int>(m_left_file_infos.size()) - 1);
    m_left_selected = std::max(0, m_left_selected);
  }

  m_current_status = "Directory changed: " + m_left_panel_path;
  return true;
}

bool FileManagerUI::handleFileSelection(const FileInfo &selected_info) {
  if (selected_info.isDirectory()) {
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