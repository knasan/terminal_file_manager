#include "filescanner.hpp"
#include <algorithm>

std::vector<FileInfo> FileScanner::scanDirectory(
    const std::string& path, 
    bool useRecursively,
    bool includeParent) const {
    
    std::vector<FileInfo> results;
    std::filesystem::path currentPath(path);

    // 1. Parent directory hinzufügen (wenn gewünscht und nicht recursive)
    if (includeParent && !useRecursively && currentPath.has_parent_path()) {
        FileInfo parent_dir(
            currentPath.parent_path().string(),
            0,
            true
        );
        results.push_back(parent_dir);
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

    // 3. Sortieren (Ordner zuerst, dann alphabetisch)
    std::sort(results.begin(), results.end(), 
        [](const FileInfo& a, const FileInfo& b) {
            std::filesystem::path path_a(a.getPath());
            std::filesystem::path path_b(b.getPath());
            
            // Parent dir (..) immer zuerst
            if (path_a.filename() == ".." && path_b.filename() != "..") return true;
            if (path_a.filename() != ".." && path_b.filename() == "..") return false;

            // Directories vor Files
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory();
            }
            
            // Alphabetisch
            return path_a.filename().string() < path_b.filename().string();
        });

    return results;
}
