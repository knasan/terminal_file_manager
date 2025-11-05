#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "fileinfo.hpp"
#include "filescanner.hpp"
#include "fnv1a.hpp"

// Example Application

/**
 * @class Application
 * @brief High-level entrypoint for scanning a filesystem and analyzing results.
 *
 * This class orchestrates a filesystem scan, collects metadata for discovered
 * files, and provides simple analyses printed to standard output:
 *  - Detection of zero-length files (potentially defective/corrupt files).
 *  - Detection and grouping of duplicate files using the FNV-1a content hash.
 *
 * The class holds a single mutable member:
 *  - std::vector<FileInfo> allFiles: an in-memory collection of file metadata
 *    produced by FileScanner::scanDirectory.
 *
 * Responsibilities
 *  - Create and configure a FileScanner (using an FNV1A hasher instance) and
 *    invoke it to populate allFiles.
 *  - Present a human-readable analysis summary on stdout:
 *      * showZeroFiles() lists files whose size is 0 bytes and prints a count.
 *      * showDuplicates() groups non-directory, non-empty files by their
 *        precomputed hash and prints groups that contain more than one entry.
 *
 * Usage
 *  - Call run(startPath, recursiv) to perform a scan and immediately output
 *    the analysis. The method prints progress and summary information to stdout.
 *
 * Public API
 *  - void run(const std::string &startPath, bool recursiv)
 *      @param startPath  Path of the directory from which the scan begins.
 *      @param recursiv   If true, scan directories recursively; otherwise only
 *                       scan the top-level directory.
 *      @effects Populates allFiles with results from FileScanner and prints
 *               analysis output to stdout.
 *      @note The method constructs an FNV1A hasher and a FileScanner each time
 *            it is invoked. Any exceptions raised by FileScanner (e.g. I/O
 *            errors) will propagate unless handled by the caller.
 *
 * Private helpers (behavior summarized)
 *  - void showZeroFiles() const
 *      Scans allFiles for entries reporting a size of 0 bytes, prints each
 *      candidate path with a warning marker, and prints a final count.
 *
 *  - void showDuplicates()
 *      Builds a mapping from file-hash (std::string) to a list of references
 *      to FileInfo objects. Only includes entries that:
 *        * are not directories,
 *        * have file size > 0,
 *        * have a non-empty hash.
 *      Prints a summary of the number of unique hash values and then lists
 *      each hash that maps to multiple FileInfo entries (duplicate groups),
 *      showing path and size for each file in the group.
 *
 * Implementation notes
 *  - The duplicate detection uses std::unordered_map<std::string,
 *    std::vector<std::reference_wrapper<const FileInfo>>> to avoid copying
 *    FileInfo objects when grouping by hash.
 *  - Output is written directly to std::cout; this class is designed for CLI
 *    usage and not for use as a library component that returns structured
 *    results.
 *  - Complexity:
 *      * Scanning: cost depends on FileScanner implementation and filesystem
 *        content.
 *      * Duplicate grouping: linear in the number of files (hash table
 *        insertions), plus linear-time printing of groups.
 *
 * Thread-safety
 *  - Application is not thread-safe. It owns mutable state (allFiles) and
 *    performs printing to shared stdout. Concurrent calls to run() or to the
 *    private helpers must be externally synchronized.
 *
 * Error handling
 *  - The class does not catch or translate exceptions thrown by FileScanner
 *    or underlying filesystem operations. Callers should handle exceptions as
 *    appropriate for their use case.
 *
 * See also
 *  - FNV1A: the hashing strategy used for duplicate detection.
 *  - FileScanner::scanDirectory: produces the FileInfo collection consumed by
 *    this class.
 *  - FileInfo: provides metadata accessors used by the analyses.
 */
class Application {
private:
  std::vector<FileInfo> allFiles;

public:
  void run(const std::string &startPath, bool recursiv) {
    FNV1A fileHash;
    FileScanner scanner(fileHash);

    std::cout << "Scan directory: " << startPath << std::endl;
    allFiles = scanner.scanDirectory(startPath, recursiv);
    std::cout << "Scan finished. " << allFiles.size() << " Entries found."
              << std::endl;

    showZeroFiles();
    showDuplicates();
  }

private:
  void showZeroFiles() const {
    int corruptFileCounter = 0;

    std::cout << "\n--- Analysis of the results ---" << std::endl;

    for (const auto &info : allFiles) {
      if (info.zeroFiles()) {
        std::cout << "⚠️ Possibly defective (0 Bytes): " << info.getPath()
                  << std::endl;
        corruptFileCounter++;
      }
    }
    std::cout << "All potentially defective files: " << corruptFileCounter
              << std::endl;
  }

  void showDuplicates() {
    std::cout << "\n--- Duplicate detection (FNV-1a Hash) ---" << std::endl;

    std::unordered_map<std::string,
                       std::vector<std::reference_wrapper<const FileInfo>>>
        hashToFile;

    for (const auto &info : allFiles) {
      if (!info.isDirectory() && info.getFileSize() > 0 &&
          !info.getHash().empty()) {
        hashToFile[info.getHash()].push_back(std::cref(info));
      }
    }

    std::cout << "Grouping complete. " << hashToFile.size()
              << " Unique hash values ​​found." << std::endl;

    int totalDupGroups = 0;

    for (const auto &paar : hashToFile) {
      const std::vector<std::reference_wrapper<const FileInfo>> &files =
          paar.second;

      if (files.size() > 1) {
        totalDupGroups++;
        std::cout << "\n# DUPLICATE GROUP" << totalDupGroups
                  << " (Hash: " << paar.first << ", " << files.size()
                  << " files)" << std::endl;
        // ...
        for (const auto &fileRef : files) {
          const FileInfo &file = fileRef.get(); // <--- Zugriff auf das Objekt
          std::cout << "    -> Path: " << file.getPath()
                    << " (Size: " << file.getFileSize() << " Bytes)"
                    << std::endl;
        }
      }
    }

    if (totalDupGroups == 0) {
      std::cout << "\nNo duplicate groups found." << std::endl;
    } else {
      std::cout << "\nTotal " << totalDupGroups << " Duplicate groups found."
                << std::endl;
    }
  }
};

int main(int argc, char *argv[]) {
  Application app;
  std::string current_dir = std::filesystem::current_path();

  bool isRecursive = false;
  std::string startPath;

  // Einfacher Argument-Parser
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--recursive") {
      isRecursive = true;
    }

    if (arg == "-p" || arg == "--path") {
      startPath = argv[i + 1];
      i++;
    }

    if (arg == "-h" || arg == "--help") {
      std::cout << "[-p directory | -r (optional use recursive, defalt: false) ]\n";
      return 0;
    }
  }

  if (startPath == "") {
    startPath = current_dir;
  }

  app.run(startPath, isRecursive);

  return 0;
}
