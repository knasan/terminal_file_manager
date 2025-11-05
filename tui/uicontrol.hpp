#ifndef UI_CONTROL_HPP
#define UI_CONTROL_HPP

#include <map>
#include <string>
#include <vector>

// 1. Eindeutige ID für jede mögliche Aktion
enum class ActionID {
    SelectDirectory,
    FindZeroBytes,
    FindDuplicates,
    DeleteMarkedFiles,
    Quit
};

// 2. Metadaten-Struktur für jede Aktion
struct ActionInfo {
    char shortcut;        // Das globale Tastenkürzel
    std::string menu_text; // Der Text für das Menü/die Statusleiste
};

// 3. Zentrale Tabelle, die ID mit Metadaten verknüpft
inline const std::map<ActionID, ActionInfo> ActionMap = {
    //^^^^ WICHTIG: inline keyword für C++17
    {ActionID::SelectDirectory,   {'p', "(p) Select Directory"}},
    {ActionID::FindZeroBytes,     {'0', "(0) 0-Byte-Dateien anzeigen"}},
    {ActionID::FindDuplicates,    {'d', "(d) Duplikate finden"}},
    {ActionID::DeleteMarkedFiles, {'D', "(D) Löschen markierter Dateien"}},
    {ActionID::Quit,              {'q', "(q) Beenden"}}
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