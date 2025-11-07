#ifndef UI_CONTROL_HPP
#define UI_CONTROL_HPP

#include <map>
#include <string>
#include <vector>

struct ActionInfo {
  char m_shortcut;         // global shortcuts
  std::string m_menu_title; // title menü
};

enum class ActionID {
  // SelectDirectory,
  FindZeroBytes,
  FindDuplicates,    // 'd' - Show duplicates
  ClearFilter,       // 'c' - Clear filter
  DeleteMarkedFiles, // 'D' - Delete
  Quit               // 'q' - Quit
};

inline const std::map<ActionID, ActionInfo> ActionMap = {
    // {ActionID::SelectDirectory, {'p', "(p) Select Directory"}},
    {ActionID::FindZeroBytes, {'0', "(0) 0-Byte Files"}},
    {ActionID::FindDuplicates, {'d', "(d) Show Duplicates"}},
    {ActionID::ClearFilter, {'c', "(c) Clear Filter"}},
    {ActionID::DeleteMarkedFiles, {'D', "(D) Delete Marked"}},
    {ActionID::Quit, {'q', "(q) Quit"}}};

// Optional: helper function for menu entries
inline std::vector<std::string> getMenuEntries() {
  std::vector<std::string> entries;
  entries.reserve(ActionMap.size());
  for (const auto &[id, info] : ActionMap) {
    entries.push_back(info.m_menu_title);
  }
  return entries;
}

#endif // UI_CONTROL_HPP