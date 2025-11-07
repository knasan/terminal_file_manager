/**
 * @file filemanagerui.hpp
 * @brief Terminal-based file manager user interface using FTXUI
 *
 * This header defines the FileManagerUI class which provides a full-featured
 * terminal user interface for file management operations including directory
 * navigation, file filtering, duplicate detection, and deletion with safety checks.
 *
 * Key features:
 * - Asynchronous directory loading with progress indication
 * - Virtualized rendering for large directories (100 visible items at a time)
 * - Duplicate file detection and filtering
 * - Zero-byte file filtering
 * - File deletion with safety confirmations
 * - Full-screen terminal UI using FTXUI library
 *
 * @see FileInfo
 * @see FileProcessorAdapter
 */

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

/**
 * @class FileManagerUI
 * @brief Terminal-based file manager interface with async operations and filtering
 *
 * FileManagerUI provides a comprehensive terminal user interface for file management
 * operations. It uses the FTXUI library for rendering and supports asynchronous
 * directory loading, file filtering, duplicate detection, and safe file deletion.
 *
 * Architecture:
 * - Asynchronous loading: Directory scans run in background threads to keep UI responsive
 * - Virtualized rendering: Only renders visible portion of large file lists (100 items)
 * - Multi-threaded: Uses std::future for async operations and std::atomic for thread safety
 * - Filter states: None, DuplicatesOnly, ZeroBytesOnly (enum-based state machine)
 *
 * @see FileInfo
 * @see FileProcessorAdapter
 * @see UIControl
 */
class FileManagerUI {
private:
  // ===== UI State =====

  /** @brief Index of currently selected item in the file list */
  int m_selected = 0;

  /** @brief Index of selected item in the right panel (if applicable) */
  int m_right_selected = 0;

  /** @brief Display full file paths instead of just filenames */
  bool m_show_full_paths = false;

  /**
   * @enum FilterState
   * @brief File filtering mode for the UI
   *
   * Represents the current filter applied to the file list display:
   * - None: Show all files (no filtering)
   * - DuplicatesOnly: Show only files marked as duplicates
   * - ZeroBytesOnly: Show only zero-byte files
   */
  enum class FilterState { None, DuplicatesOnly, ZeroBytesOnly };

  /** @brief Current active filter state */
  FilterState m_current_filter_state = FilterState::None;

  // ===== Paths and Files =====

  /** @brief Current working directory path */
  std::string m_current_dir;

  /** @brief Path displayed in the file panel */
  std::string m_panel_path;

  /** @brief Currently displayed file information objects */
  std::vector<FileInfo> m_file_infos;

  /** @brief String representations of files for UI rendering */
  std::vector<std::string> m_panel_files;

  /** @brief Backup of all files before filtering is applied */
  std::vector<FileInfo> m_all_files;

  /** @brief Temporary storage for file information during operations */
  std::vector<FileInfo> m_store_files;

  // ===== UI Components =====

  /** @brief Top menu bar component */
  Component m_top_menu;

  /** @brief Main menu component */
  Component m_menu;

  /** @brief Primary view component */
  Component m_main_view;

  /** @brief Document/content area component */
  Component m_document;

  /**
   * @brief Creates a panel component with table layout for file display
   *
   * Constructs an FTXUI component that displays files in a tabular format
   * with columns for name, size, and other attributes.
   *
   * @return Component FTXUI component for the file panel
   */
  Component createPanelWithTable();

  /** @brief Current status message displayed in the UI */
  std::string m_current_status = "Ready.";

  /** @brief FTXUI fullscreen terminal screen instance */
  ScreenInteractive m_screen = ScreenInteractive::Fullscreen();

  /** @brief Menu entry labels for the UI menus */
  std::vector<std::string> m_menu_entries;

  // ===== Threading and Async Operations =====

  /** @brief Future for asynchronous directory loading operation */
  std::future<std::vector<FileInfo>> m_load_future;

  /** @brief Thread-safe flag indicating if a directory load is in progress */
  std::atomic<bool> m_loading{false};

  /** @brief Thread-safe counter for number of items loaded so far */
  std::atomic<int> m_loaded_count{0};

  /** @brief Loading status message displayed during async operations */
  std::string m_loading_message = "";

