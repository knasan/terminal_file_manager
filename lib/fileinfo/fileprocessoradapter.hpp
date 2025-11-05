// tui/fileprocessoradapter.hpp
#ifndef FILEPROCESSORADAPTER_HPP
#define FILEPROCESSORADAPTER_HPP

#include "filescanner.hpp"
#include "fnv1a.hpp"
#include "duplicatefinder.hpp"
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
    FileProcessorAdapter(const std::filesystem::path& path)
        : m_path(path), m_scanner(m_hasher) {}
    
    std::vector<FileInfo> scanDirectory(bool includeParent = true) {
        return m_scanner.scanDirectory(m_path.string(), false, includeParent);
    }
    
    void calculateHashes(std::vector<FileInfo>& files) {
        // Already done by FileScanner!
        // This is a no-op for compatibility
    }
    
    std::vector<DuplicateFinder::DuplicateGroup> findDuplicates(
        std::vector<FileInfo>& files
    ) {
        return DuplicateFinder::findDuplicates(files);
    }
};

#endif