/*
    FileProcessor ist Ihre Logikklasse (Controller/Service).
    Sie definiert, was mit diesen Dateien getan werden kann (Scannen, Löschen, Hashen).
*/
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <vector> // Sicherstellen, dass vector dabei ist

#include "fileprocessor.hpp" // Enthält FileProcessor Klasse
#include "fileinfo.hpp"      // Enthält FileInfo Struktur

namespace fs = std::filesystem;

// ----------------------------------------------------------------------
// Implementierung der scanDirectory Methode
// ----------------------------------------------------------------------

std::vector<FileInfo> FileProcessor::scanDirectory() {
    std::vector<FileInfo> found_files;

    // Das aktuelle Verzeichnis des FileProcessor-Objekts
    const fs::path& current_path = m_path; 

    // 1. Eintrag für das Elternverzeichnis hinzufügen ("..")
    if (current_path.has_parent_path()) {
        FileInfo parent_dir;
        parent_dir.path = current_path.parent_path();
        parent_dir.is_directory = true;
        parent_dir.size = 0; // Größe für Verzeichnisse irrelevant
        
        // Verwenden Sie den Dateinamen ".." für die Anzeige 
        // (oder Ihre getDisplayName-Methode muss dies handhaben)
        found_files.push_back(parent_dir); 
    }
    
    // 2. Durchlaufen der Einträge im aktuellen Verzeichnis
    try {
        for (const auto& entry : fs::directory_iterator(current_path)) {
            FileInfo info;
            info.path = entry.path();
            info.is_directory = entry.is_directory();
            
            // Füllen der Größe nur für reguläre Dateien
            if (entry.is_regular_file()) {
                info.size = fs::file_size(entry.path());
            }

            found_files.push_back(info);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Dateisystem-Fehler beim Scannen von " << current_path << ": " << e.what() << std::endl;
        // Optional: Status setzen, um Fehler in der TUI anzuzeigen
    }

    // 3. Sortierung: Ordner zuerst, dann Dateien (alphabetisch nach Name)
    std::sort(found_files.begin(), found_files.end(), 
        [](const FileInfo& a, const FileInfo& b) {
        
        // Regel 1: ".." (Elternverzeichnis) kommt immer zuerst (wenn die Pfade unterschiedlich sind)
        if (a.path.filename() == ".." && b.path.filename() != "..") return true;
        if (a.path.filename() != ".." && b.path.filename() == "..") return false;

        // Regel 2: Ordner vor Dateien
        if (a.is_directory != b.is_directory) {
            return a.is_directory; // true, wenn a ein Ordner ist und b nicht
        }
        
        // Regel 3: Alphabetisch nach Dateiname
        return a.path.filename().string() < b.path.filename().string();
    });

    return found_files;
}


// TODO: Die anderen Methoden (deleteFiles, findDuplicates etc.) müssen auch noch
// den Qualifizierer FileProcessor:: erhalten und die Logik implementieren.

// ... Implementierungen für deleteFiles, findDuplicates, findZeroByteFiles, getDisplayName ...