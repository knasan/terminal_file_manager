#include "fileprocessoradapter.hpp"
#include "uicontrol.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <map>

#include <atomic>
#include <chrono>
#include <future>

#include "utils.hpp"

using namespace ftxui;

class FileManagerUI {
private:
  // UI State
  int m_selected = 0;
  int m_right_selected = 0;

  bool m_show_full_paths = false;      // show file with full path
  bool m_show_duplicates_only = false; // show only duplicates

  // Paths and files
  std::string m_current_dir;
  std::string m_panel_path;
  std::vector<FileInfo> m_file_infos;
  std::vector<std::string> m_panel_files;
  std::vector<FileInfo> m_all_files;       // backup all files
  std::vector<FileInfo> m_duplicate_files; // Only duplicates

  // UI Components
  Component m_top_menu;
  Component m_menu;
  Component m_main_view;
  Component m_document;
  Component createPanel();
  Component createPanelWithTable();

  std::string m_current_status = "Ready.";
  ScreenInteractive m_screen = ScreenInteractive::Fullscreen();

  std::vector<std::string> m_menu_entries;

  // Threading
  std::future<std::vector<FileInfo>> m_load_future; // Worker-Thread
  std::atomic<bool> m_loading{false}; // Loading status (thread-safe)
  std::atomic<int> m_loaded_count{0}; // Progress
  std::string m_loading_message = "";

  // Virtualisierung
  static constexpr int VISIBLE_ITEMS = 1000; // Only 1000 items at a time
  int m_virtual_offset = 0;                  // Offset for virtualized list
  std::vector<std::string> m_visible_files;  // Only visible items

  // Background operations
  // Async loading
  void loadDirectoryAsync(const std::filesystem::path &path);

  void updateUIAfterLoad();

  // Virtualisierung
  void updateVirtualizedView();

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

  // Helper methods
  void showDuplicates();
  void clearFilter();

  // Delete functionality
  bool showDeleteConfirmation(const FileInfo &file);
  bool deleteFile(const FileInfo &file);
  bool deleteDirectory(const FileInfo &dir, bool recursive);

  // Dialog state
  bool m_dialog_active = false;

  // Animation thread
  std::thread m_animation_thread;
  std::atomic<bool> m_animating{false};

  void startAnimation();
  void stopAnimation();

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