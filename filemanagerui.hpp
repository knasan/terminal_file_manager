#include "fileprocessor.hpp"
#include "uicontrol.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <map>

#include "utils.hpp"

using namespace ftxui;

class FileManagerUI {
private:
  // UI State
  int left_selected;
  int right_selected = 0;
  int top_menu_selected = 0;
  int left_panel_size = 100;

  // Paths and files
  std::string current_dir;
  std::string left_panel_path;
  std::string right_panel_path;
  std::vector<FileInfo> left_file_infos;
  std::vector<std::string> left_panel_files;
  std::vector<std::string> right_panel_files;

  // UI Components
  Component top_menu;
  Component left_menu;
  Component right_menu;
  Component file_split_view;
  Component document;

  std::string current_status = "Ready. Press Tab to switch panels.";
  ScreenInteractive screen = ScreenInteractive::TerminalOutput();

  // Helper methods
  std::vector<std::string>
  fileInfoToMenuStrings(const std::vector<FileInfo> &infos) {
    std::vector<std::string> strings;
    for (const auto &info : infos) {
      strings.push_back(info.getDisplayName());
    }
    return strings;
  }

  ActionID getActionIdByIndex(int index) {
    auto it = ActionMap.begin();
    std::advance(it, index);
    return (it != ActionMap.end()) ? it->first : ActionID::Quit;
  }

  void setupTopMenu() {
    std::vector<std::string> menu_entries = getMenuEntries();
    top_menu =
        Menu(&menu_entries, &top_menu_selected, MenuOption::Horizontal());

    top_menu = top_menu | CatchEvent([&](Event event) {
                 if (event == Event::Return) {
                   ActionID action_id = getActionIdByIndex(top_menu_selected);
                   if (action_id == ActionID::Quit) {
                     screen.Exit();
                   } else {
                     current_status =
                         "Menu action: " + ActionMap.at(action_id).menu_text +
                         " executed.";
                   }
                   return true;
                 }
                 return false;
               });
  }

  void setupFilePanels() {

    left_menu = Menu(&left_panel_files, &left_selected);
    right_menu = Menu(&right_panel_files, &right_selected);

    setupLeftPanelEvents();

    auto left_panel = createPanel(left_menu, left_panel_path);
    auto right_panel = createPanel(right_menu, right_panel_path);

    file_split_view =
        ResizableSplitLeft(left_panel, right_panel, &left_panel_size);
  }

  Component createPanel(Component menu, const std::string &path) {
    return Renderer(menu, [&, path] {
      return vbox({text(path) | bold | color(Color::Green), separator(),
                   menu->Render() | vscroll_indicator | frame}) |
             border;
    });
  }

  void setupLeftPanelEvents() {
    left_menu = left_menu | CatchEvent([&](Event event) {
                  if (event == Event::Return && !left_file_infos.empty()) {

                    if (auto ptr = safe_at(left_file_infos, left_selected)) {
                      handleFileSelection(*ptr);
                      return true;
                    }
                  }
                  return false;
                });
  }

  bool handleFileSelection(const FileInfo &selected_info) {
    if (selected_info.is_directory) {
      return handleDirectoryChange(selected_info);
    } else {
      current_status =
          "File marked for processing: " + selected_info.getDisplayName();
      return true;
    }
  }

  bool handleDirectoryChange(const FileInfo &selected_info) {
    left_panel_path = selected_info.path.string();
    FileProcessor new_fp(left_panel_path);
    left_file_infos = new_fp.scanDirectory();
    left_panel_files = fileInfoToMenuStrings(left_file_infos);
    left_selected = 0;
    current_status = "Directory changed: " + left_panel_path;
    return true;
  }

public:
  FileManagerUI()
      : current_dir(std::filesystem::current_path()),
        left_panel_path(current_dir), right_panel_path("right panel path") {
    initialize();
  }

  void initialize() {
    FileProcessor fp(current_dir);
    left_file_infos = fp.scanDirectory();
    left_panel_files = fileInfoToMenuStrings(left_file_infos);

    setupTopMenu();
    setupFilePanels();
    setupMainLayout();
  }

  void setupMainLayout() {
    document =
        Container::Vertical({top_menu, Renderer([] { return separator(); }),
                             file_split_view | flex, Renderer([&] {
                               return text("STATUS: " + current_status) |
                                      color(Color::GrayLight) | hcenter;
                             })});
  }

  void run() {
    auto global_handler = CatchEvent(document, [&](Event event) {
      if (event.is_character()) {
        return handleGlobalShortcut(event.character()[0]);
      }
      return false;
    });

    screen.Loop(global_handler);
    std::cout << "Program ended. Last status: " << current_status << std::endl;
  }

  bool handleGlobalShortcut(char key_pressed) {
    for (const auto &pair : ActionMap) {
      if (key_pressed == pair.second.shortcut) {
        if (pair.first == ActionID::Quit) {
          screen.Exit();
        } else {
          current_status = "Global shortcut: '" + std::string(1, key_pressed) +
                           "' -> " + pair.second.menu_text;
        }
        return true;
      }
    }
    return false;
  }
};
