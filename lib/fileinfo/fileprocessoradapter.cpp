#include "fileprocessoradapter.hpp"

std::vector<FileInfo>
FileProcessorAdapter::scanDirectory(bool include_parent_dir, bool recursive,
                                    ProgressCallback progress) {

  FNV1A hasher;
  FileScanner scanner(hasher);

  return scanner.scanDirectory(m_path, recursive, include_parent_dir, progress);
}