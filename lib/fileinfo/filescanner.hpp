/**
 * @file filescanner.hpp
 * @brief Directory scanning and file information collection
 *
 * This header defines the FileScanner class which provides functionality for
 * traversing directories, collecting file information, and calculating file hashes.
 */

#ifndef FILESCANNER_HPP
#define FILESCANNER_HPP

#include <filesystem>
#include <vector>
#include <atomic>
#include <functional>

#include "fileinfo.hpp"
#include "ihashcalculator.hpp"

/**
 * @class FileScanner
 * @brief Scans directories and collects file information with hash calculation
 *
 * FileScanner traverses filesystem directories (recursively or non-recursively)
 * and builds a collection of FileInfo objects. It integrates hash calculation
 * for regular files using an injected IHashCalculator implementation.
 *
 * Key features:
 * - Recursive and non-recursive directory scanning
 * - Automatic file hash calculation for non-empty regular files
 * - Progress reporting via callbacks or atomic counters
 * - Sorted output with directories before files
 * - Parent directory (..) inclusion support
 *
 * @see FileInfo
 * @see IHashCalculator
 */
class FileScanner {
private:
  /** @brief Hash calculator used for computing file hashes */
  const IHashCalculator &hashCalculator;

  /** @brief Optional atomic counter for thread-safe progress tracking */
  std::atomic<int> *m_progress_counter = nullptr;

public:
  /**
   * @brief Sets an atomic progress counter for thread-safe progress tracking
   *
   * Allows external code to monitor scanning progress through a shared atomic
   * counter. The counter is incremented as files are processed.
   *
   * @param counter Pointer to atomic integer counter, or nullptr to disable
   *
   * @note This is separate from the ProgressCallback mechanism
   * @note The counter is not reset by this class; caller manages initialization
   */
  void setProgressCounter(std::atomic<int> *counter) {
    m_progress_counter = counter;
  }

  /**
   * @brief Callback function type for progress notifications
   *
   * Function signature: void(int count)
   * - count: Number of items processed so far
   *
   * @see scanDirectory()
   */
  using ProgressCallback = std::function<void(int count)>;

  /**
   * @brief Constructs a FileScanner with the specified hash calculator
   *
   * Initializes the scanner with a hash calculator implementation that will
   * be used to compute file hashes during directory traversal.
   *
   * @param calculator Reference to hash calculator implementation (e.g., FNV1A)
   *
   * @note The calculator reference must remain valid for the lifetime of the
   *       FileScanner object
   * @see IHashCalculator
   */
  FileScanner(const IHashCalculator &calculator) : hashCalculator(calculator) {}

  /**
   * @brief Scans a directory and returns file information
   *
   * Traverses the specified directory path and collects FileInfo objects for
   * all discovered files and subdirectories. Calculates hashes for non-empty
   * regular files using the configured hash calculator.
   *
   * @param dir_path The filesystem path to scan
   * @param recursive If true, recursively scan all subdirectories
   * @param include_parent If true (and non-recursive), include parent directory (..)
   * @param progress Optional callback for progress updates (default: nullptr)
   *
   * @return std::vector<FileInfo> Sorted collection of file information objects
   *
   * @see FileInfo
   * @see ProgressCallback
   * @see processEntry()
   * @see sortEntries()
   *
   * @note Implementation is in filescanner.cpp
   */
  std::vector<FileInfo> scanDirectory(
      const std::filesystem::path &dir_path,
      bool recursive,
      bool include_parent,
      ProgressCallback progress = nullptr);  // callback

private:
  /**
   * @brief Processes a single directory entry and adds it to results
   *
   * Extracts file information from a directory entry, calculates file size,
   * computes hash for non-empty regular files, and appends the FileInfo
   * object to the results vector.
   *
   * Processing steps:
   * 1. Determine if entry is a directory
   * 2. Get file size for regular files (with error handling)
   * 3. Create FileInfo object with path, size, and directory flag
   * 4. Calculate and set hash for non-empty regular files
   * 5. Add to results vector
   *
   * @param entry The filesystem directory entry to process
   * @param results Vector to append the FileInfo object to
   *
   * @note Inline implementation for performance
   * @note File size errors are silently ignored (size defaults to 0)
   * @note Hash calculation is skipped for directories and empty files
   * @note Uses the injected hashCalculator for hash computation
   *
   * @see FileInfo
   * @see IHashCalculator::calculateHash()
   */
  void processEntry(const std::filesystem::directory_entry &entry,
                    std::vector<FileInfo> &results) const {
    long long size = 0;
    bool isDir = entry.is_directory();

    if (!isDir) {
      try {
        size = entry.file_size();
      } catch (...) {
        // Ignore errors
      }
    }

    FileInfo info(entry.path().string(), size, isDir);

    // Hash only for non-empty regular files
    if (!isDir && size > 0) {
      std::string hash = hashCalculator.calculateHash(info.getPath());
      info.setHash(hash);
    }

    results.push_back(info);
  }

  /**
   * @brief Sorts directory entries in hierarchical order
   *
   * Sorts FileInfo objects with the following priority:
   * 1. Parent directory (..) first (if include_parent is true)
   * 2. Directories before files
   * 3. Alphabetical by display name within each category
   *
   * @param results Vector of FileInfo objects to sort in-place
   * @param include_parent If true, ensures parent directory is sorted first
   *
   * @see FileInfo::isParentDir()
   * @see FileInfo::isDirectory()
   * @see FileInfo::getDisplayName()
   *
   * @note Implementation is in filescanner.cpp
   */
  void sortEntries(std::vector<FileInfo> &results, bool include_parent);
};

#endif // FILESCANNER_HPP