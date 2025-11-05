#include "filemanagerui.hpp"
#include "fileprocessoradapter.hpp"

FileManagerUI::~FileManagerUI() {
  // FTXUI bug workaround: Terminal cleanup requires output to properly restore
  // state This ensures the terminal is left in a clean state even if the
  // program terminates unexpectedly (e.g., via exception or early exit)
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

// void FileManagerUI::setupFilePanels() {
//   m_left_menu = Menu(&m_left_panel_files, &m_left_selected);
//   m_right_menu = Menu(&m_right_panel_files, &m_right_selected);

//   m_left_menu = m_left_menu | CatchEvent([this](Event event) {
//                   if (event == Event::Return && !m_left_file_infos.empty()) {
//                     if (auto *selected_info =
//                             safe_at(m_left_file_infos, m_left_selected)) {
//                       return handleFileSelection(*selected_info);
//                     }
//                   }

//                   if (event.is_character()) {
//                     return handleGlobalShortcut(event.character()[0]);
//                   }

//                   return false;
//                 });

//   // 3. Panels rendern
//   auto left_panel =
//       FileManagerUI::createLeftPanel(); // createPanel(m_left_menu,
//                                         // m_left_panel_path);
//   auto right_panel =
//       FileManagerUI::createRightPanel(); // createPanel(m_right_menu,
//                                          // m_right_panel_path);

//   m_file_split_view =
//       ResizableSplitLeft(left_panel, right_panel, &m_left_panel_size);
// }

void FileManagerUI::setupFilePanels() {
  // Custom MenuOption mit Tabellen-Layout
  auto menu_option = MenuOption::Vertical();
  menu_option.entries_option.transform = [this](EntryState state) {
    // Finde das entsprechende FileInfo
    const FileInfo* info = nullptr;
    if (state.state >= 0 && state.state < static_cast<int>(m_left_file_infos.size())) {
      info = &m_left_file_infos[state.state];
    }
    
    // Tabellen-Layout: Name (links) | Größe (rechts)
    auto name_element = text(state.label);
    Element size_element = text("");
    
    if (info) {
      // Farbe für Name
      switch (info->getColorCode()) {
        case 1: name_element = name_element | color(Color::Red); break;
        case 2: name_element = name_element | color(Color::Green); break;
        case 3: name_element = name_element | color(Color::Yellow); break;
        case 4: name_element = name_element | color(Color::Blue); break;
        default: break;
      }
      
      // Größe
      size_element = text(info->getSizeFormatted()) | color(Color::GrayLight);
    }
    
    // Kombiniere Name und Größe
    auto row = hbox({
      name_element | size(WIDTH, EQUAL, 40),  // Name: 40 Zeichen breit
      filler(),                                 // Spacer
      size_element | align_right                // Größe: rechts-aligned
    });
    
    if (state.focused) {
      row = row | inverted | bold;
    }
    
    return row;
  };

  m_left_menu = Menu(&m_left_panel_files, &m_left_selected, menu_option);
  
  // Für right panel gleiche Option oder separate
  m_right_menu = Menu(&m_right_panel_files, &m_right_selected, menu_option);

  // Event handling wie gehabt
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

  // Panels mit Header
  auto left_panel = createLeftPanelWithTable();
  auto right_panel = createRightPanelWithTable();

  m_file_split_view =
      ResizableSplitLeft(left_panel, right_panel, &m_left_panel_size);
}

Component FileManagerUI::createLeftPanelWithTable() {
  return Renderer(m_left_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = terminal_height - 7;  // +1 für Header
    
    // Tabellen-Header
    auto header = hbox({
      text("Name") | bold | size(WIDTH, EQUAL, 40),
      filler(),
      text("Size") | bold | align_right
    }) | color(Color::Cyan);
    
    return vbox({
        text(m_left_panel_path) | bold | color(Color::Green),
        separator(),
        header,
        separator(),
        m_left_menu->Render() | 
            vscroll_indicator | 
            frame |
            size(HEIGHT, EQUAL, available_height)
    }) | border;
  });
}