  // ===== Virtualization (Performance Optimization) =====

  /** @brief Maximum number of items to render at once (performance limit) */
  static constexpr int VISIBLE_ITEMS = 100;

  /** @brief Starting index for the currently visible window of items */
  int m_virtual_offset = 0;

  /** @brief Subset of files currently visible in the UI (virtualized view) */
  std::vector<std::string> m_visible_files;

  // ===== Asynchronous Operations =====

  /**
   * @brief Loads directory contents asynchronously in a background thread
   *
   * Initiates an async directory scan operation that runs in a separate thread
   * to avoid blocking the UI. Updates m_loaded_count atomically as items are
   * discovered for progress tracking.
   *
   * @param path The directory path to scan asynchronously
   *
   * @see updateUIAfterLoad()
   * @see m_load_future
   * @see m_loading
   */
  void loadDirectoryAsync(const std::filesystem::path &path);

  /**
   * @brief Updates UI components after async directory load completes
   *
   * Called when the async directory loading operation finishes. Retrieves
   * results from m_load_future, updates file lists, resets loading flags,
   * and refreshes the UI display.
   *
   * @see loadDirectoryAsync()
   */
  void updateUIAfterLoad();

  // ===== Virtualization =====

  /**
   * @brief Updates the virtualized view window for large file lists
   *
   * Recalculates which subset of files should be visible based on the
   * current selection and m_virtual_offset. Only renders VISIBLE_ITEMS
   * (100) files at a time for performance optimization.
   *
   * @see VISIBLE_ITEMS
   * @see m_virtual_offset
   * @see m_visible_files
   */
  void updateVirtualizedView();

  // ===== File and Directory Handling =====

  /**
   * @brief Handles file selection/activation events
   *
   * Processes user selection of a file entry. Behavior depends on file type
   * and current UI state.
   *
   * @param file The FileInfo object representing the selected file
   * @return true if the selection was handled successfully, false otherwise
   */
  bool handleFileSelection(const FileInfo &file);

  /**
   * @brief Handles directory navigation when user selects a directory
   *
   * Changes the current directory when a directory entry is selected.
   * Handles parent directory (..) navigation as well.
   *
   * @param dir The FileInfo object representing the selected directory
   * @return true if directory change succeeded, false otherwise
   */
  bool handleDirectoryChange(const FileInfo &dir);

  /**
   * @brief Maps menu index to action identifier
   *
   * Converts a numeric menu selection index to the corresponding ActionID
   * enum value for processing menu actions.
   *
   * @param index The menu item index
   * @return ActionID The action identifier corresponding to the menu item
   *
   * @see ActionID
   */
  ActionID getActionIdByIndex(int index);

  // ===== UI Setup and Management =====

  /**
   * @brief Initializes and configures the top menu bar component
   *
   * Sets up the top-level menu bar with action items and keyboard shortcuts.
   *
   * @see m_top_menu
   */
  void setupTopMenu();

  /**
   * @brief Initializes and configures the file panel components
   *
   * Creates and configures the file browsing panels with tables and lists.
   *
   * @see createPanelWithTable()
   */
  void setupFilePanels();

  /**
   * @brief Retrieves and populates menu entry strings
   *
   * Loads menu labels and action descriptions into m_menu_entries for
   * display in the UI menus.
   *
   * @see m_menu_entries
   */
  void getMenuEntries();

  /**
   * @brief Updates menu string representations from FileInfo objects
   *
   * Converts a vector of FileInfo objects to formatted strings suitable
   * for display in menus and lists.
   *
   * @param i Source vector of FileInfo objects
   * @param t Target vector to fill with formatted strings
   *
   * @see FileInfo
   */
  void updateMenuStrings(const std::vector<FileInfo> &i,
                         std::vector<std::string> &t);

  // ===== Filtering Functions =====

  /**
   * @brief Filters file list to show only duplicate files
   *
   * Updates the display to show only files that have been marked as
   * duplicates. Sets filter state to DuplicatesOnly.
   *
   * @see FilterState
   * @see FileInfo::isDuplicate()
   */
  void showDuplicates();

  /**
   * @brief Filters file list to show only zero-byte files
   *
   * Updates the display to show only files with zero size. Sets filter
   * state to ZeroBytesOnly.
   *
   * @see FilterState
   * @see FileInfo::zeroFiles()
   */
  void showZeroByteFiles();

