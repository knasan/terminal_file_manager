#ifndef FILESCANNER_HPP
#define FILESCANNER_HPP

#include <filesystem>
#include <vector>
#include <atomic>
#include <functional>

#include "fileinfo.hpp"
#include "ihashcalculator.hpp"

class FileScanner {
private:
  const IHashCalculator &hashCalculator;

  std::atomic<int> *m_progress_counter = nullptr;

public:
  void setProgressCounter(std::atomic<int> *counter) {
    m_progress_counter = counter;
  }

  using ProgressCallback = std::function<void(int count)>;
  
  FileScanner(const IHashCalculator &calculator) : hashCalculator(calculator) {}

 std::vector<FileInfo> scanDirectory(
      const std::filesystem::path &dir_path, 
      bool recursive, 
      bool include_parent,
      ProgressCallback progress = nullptr);  // callback

private:
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

  void sortEntries(std::vector<FileInfo> &results, bool include_parent);
};

#endif // FILESCANNER_HPP