#ifndef FILESCANNER_HPP
#define FILESCANNER_HPP

#include <filesystem>
#include <vector>

#include "fileinfo.hpp"
#include "ihashcalculator.hpp"

class FileScanner {
private:
  const IHashCalculator &hashCalculator;

public:
  FileScanner(const IHashCalculator &calculator) : hashCalculator(calculator) {}

  std::vector<FileInfo> scanDirectory(const std::string &path,
                                      bool useRecursively = false,
                                      bool includeParent = false) const;

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
};

#endif // FILESCANNER_HPP