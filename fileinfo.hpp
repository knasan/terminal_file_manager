#ifndef FILE_INFO_HPP
#define FILE_INFO_HPP

/*
FileInfo ist Ihre Datenstruktur (Model). Sie definiert, wie eine Datei aussieht (Pfad, Größe, Markierung, etc.).
*/
#include <filesystem>

class FileInfo {
public:
  bool is_marked = false;
  bool is_directory = false;
  std::filesystem::path path;
  std::string hash;
  std::string getDisplayName() const {
    std::string name = path.filename().string();
    // Spezialfall für das Elternverzeichnis
    if (path.filename().empty() && path.has_parent_path()) {
        name = "..";
    }
    if (is_directory && name != "..") {
        name += "/"; // Visuelle Kennzeichnung
    }
    return name;
  }
  uintmax_t size;
};

#endif // FILE_INFO_HPP