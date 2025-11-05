#ifndef UI_CONTROL_HPP
#define UI_CONTROL_HPP

#include <map>
#include <string>
#include <vector>

struct ActionInfo {
    char shortcut;        // Das globale Tastenkürzel
    std::string menu_text; // Der Text für das Menü/die Statusleiste
};

enum class ActionID {
    SelectDirectory,
    FindZeroBytes,
    FindDuplicates,       // 'd' - Show duplicates
    CalculateHashes,      // 'h' - Calculate hashes
    ClearFilter,          // 'c' - Clear filter
    DeleteMarkedFiles,    // 'D' - Delete
    Quit                  // 'q'
};

inline const std::map<ActionID, ActionInfo> ActionMap = {
    {ActionID::SelectDirectory,   {'p', "(p) Select Directory"}},
    {ActionID::FindZeroBytes,     {'0', "(0) 0-Byte Files"}},
    {ActionID::FindDuplicates,    {'d', "(d) Show Duplicates"}},
    {ActionID::CalculateHashes,   {'h', "(h) Calculate Hashes"}},
    {ActionID::ClearFilter,       {'c', "(c) Clear Filter"}},
    {ActionID::DeleteMarkedFiles, {'D', "(D) Delete Marked"}},
    {ActionID::Quit,              {'q', "(q) Quit"}}
};

// Optional: Helper-Funktion für Menu-Entries
inline std::vector<std::string> getMenuEntries() {
    std::vector<std::string> entries;
    entries.reserve(ActionMap.size());
    for (const auto& [id, info] : ActionMap) {
        entries.push_back(info.menu_text);
    }
    return entries;
}

#endif // UI_CONTROL_HPP