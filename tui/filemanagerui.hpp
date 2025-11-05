#include "fileprocessoradapter.hpp"
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

  // Paths and files
  std::string m_current_dir;
  std::string m_panel_path;
  std::vector<FileInfo> m_file_infos;
  std::vector<std::string> m_panel_files;

  // UI Components
  Component m_top_menu;
  Component m_left_menu;
  Component m_main_view;
  Component m_document;
  Component createPanel();
  Component createPanelWithTable();

  std::string m_current_status = "Ready.";
  ScreenInteractive m_screen = ScreenInteractive::TerminalOutput();

  std::vector<std::string> m_menu_entries;

  // Helper methods
  bool handleFileSelection(const FileInfo &);
  bool handleDirectoryChange(const FileInfo &);
  ActionID getActionIdByIndex(int);

  // UI
  void setupTopMenu();
  void setupFilePanels();
  void getMenuEntries();

  void updateMenuStrings(const std::vector<FileInfo> &i,
                         std::vector<std::string> &t);

  std::vector<FileInfo> m_all_files;       // Backup aller Files
  std::vector<FileInfo> m_duplicate_files; // Nur Duplikate
  bool m_show_duplicates_only = false;     // Filter aktiv?

  // Helper methods
  void calculateHashes();
  void showDuplicates();
  void clearFilter();

public:
  int m_top_menu_selected = 0;
  
  FileManagerUI()
      : m_current_dir(std::filesystem::current_path()),
        m_panel_path(m_current_dir) {};
  ~FileManagerUI();

  void initialize();

  void setupMainLayout();
  bool handleGlobalShortcut(char);

  void run();
};