Component FileManagerUI::createRightPanelWithTable() {
  return Renderer(m_right_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = terminal_height - 7;
    
    auto header = hbox({
      text("Name") | bold | size(WIDTH, EQUAL, 40),
      filler(),
      text("Size") | bold | align_right
    }) | color(Color::Cyan);
    
    return vbox({
        text(m_right_panel_path) | bold | color(Color::Green),
        separator(),
        header,
        separator(),
        m_right_menu->Render() | 
            vscroll_indicator | 
            frame |
            size(HEIGHT, EQUAL, available_height)
    }) | border;
  });
}

void FileManagerUI::initialize() {
  FileProcessorAdapter fp(m_current_dir);
  m_left_file_infos = fp.scanDirectory(true);
  updateMenuStrings(m_left_file_infos, m_left_panel_files);
  setupTopMenu();
  setupFilePanels();
  setupMainLayout();
}

void FileManagerUI::setupMainLayout() {
  m_document = Container::Vertical(
      {m_top_menu,                           // Fixed height (1 Zeile)
      //  Renderer([] { return separator(); }), // Fixed height (1 Zeile)
       m_file_split_view | flex,             // <-- NIMMT DEN REST!
       Renderer([this] {                     // Fixed height (1 Zeile)
         return text("STATUS: " + m_current_status) | color(Color::GrayLight) |
                hcenter;
       })});
}

Component FileManagerUI::createLeftPanel() {
  return Renderer(m_left_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = terminal_height - 6;
    
    // Custom Rendering mit Farben
    std::vector<Element> entries;
    for (size_t i = 0; i < m_left_file_infos.size(); ++i) {
        const auto& info = m_left_file_infos[i];
        bool selected = (i == static_cast<size_t>(m_left_selected));
        
        auto element = text(info.getDisplayName());
        
        // Farbe anwenden
        switch (info.getColorCode()) {
            case 1: element = element | color(Color::Red); break;       // 0-Byte
            case 2: element = element | color(Color::Green); break;     // Executable
            case 3: element = element | color(Color::Yellow); break;    // Duplicate
            case 4: element = element | color(Color::Blue); break;      // Directory
            default: element = element | color(Color::White); break;    // Normal
        }
        
        if (selected) {
            element = element | inverted | bold;
        }
        
        entries.push_back(element);
    }
    
    return vbox({
        text(m_left_panel_path) | bold | color(Color::Green),
        separator(),
        vbox(entries) | vscroll_indicator | frame | 
            size(HEIGHT, EQUAL, available_height)
    }) | border;
  });
}

Component FileManagerUI::createRightPanel() {
  return Renderer(m_right_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = terminal_height - 6;
    
    // Custom Rendering mit Farben
    std::vector<Element> entries;
    for (size_t i = 0; i < m_right_file_infos.size(); ++i) {
        const auto& info = m_right_file_infos[i];
        bool selected = (i == static_cast<size_t>(m_left_selected));
        
        auto element = text(info.getDisplayName());
        
        // Farbe anwenden
        switch (info.getColorCode()) {
            case 1: element = element | color(Color::Red); break;       // 0-Byte
            case 2: element = element | color(Color::Green); break;     // Executable
            case 3: element = element | color(Color::Yellow); break;    // Duplicate
            case 4: element = element | color(Color::Blue); break;      // Directory
            default: element = element | color(Color::White); break;    // Normal
        }
        
        if (selected) {
            element = element | inverted | bold;
        }
        
        entries.push_back(element);
    }
    
    return vbox({
        text(m_right_panel_path) | bold | color(Color::Green),
        separator(),
        vbox(entries) | vscroll_indicator | frame | 
            size(HEIGHT, EQUAL, available_height)
    }) | border;
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
  // std::cout << "[DEBUG] Changing to: " << selected_info.getPath() << "\n";

  m_left_panel_path = selected_info.getPath();

  FileProcessorAdapter fp(m_left_panel_path);
  m_left_file_infos = fp.scanDirectory(true);

  // std::cout << "[DEBUG] Scanned " << m_left_file_infos.size() << "
  // entries\n";

  updateMenuStrings(m_left_file_infos, m_left_panel_files);

  m_left_selected =
      m_left_file_infos.empty()
          ? 0
          : std::clamp(m_left_selected, 0,
                       static_cast<int>(m_left_file_infos.size()) - 1);

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