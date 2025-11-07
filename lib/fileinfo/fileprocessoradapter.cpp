#include "fileprocessoradapter.hpp"

/**
 * @brief Scans a directory and returns file information for all contained files
 *
 * Creates a FileScanner instance with an FNV1A hasher and delegates the
 * scanning operation to scan the directory at the adapter's configured path.
 *
 * @param include_parent_dir Whether to include the parent directory (..) in
 * results
 * @param recursive If true, recursively scan subdirectories; if false, scan
 * only the top-level directory (default: false)
 * @param progress Optional callback function for progress updates, called with
 *                 the current count of processed items (not percentage).
 *                 Pass nullptr for no callbacks
 *
 * @return std::vector<FileInfo> Vector containing FileInfo objects for each
 * file found during the scan
 *
 * @note The function creates a new FNV1A hasher and FileScanner for each call
 */
std::vector<FileInfo>
FileProcessorAdapter::scanDirectory(bool include_parent_dir, bool recursive,
                                    ProgressCallback progress) {

  FNV1A hasher;
  FileScanner scanner(hasher);

  return scanner.scanDirectory(m_path, recursive, include_parent_dir, progress);
}