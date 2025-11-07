#ifndef FILE_INFO_HPP
#define FILE_INFO_HPP

#include <filesystem>

/**
 * @brief Represents file system entry information for the terminal file manager.
 *
 * This class encapsulates metadata about files and directories, including path,
 * size, hash, and various state flags. It provides methods for retrieving formatted
 * information suitable for display in a terminal interface with color coding.
 */
class FileInfo {
private:
  std::string m_path;         ///< Full path to the file or directory
  long long m_size;           ///< Size in bytes (0 for directories)
  std::string m_hash;         ///< Hash value for duplicate detection
  bool m_isDir;               ///< True if this is a directory
  bool m_isDuplicate = false; ///< True if marked as duplicate
  bool m_isParent = false;    ///< True if this represents the parent directory (..)
  // bool m_broken = false;      ///< True if the file/link is broken

  /**
   * @brief Checks if the file has executable permissions.
   * @return True if the file is executable by the owner, false otherwise.
   * @note Always returns false for directories.
   */
  bool isExecutable() const {
    if (m_isDir)
      return false;

    namespace fs = std::filesystem;
    try {
      auto perms = fs::status(m_path).permissions();
      return (perms & fs::perms::owner_exec) != fs::perms::none;
    } catch (...) {
      return false;
    }
  }

public:
  /**
   * @brief Constructs a FileInfo object.
   * @param p Full path to the file or directory.
   * @param s Size in bytes (should be 0 for directories).
   * @param isDir True if this is a directory.
   * @param isParent True if this represents the parent directory (..), defaults to false.
   */
  FileInfo(const std::string &p, long long s, bool isDir, bool isParent = false)
      : m_path(p), m_size(s), m_hash(""), m_isDir(isDir), m_isParent(isParent) {
  }

  /**
   * @brief Gets the full path to the file or directory.
   * @return Constant reference to the path string.
   */
  const std::string &getPath() const { return m_path; }

  /**
   * @brief Gets the file size in bytes.
   * @return Size in bytes (0 for directories).
   */
  long long getFileSize() const { return m_size; }

  /**
   * @brief Gets the hash value used for duplicate detection.
   * @return Constant reference to the hash string.
   */
  const std::string &getHash() const { return m_hash; }

  /**
   * @brief Checks if this entry is a directory.
   * @return True if directory, false if file.
   */
  bool isDirectory() const { return m_isDir; }

  /**
   * @brief Gets the display name for terminal output.
   * @return Formatted name: ".." for parent dir, name with "/" suffix for directories,
   *         plain filename for files, or full path for root/current directory.
   */
  std::string getDisplayName() const {
    if (m_isParent)
      return ".."; // <-- Parent Dir

    std::filesystem::path p(m_path);
    std::string name = p.filename().string();

    if (name.empty() && m_isDir) {
      // Root or current dir
      return p.string();
    }

    return m_isDir ? name + "/" : name;
  }

  /**
   * @brief Checks if this entry represents the parent directory.
   * @return True if this is the parent directory (..), false otherwise.
   */
  bool isParentDir() const { return m_isParent; }

  /**
   * @brief Gets the color code for terminal display.
   * @return ANSI color code: 1 (red) for zero-byte files, 3 (yellow) for duplicates,
   *         4 (blue) for directories, 2 (green) for executables, 7 (white) for normal files.
   */
  int getColorCode() const {
    if (m_size == 0 && !m_isDir)
      return 1; // Red: 0-Byte Files
    if (m_isDuplicate)
      return 3; // Yellow: duplicate
    if (m_isDir)
      return 4; // Blue: Directories
    if (isExecutable())
      return 2; // Green: Executables
    return 7;   // White: Normal
  }

  /**
   * @brief Checks if this file is marked as a duplicate.
   * @return True if marked as duplicate, false otherwise.
   */
  bool isDuplicate() const { return m_isDuplicate; }

  /**
   * @brief Sets the duplicate flag for this file.
   * @param dup True to mark as duplicate, false otherwise.
   */
  void setDuplicate(bool dup) { m_isDuplicate = dup; }

  /**
   * @brief Sets the hash value for duplicate detection.
   * @param hash The hash string to set.
   */
  void setHash(const std::string &hash) { m_hash = hash; }

  /**
   * @brief Checks if this is a zero-byte file (not a directory).
   * @return True if this is a file with size 0, false otherwise.
   */
  bool zeroFiles() const { return (m_size == 0 && !m_isDir); }
};

#endif // FILE_INFO_HPP
