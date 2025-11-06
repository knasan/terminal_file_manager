// tui/fileprocessoradapter.hpp
#ifndef FILEPROCESSORADAPTER_HPP
#define FILEPROCESSORADAPTER_HPP

#include "duplicatefinder.hpp"
#include "filescanner.hpp"
#include "fnv1a.hpp"
#include <filesystem>

/**
 * @brief Adapter to bridge old FileProcessor interface with new lib
 */
class FileProcessorAdapter {
private:
  std::filesystem::path m_path;
  FNV1A m_hasher;
  FileScanner m_scanner;

public:
  using ProgressCallback = std::function<void(int)>;
  
  FileProcessorAdapter(const std::filesystem::path &path)
      : m_path(path), m_scanner(m_hasher) {}

    // recursive as parameter
  std::vector<FileInfo> scanDirectory(
      bool include_parent_dir,
      bool recursive = false,  // default false!
      ProgressCallback progress = nullptr);

  void calculateHashes(std::vector<FileInfo> &) {
    // Already done by FileScanner!
    // This is a no-op for compatibility
  }



  std::vector<DuplicateFinder::DuplicateGroup>
  findDuplicates(std::vector<FileInfo> &files) {
    return DuplicateFinder::findDuplicates(files);
  }
};

#endif