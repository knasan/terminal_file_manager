#include "fileprocessoradapter.hpp"

std::vector<FileInfo> FileProcessorAdapter::scanDirectory(
    bool include_parent_dir, bool recursive,
    ProgressCallback progress) {
  
      // FIX false, no recursive - options
  return m_scanner.scanDirectory(m_path, recursive, include_parent_dir, progress);
}