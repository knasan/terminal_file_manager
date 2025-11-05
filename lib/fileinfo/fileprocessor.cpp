#include <filesystem>
#include <iostream>
#include <algorithm>
#include <vector>

#include "fileprocessor.hpp"
#include "fileinfo.hpp"

namespace fs = std::filesystem;

std::vector<FileInfo> FileProcessor::scanDirectory() {
    std::vector<FileInfo> found_files;
    const fs::path& current_path = m_path; 

    // 1. Eintrag für das Elternverzeichnis hinzufügen ("..")
    if (current_path.has_parent_path()) {
        FileInfo parent_dir(
            current_path.parent_path().string(),  // path
            0,                                     // size
            true                                   // isDir
        );
        found_files.push_back(parent_dir); 
    }
    
    // 2. Durchlaufen der Einträge im aktuellen Verzeichnis
    try {
        for (const auto& entry : fs::directory_iterator(current_path)) {
            long long size = 0;
            bool isDir = entry.is_directory();
            
            // Größe nur für reguläre Dateien
            if (entry.is_regular_file()) {
                size = fs::file_size(entry.path());
            }

            FileInfo info(
                entry.path().string(),  // path
                size,                   // size
                isDir                   // isDir
            );
            
            found_files.push_back(info);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Dateisystem-Fehler beim Scannen von " << current_path 
                  << ": " << e.what() << std::endl;
    }

    // 3. Sortierung: Ordner zuerst, dann Dateien
    std::sort(found_files.begin(), found_files.end(), 
        [](const FileInfo& a, const FileInfo& b) {
        
        // Pfade als filesystem::path für Vergleich
        fs::path path_a(a.getPath());
        fs::path path_b(b.getPath());
        
        // Regel 1: ".." (Elternverzeichnis) kommt immer zuerst
        if (path_a.filename() == ".." && path_b.filename() != "..") return true;
        if (path_a.filename() != ".." && path_b.filename() == "..") return false;

        // Regel 2: Ordner vor Dateien
        if (a.isDirectory() != b.isDirectory()) {
            return a.isDirectory(); // true wenn a Ordner ist und b nicht
        }
        
        // Regel 3: Alphabetisch nach Dateiname
        return path_a.filename().string() < path_b.filename().string();
    });

    return found_files;
}