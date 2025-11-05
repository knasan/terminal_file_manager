#include "filescanner.hpp"
#include <algorithm>
#include <iostream>

std::vector<FileInfo> FileScanner::scanDirectory(
    const std::string& path, 
    bool useRecursively,
    bool includeParent) const {
    
    std::vector<FileInfo> results;
    std::filesystem::path currentPath(path);

    // Parent directory hinzufügen
    if (includeParent && !useRecursively) {
        if (currentPath.has_parent_path() && 
            currentPath != currentPath.root_path()) {
            
            FileInfo parent_dir(
                currentPath.parent_path().string(),
                0,      // size
                true,   // isDir
                true    // isParent <-- NEU!
            );
            
            results.push_back(parent_dir);
        }
    }

    // 2. Directory scannen
    try {
        if (useRecursively) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                processEntry(entry, results);
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                processEntry(entry, results);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Fehler ignorieren oder loggen
    }

    // 3. Sortieren: ".." zuerst, dann Ordner, dann Dateien (alphabetisch)
    std::sort(results.begin(), results.end(), 
        [&currentPath](const FileInfo& a, const FileInfo& b) {
            std::filesystem::path path_a(a.getPath());
            std::filesystem::path path_b(b.getPath());
            
            // Regel 1: Parent-Dir (..) immer zuerst
            bool a_is_parent = (path_a == currentPath.parent_path());
            bool b_is_parent = (path_b == currentPath.parent_path());
            
            if (a_is_parent && !b_is_parent) return true;
            if (!a_is_parent && b_is_parent) return false;
            
            // Regel 2: Directories vor Files
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory();
            }
            
            // Regel 3: Alphabetisch nach Filename
            return path_a.filename().string() < path_b.filename().string();
        });

    return results;
}
