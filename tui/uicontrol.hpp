/**
 * @file uicontrol.hpp
 * @brief UI action definitions and keyboard shortcut mappings
 *
 * This header defines the action system for the file manager UI, including
 * action identifiers, keyboard shortcuts, menu titles, and helper functions
 * for menu generation.
 *
 * The action system provides a centralized mapping between:
 * - Action identifiers (ActionID enum)
 * - Keyboard shortcuts (single character keys)
 * - Menu display strings (with shortcut hints)
 *
 * @see ActionID
 * @see ActionInfo
 * @see ActionMap
 */

#ifndef UI_CONTROL_HPP
#define UI_CONTROL_HPP

#include <map>
#include <string>
#include <vector>

/**
 * @struct ActionInfo
 * @brief Information about a UI action including shortcut and menu title
 *
 * Encapsulates the metadata for a single UI action, providing both the
 * keyboard shortcut for quick access and the formatted menu title for
 * display in menus and help text.
 *
 * @see ActionID
 * @see ActionMap
 */
struct ActionInfo {
  /** @brief Single character keyboard shortcut for this action */
  char m_shortcut;

  /** @brief Formatted menu title string including shortcut hint (e.g., "(q) Quit") */
  std::string m_menu_title;
};

/**
 * @enum ActionID
 * @brief Enumeration of all available UI actions
 *
 * Defines the set of actions that can be performed in the file manager UI.
 * Each action has an associated keyboard shortcut and menu entry defined
 * in the ActionMap.
 *
 * Available Actions:
 * - FindZeroBytes: Filter to show only zero-byte files
 * - FindDuplicates: Filter to show only duplicate files
 * - ClearFilter: Remove active filters and show all files
 * - DeleteMarkedFiles: Delete selected/marked files with safety checks
 * - Quit: Exit the application
 *
 * @see ActionInfo
 * @see ActionMap
 */
enum class ActionID {
  // SelectDirectory,  // Reserved for future use

  /** @brief Filter view to show only zero-byte files (shortcut: '0') */
  FindZeroBytes,

  /** @brief Filter view to show only duplicate files (shortcut: 'd') */
  FindDuplicates,

  /** @brief Clear all active filters (shortcut: 'c') */
  ClearFilter,

  /** @brief Delete marked/selected files (shortcut: 'D') */
  DeleteMarkedFiles,

  /** @brief Quit the application (shortcut: 'q') */
  Quit
};

/**
 * @brief Global mapping of actions to their shortcuts and menu titles
 *
 * Provides a centralized lookup table that maps each ActionID to its
 * corresponding ActionInfo (shortcut key and menu title). This map is
 * used throughout the UI to:
 * - Handle keyboard input and map keys to actions
 * - Generate menu entries with consistent formatting
 * - Display help text with shortcut information
 *
 * Current Mappings:
 * - FindZeroBytes: '0' -> "(0) 0-Byte Files"
 * - FindDuplicates: 'd' -> "(d) Show Duplicates"
 * - ClearFilter: 'c' -> "(c) Clear Filter"
 * - DeleteMarkedFiles: 'D' -> "(D) Delete Marked"
 * - Quit: 'q' -> "(q) Quit"
 *
 * @see ActionID
 * @see ActionInfo
 * @see getMenuEntries()
 *
 * @note This is an inline constant map available at compile-time
 * @note Shortcuts are case-sensitive ('d' vs 'D' are different)
 */
inline const std::map<ActionID, ActionInfo> ActionMap = {
    // {ActionID::SelectDirectory, {'p', "(p) Select Directory"}},  // Reserved
    {ActionID::FindZeroBytes, {'0', "(0) 0-Byte Files"}},
    {ActionID::FindDuplicates, {'d', "(d) Show Duplicates"}},
    {ActionID::ClearFilter, {'c', "(c) Clear Filter"}},
    {ActionID::DeleteMarkedFiles, {'D', "(D) Delete Marked"}},
    {ActionID::Quit, {'q', "(q) Quit"}}};

/**
 * @brief Extracts menu entry strings from the ActionMap
 *
 * Iterates through the ActionMap and extracts all menu title strings,
 * returning them in a vector suitable for populating UI menus. The order
 * of entries matches the iteration order of the map (typically sorted by
 * ActionID enum value).
 *
 * Example output:
 * - "(0) 0-Byte Files"
 * - "(d) Show Duplicates"
 * - "(c) Clear Filter"
 * - "(D) Delete Marked"
 * - "(q) Quit"
 *
 * @return std::vector<std::string> Vector of formatted menu entry strings
 *
 * @see ActionMap
 * @see ActionInfo
 *
 * @note This is an inline function available at compile-time
 * @note Memory is pre-allocated using reserve() for efficiency
 */
inline std::vector<std::string> getMenuEntries() {
  std::vector<std::string> entries;
  entries.reserve(ActionMap.size());
  for (const auto &[id, info] : ActionMap) {
    entries.push_back(info.m_menu_title);
  }
  return entries;
}

#endif // UI_CONTROL_HPP