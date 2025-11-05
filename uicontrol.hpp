#ifndef FILE_PROCESSOR_HPP
#define FILE_PROCESSOR_HPP

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
    char shortcut; // Das globale Tastenkürzel
    std::string menu_text; // Der Text für das Menü/die Statusleiste
};

// 3. Zentrale Tabelle, die ID mit Metadaten verknüpft
const std::map<ActionID, ActionInfo> ActionMap = {
    {ActionID::SelectDirectory,   {'p', "(p) Select Directory"}},
    {ActionID::FindZeroBytes,     {'0', "(0) 0-Byte-Dateien anzeigen"}},
    {ActionID::FindDuplicates,    {'d', "(d) Duplikate finden"}},
    {ActionID::DeleteMarkedFiles, {'D', "(D) Löschen markierter Dateien"}},
    {ActionID::Quit,              {'q', "(q) Beenden"}}
};

#endif // FILE_PROCESSOR_HPP
