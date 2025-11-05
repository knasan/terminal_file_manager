// FileProcessor.h

#include <filesystem>
#include <string>
#include <vector>

#include "fileinfo.hpp"

class FileProcessor {
public:
  // Konstruktor
  FileProcessor(std::string path) : m_path(path) {};

  // Liest Verzeichnisse und füllt die interne Datenstruktur
  std::vector<FileInfo> scanDirectory();

  // Filtert die Liste nach Dateien mit Größe 0
  std::vector<FileInfo> findZeroByteFiles();

  // Findet Duplikate basierend auf Größe und Hash
  std::vector<FileInfo> findDuplicates();

  // Führt die Löschung aus (Ihre pc.deleteFile(FileName) Logik)
  bool deleteFiles();

  private:
    const std::filesystem::path m_path;
    // const std::vector<FileInfo> m_files;
    // const std::vector<FileInfo> m_filesToDelete;
    const std::string m_filepath;

};