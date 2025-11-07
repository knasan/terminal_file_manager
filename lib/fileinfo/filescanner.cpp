/**
 * @file filescanner.cpp
 * @brief Implementation of directory scanning functionality for file
 * information
 *
 * This file implements the FileScanner class which provides directory traversal
 * and file information collection capabilities with progress reporting.
 */

#include "filescanner.hpp"
#include <algorithm>
#include <iostream>

/**
 * @brief Scans a directory and collects file information
 *
 * Traverses a directory (optionally recursively) and collects FileInfo objects
 * for all entries. The function supports progress callbacks for long operations
 * and can optionally include the parent directory (..) in the results.
 *
 * Progress callbacks are invoked periodically during scanning:
 * - Every 100 items in recursive mode
 * - Every 10 items in non-recursive mode
 * - Once at the end with the final count
 *
 * Results are automatically sorted with the following order:
 * 1. Parent directory (..) if included
 * 2. Directories (alphabetical)
 * 3. Files (alphabetical)
 *
 * @param dir_path The directory path to scan (e.g., /home/users/foobar)
 * @param recursive If true, recursively scan all subdirectories; if false,
 *                  scan only the top-level directory
 * @param include_parent_dir If true and non-recursive, include parent directory
 *                           (..) as the first entry in results
 * @param progress Optional callback function for progress updates. Called with
 *                 the current count of processed items. Pass nullptr for no
 * callbacks.
 *
 * @return std::vector<FileInfo> Sorted vector of FileInfo objects for all
 *         discovered files and directories
 *
 * @see FileInfo
 * @see processEntry()
 * @see sortEntries()
 * @see ProgressCallback
 *
 * @note Exceptions during directory iteration are caught and ignored (empty
 * catch block)
 * @note Parent directory is only included in non-recursive mode to avoid
 * confusion
 * @note Progress callback receives item count, not percentage
 */
std::vector<FileInfo>
FileScanner::scanDirectory(const std::filesystem::path &dir_path,
                           bool recursive, bool include_parent_dir,
                           ProgressCallback progress) {

  std::vector<FileInfo> results;
  int count = 0;

  // Add parent directory if requested (non-recursive only)
  if (include_parent_dir && !recursive) {
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
    // Unexpected error
  }

  // Sorting order: ".." first, then folders, then files (alphabetical)
  sortEntries(results, include_parent_dir);

  return results;
}

/**
 * @brief Sorts directory entries in a specific hierarchical order
 *
 * Implements a three-tier sorting strategy for file system entries:
 * 1. Parent directory (..) appears first (if include_parent_dir is true)
 * 2. Directories appear before regular files
 * 3. Within each category, entries are sorted alphabetically by display name
 *
 * This sorting order provides an intuitive directory listing where navigation
 * entries appear first, followed by subdirectories, and finally files.
 *
 * @param results Vector of FileInfo objects to sort in-place
 * @param include_parent_dir If true, ensures parent directory (..) is sorted
 *                           to the first position
 *
 * @see FileInfo::isParentDir()
 * @see FileInfo::isDirectory()
 * @see FileInfo::getDisplayName()
 *
 * @note The sort is performed in-place using std::sort with a custom
 * comparator
 * @note The comparator uses lexicographical comparison for alphabetical
 * ordering
 *
 * Sorting logic:
 * - Parent directory check: a.isParentDir() returns true → a comes first
 * - Directory vs file: a.isDirectory() != b.isDirectory() → directory comes
 * first
 * - Same type: a.getDisplayName() < b.getDisplayName() → alphabetical order
 */
void FileScanner::sortEntries(std::vector<FileInfo> &results,
                              bool include_parent_dir) {
  std::sort(results.begin(), results.end(),
            [include_parent_dir](const FileInfo &a, const FileInfo &b) {
              // Parent (..) always first
              if (include_parent_dir) {
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