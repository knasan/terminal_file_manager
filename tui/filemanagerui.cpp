#include "filemanagerui.hpp"
#include "duplicatefinder.hpp"
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

// Hash calculation for the current directory
void FileManagerUI::calculateHashes() {
  FNV1A hasher;
  int count = 0;

  for (auto &info : m_file_infos) {
    if (!info.isDirectory() && info.getFileSize() > 0 &&
        info.getHash().empty()) {
      std::string hash = hasher.calculateHash(info.getPath());
      info.setHash(hash);
      count++;
    }
  }

  m_current_status = "Calculated " + std::to_string(count) + " hashes";
}

// Show duplicates
void FileManagerUI::showDuplicates() {
  // Backup aller Files
  m_all_files = m_file_infos;

  // Find and mark duplicates
  auto groups = DuplicateFinder::findDuplicates(m_file_infos);

  if (groups.empty()) {
    m_current_status =
        "No duplicates found. Press 'h' to calculate hashes first.";
    return;
  }

  // Only keep duplicates.
  m_duplicate_files.clear();
  for (const auto &info : m_file_infos) {
    if (info.isDuplicate()) {
      m_duplicate_files.push_back(info);
    }
  }

  m_file_infos = m_duplicate_files;
  updateMenuStrings(m_file_infos, m_panel_files);
  m_selected = 0;
  m_show_duplicates_only = true;

  // Status mit Wasted Space
  long long wasted = DuplicateFinder::calculateWastedSpace(groups);
  m_current_status = "Showing " + std::to_string(m_duplicate_files.size()) +
                     " duplicates (" + DuplicateFinder::formatBytes(wasted) +
                     " wasted). Press 'c' to clear filter.";
}

// Reset filter
void FileManagerUI::clearFilter() {
  if (!m_show_duplicates_only) {
    m_current_status = "No filter active.";
    return;
  }

  // Restore original files
  m_file_infos = m_all_files;
  updateMenuStrings(m_file_infos, m_panel_files);
  m_selected = 0;
  m_show_duplicates_only = false;

  m_current_status = "Filter cleared. Showing all files.";
}

void FileManagerUI::setupFilePanels() {
  auto menu_option = MenuOption::Vertical();
  menu_option.entries_option.transform = [this](EntryState state) {
    const FileInfo *info = nullptr;
    auto index = state.state;

    if (index >= 0 && index < static_cast<int>(m_file_infos.size())) {
      info = &m_file_infos[index];
    }

    auto name_element = text(state.label);
    Element size_element = text("");

    // Find FileInfo via Label
    for (const auto &file_info : m_file_infos) {
      if (file_info.getDisplayName() == state.label) {
        info = &file_info;
        break;
      }
    }

    if (info) {
      // Apply color
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

      // Size
      size_element = text(info->getSizeFormatted()) | color(Color::GrayLight);
    }

    // Table row
    auto row = hbox({name_element | size(WIDTH, EQUAL, 40), filler(),
                     size_element | align_right});

    if (state.focused) {
      row = row | inverted | bold;
    }

    return row;
  };

  m_menu = Menu(&m_panel_files, &m_selected, menu_option);

  // Event handling as usual
  m_menu = m_menu | CatchEvent([this](Event event) {
                  if (event == Event::Return && !m_file_infos.empty()) {
                    if (auto *selected_info =
                            safe_at(m_file_infos, m_selected)) {
                      return handleFileSelection(*selected_info);
                    }
                  }

                  if (event.is_character()) {
                    return handleGlobalShortcut(event.character()[0]);
                  }

                  return false;
                });

  m_main_view = createPanelWithTable();
}

Component FileManagerUI::createPanelWithTable() {
  return Renderer(m_menu, [this] {
    int terminal_height = Terminal::Size().dimy;

    // Layout: Top(1) + Sep(1) + Status(1) = 3
    // Panel:  Border(2) + Path(1) + Sep(1) + Header(1) + Sep(1) = 6
    // Total: 9
    int available_height = std::max(5, terminal_height - 9);  // Min 5 Zeilen

    // Table header
    auto header = hbox({text("Name") | bold | size(WIDTH, EQUAL, 40), filler(),
                        text("Size") | bold | align_right}) |
                  color(Color::Cyan);

    return vbox({text(m_panel_path) | bold | color(Color::Green),
                 separator(), header, separator(),
                 m_menu->Render() | vscroll_indicator | frame |
                     size(HEIGHT, EQUAL, available_height)}) |
           border;
  });
}

void FileManagerUI::initialize() {  
  FileProcessorAdapter fp(m_current_dir);
  m_file_infos = fp.scanDirectory(true);
  updateMenuStrings(m_file_infos, m_panel_files);
  
  setupTopMenu();
  setupFilePanels();
  setupMainLayout();
}

void FileManagerUI::setupMainLayout() {
  m_document = Container::Vertical(
      {m_top_menu,
       Renderer([] { return separator(); }),
       m_main_view | flex, Renderer([this] {
         return text("STATUS: " + m_current_status) | color(Color::GrayLight) |
                hcenter;
       })});
}

Component FileManagerUI::createPanel() {
  return Renderer(m_menu, [this] {
    int terminal_height = Terminal::Size().dimy;
    int available_height = terminal_height - 6;

    // Custom Rendering with colors
    std::vector<Element> entries;
    for (size_t i = 0; i < m_file_infos.size(); ++i) {
      const auto &info = m_file_infos[i];
      bool selected = (i == static_cast<size_t>(m_selected));

      auto element = text(info.getDisplayName());

      // use color
      switch (info.getColorCode()) {
      case 1:
        element = element | color(Color::Red);
        break; // 0-Byte
      case 2:
        element = element | color(Color::Green);
        break; // Executable
      case 3:
        element = element | color(Color::Yellow);
        break; // Duplicate
      case 4:
        element = element | color(Color::Blue);
        break; // Directory
      default:
        element = element | color(Color::White);
        break; // Normal
      }

      if (selected) {
        element = element | inverted | bold;
      }

      entries.push_back(element);
    }

    return vbox({text(m_panel_path) | bold | color(Color::Green),
                 //separator(),
                 vbox(entries) | vscroll_indicator | frame |
                     size(HEIGHT, EQUAL, available_height)}) |
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
  m_panel_path = selected_info.getPath();

  FileProcessorAdapter fp(m_panel_path);
  m_file_infos = fp.scanDirectory(true);

  updateMenuStrings(m_file_infos, m_panel_files);

  m_selected =
      m_file_infos.empty()
          ? 0
          : std::clamp(m_selected, 0,
                       static_cast<int>(m_file_infos.size()) - 1);

  m_current_status = "Directory changed: " + m_panel_path;
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
      switch (pair.first) {
      case ActionID::Quit:
        m_screen.Exit();
        return true;

      case ActionID::CalculateHashes:
        calculateHashes();
        return true;

      case ActionID::FindDuplicates:
        showDuplicates();
        return true;

      case ActionID::ClearFilter:
        clearFilter();
        return true;

      default:
        m_current_status = "Global shortcut: '" + std::string(1, key_pressed) +
                           "' -> " + pair.second.menu_text;
        return true;
      }
    }
  }
  return false;
}
