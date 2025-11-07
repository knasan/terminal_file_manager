#ifndef DUPLICATEFINDER_HPP
#define DUPLICATEFINDER_HPP

#include "fileinfo.hpp"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief Service for duplicate file detection based on file hashes
 *
 * DuplicateFinder identifies duplicate files by grouping them based on their
 * hash values. Files with identical hashes are considered duplicates. The class
 * provides functionality to:
 * - Find and mark duplicate files
 * - Calculate wasted disk space from duplicates
 * - Group duplicates by hash for further processing
 *
 * @note Files must have valid hash values set before calling findDuplicates()
 * @note Only regular files (not directories) are considered for duplication
 * @note Zero-byte files are ignored
 *
 * @see FileInfo::setHash()
 * @see DuplicateGroup
 *
 * Example usage:
 * @code
 * std::vector<FileInfo> files = scanner.scanDirectory("/path");
 * auto groups = DuplicateFinder::findDuplicates(files);
 * long long wasted = DuplicateFinder::calculateWastedSpace(groups);
 * std::cout << "Wasted space: " << wasted << " bytes\n";
 * @endcode
 */
class DuplicateFinder {
public:
    struct DuplicateGroup {
        std::string hash;
        std::vector<FileInfo*> files;
        long long wastedSpace = 0;  // Total size - 1 file (keep original)
    };
    
    /**
     * @brief Find duplicates and mark them in the vector
     * @param files Vector of FileInfo to analyze (will be modified!)
     * @return Vector of duplicate groups
     */
    static std::vector<DuplicateGroup> findDuplicates(std::vector<FileInfo>& files) {
        std::unordered_map<std::string, std::vector<FileInfo*>> hashMap;
        
        // Group by hash
        for (auto& info : files) {
            if (!info.isDirectory() && 
                info.getFileSize() > 0 && 
                !info.getHash().empty()) {
                hashMap[info.getHash()].push_back(&info);
            }
        }
        
        // Extract duplicates and mark them
        std::vector<DuplicateGroup> groups;
        for (auto& [hash, fileList] : hashMap) {
            if (fileList.size() > 1) {
                DuplicateGroup group;
                group.hash = hash;
                
                for (auto* file : fileList) {
                    file->setDuplicate(true);  // Mark as duplicate
                    group.files.push_back(file);
                }
                
                // Calculate wasted space (keep 1, rest is waste)
                group.wastedSpace = 
                    (fileList.size() - 1) * fileList[0]->getFileSize();
                
                groups.push_back(group);
            }
        }
        
        return groups;
    }
    
    /**
     * @brief Calculate total wasted space
     */
    static long long calculateWastedSpace(const std::vector<DuplicateGroup>& groups) {
        long long total = 0;
        for (const auto& group : groups) {
            total += group.wastedSpace;
        }
        return total;
    }
};

#endif // DUPLICATEFINDER_HPP