  /**
   * @brief Clears all active filters and shows all files
   *
   * Resets the filter state to None and restores the full file list display.
   *
   * @see FilterState
   */
  void clearFilter();

  // ===== Deletion Functionality =====

  /**
   * @brief Displays confirmation dialog before file deletion
   *
   * Shows a confirmation dialog to the user before proceeding with file
   * deletion. May include safety warnings from FileSafety checks.
   *
   * @param file The FileInfo object for the file to be deleted
   * @return true if user confirmed deletion, false if cancelled
   *
   * @see FileSafety
   */
  bool showDeleteConfirmation(const FileInfo &file);

  /**
   * @brief Deletes a regular file from the filesystem
   *
   * Performs safety checks and deletes the specified file if allowed.
   *
   * @param file The FileInfo object representing the file to delete
   * @return true if deletion succeeded, false if it failed or was blocked
   *
   * @see FileSafety::checkDeletion()
   */
  bool deleteFile(const FileInfo &file);

  /**
   * @brief Deletes a directory from the filesystem
   *
   * Performs safety checks and deletes the specified directory. Can operate
   * recursively to delete non-empty directories.
   *
   * @param dir The FileInfo object representing the directory to delete
   * @param recursive If true, delete directory contents recursively
   * @return true if deletion succeeded, false if it failed or was blocked
   *
   * @see FileSafety::checkDeletion()
   */
  bool deleteDirectory(const FileInfo &dir, bool recursive);

  // ===== Dialog State =====

  /** @brief Flag indicating whether a modal dialog is currently active */
  bool m_dialog_active = false;

  // ===== Animation Thread =====

  /** @brief Background thread for UI animations (e.g., loading spinner) */
  std::thread m_animation_thread;

  /** @brief Thread-safe flag indicating if animation is currently running */
  std::atomic<bool> m_animating{false};

  /**
   * @brief Starts the animation thread for visual feedback
   *
   * Launches the background animation thread for displaying loading
   * indicators or progress animations.
   *
   * @see m_animation_thread
   * @see stopAnimation()
   */
  void startAnimation();

  /**
   * @brief Stops the animation thread
   *
   * Signals the animation thread to terminate and waits for it to finish.
   *
   * @see m_animation_thread
   * @see startAnimation()
   */
  void stopAnimation();

public:
  /** @brief Index of currently selected item in the top menu bar */
  int m_top_menu_selected = 0;

  /**
   * @brief Constructs a FileManagerUI instance
   *
   * Initializes the file manager UI with the current working directory as
   * the starting location. Sets up initial state for directory navigation.
   *
   * @note The FTXUI screen is initialized but not started until run() is called
   */
  FileManagerUI()
      : m_current_dir(std::filesystem::current_path()),
        m_panel_path(m_current_dir) {};

  /**
   * @brief Destructor for FileManagerUI
   *
   * Cleans up resources including stopping animation threads and releasing
   * FTXUI components. Ensures proper shutdown of async operations.
   */
  ~FileManagerUI();

  /**
   * @brief Initializes the file manager UI components
   *
   * Performs initial setup of all UI components, loads the starting directory,
   * and prepares the interface for display. Must be called before run().
   *
   * @see setupMainLayout()
   * @see setupTopMenu()
   * @see setupFilePanels()
   */
  void initialize();

  /**
   * @brief Sets up the main UI layout structure
   *
   * Configures the overall layout of the UI including menu bars, file panels,
   * status bar, and other visual components. Called during initialization.
   *
   * @see initialize()
   */
  void setupMainLayout();

  /**
   * @brief Handles global keyboard shortcuts
   *
   * Processes keyboard input for global shortcuts that work regardless of
   * which UI component has focus (e.g., Ctrl+C to quit, F keys for actions).
   *
   * @param key The character code of the pressed key
   * @return true if the shortcut was handled, false if not recognized
   */
  bool handleGlobalShortcut(char key);

  /**
   * @brief Starts the main UI event loop
   *
   * Begins the FTXUI event loop and displays the interface. This is a blocking
   * call that runs until the user quits the application.
   *
   * @see initialize()
   * @see m_screen
   */
  void run();
};