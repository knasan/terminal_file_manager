#ifndef DUPLICATEFINDER_HPP
#define DUPLICATEFINDER_HPP

#include "fileinfo.hpp"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief Service for duplicate file detection
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
    
    /**
     * @brief Format bytes to human-readable string
     */
    static std::string formatBytes(long long bytes) {
        if (bytes == 0) return "0 B";
        
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f %s", size, units[unit]);
        return std::string(buf);
    }
};

#endif // DUPLICATEFINDER_HPP