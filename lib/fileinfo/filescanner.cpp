#include "filescanner.hpp"
#include <algorithm>
#include <iostream>

std::vector<FileInfo>
FileScanner::scanDirectory(const std::filesystem::path &dir_path,
                           bool recursive, bool include_parent,
                           ProgressCallback progress) {

  std::vector<FileInfo> results;
  int count = 0;

  // Add parent directory if requested (non-recursive only)
  if (include_parent && !recursive) {
    auto parent_path = dir_path.parent_path();
    results.emplace_back(parent_path.string(), 0, true, true);
  }

  try {
    if (recursive) {
      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(dir_path)) {
        processEntry(entry, results);

        // Call progress callback
        if (progress && ++count % 100 == 0) { // Update every 100 items
          progress(count);
        }
      }
    } else {
      for (const auto &entry : std::filesystem::directory_iterator(dir_path)) {
        processEntry(entry, results);

        if (progress && ++count % 10 == 0) { // Update every 10 items
          progress(count);
        }
      }
    }

    // Final callback
    if (progress) {
      progress(count);
    }

  } catch (...) {
    // Error handling
  } // Sorting order: ".." first, then folders, then files (alphabetical)

  sortEntries(results, include_parent);

  return results;
}

void FileScanner::sortEntries(std::vector<FileInfo> &results,
                              bool include_parent) {
  std::sort(results.begin(), results.end(),
            [include_parent](const FileInfo &a, const FileInfo &b) {
              // Parent (..) always first
              if (include_parent) {
                if (a.isParentDir())
                  return true;
                if (b.isParentDir())
                  return false;
              }

              // Directories before files
              if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory();
              }

              // Alphabetical by name
              return a.getDisplayName() < b.getDisplayName();
            });
}