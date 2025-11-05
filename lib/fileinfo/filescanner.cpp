#include "filescanner.hpp"
#include <algorithm>
#include <iostream>

std::vector<FileInfo> FileScanner::scanDirectory(
    const std::string& path, 
    bool useRecursively,
    bool includeParent) const {
    
    std::vector<FileInfo> results;
    std::filesystem::path currentPath(path);

    // Add parent directory
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

    // 2. Directory scan
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
        // Ignore or log errors
    }

    // Sorting order: ".." first, then folders, then files (alphabetical)
    std::sort(results.begin(), results.end(), 
        [&currentPath](const FileInfo& a, const FileInfo& b) {
            std::filesystem::path path_a(a.getPath());
            std::filesystem::path path_b(b.getPath());
            
            // Rule 1: The parent directory (..) always comes first.
            bool a_is_parent = (path_a == currentPath.parent_path());
            bool b_is_parent = (path_b == currentPath.parent_path());
            
            if (a_is_parent && !b_is_parent) return true;
            if (!a_is_parent && b_is_parent) return false;
            
            // Rule 2: Directories before files
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory();
            }
            
            // Rule 3: Alphabetical order by filename
            return path_a.filename().string() < path_b.filename().string();
        });

    return results;
}
