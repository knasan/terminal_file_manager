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
  int m_left_selected = 0;
  int m_right_selected = 0;
  int m_left_panel_size = 100;

  // Paths and files
  std::string m_current_dir;
  std::string m_left_panel_path;
  std::string m_right_panel_path;
  std::vector<FileInfo> m_left_file_infos;
  std::vector<std::string> m_left_panel_files;
  std::vector<std::string> m_right_panel_files;

  // UI Components
  Component m_top_menu;
  Component m_left_menu;
  Component m_right_menu;
  Component m_file_split_view;
  Component m_document;

  std::string m_current_status = "Ready. Press Tab to switch panels.";
  ScreenInteractive m_screen = ScreenInteractive::TerminalOutput();

  std::vector<std::string> m_menu_entries;

  // Helper methods
  std::vector<std::string> fileInfoToMenuStrings(const std::vector<FileInfo> &);
  bool handleFileSelection(const FileInfo &);
  bool handleDirectoryChange(const FileInfo &);
  ActionID getActionIdByIndex(int);

  // UI
  void setupTopMenu();
  void setupFilePanels();
  void setupLeftPanelEvents();
  void initialize();

  // Component
  Component createPanel(Component, const std::string &);

public:
  int m_top_menu_selected = 0;
  FileManagerUI()
      : m_current_dir(std::filesystem::current_path()),
        m_left_panel_path(m_current_dir), m_right_panel_path("") {
    initialize();
  }
  
  void setupMainLayout();
  bool handleGlobalShortcut(char);

  void getMenuEntries();

  void run();
};