#ifndef NULLBYTESFINDER_HPP
#define NULLBYTESFINDER_HPP

#include "fileinfo.hpp"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief Service for 0 bytes file detection
 */
class NullBytesFinder {
public:
    struct NullBytesGroup {
        std::string hash;
        std::vector<FileInfo*> files;
        long long wastedSpace = 0;  // Total size - 1 file (keep original)
    };
    
    /**
     * @brief Find 0 bytes and mark them in the vector
     * @param files Vector of FileInfo to analyze (will be modified!)
     * @return Vector of 0 bytes groups
     */
    static std::vector<NullBytesGroup> findDuplicates(std::vector<FileInfo>& files) {
        std::unordered_map<std::string, std::vector<FileInfo*>> hashMap;
        
        // Group by hash
        for (auto& info : files) {
            if (!info.isDirectory() && 
                info.getFileSize() == 0 && 
                !info.getHash().empty()) {
                hashMap[info.getHash()].push_back(&info);
            }
        }
        
        // Extract 0 bytes and mark them
        std::vector<NullBytesGroup> groups;
        for (auto& [hash, fileList] : hashMap) {
            if (fileList.size() > 1) {
                NullBytesGroup group;
                group.hash = hash;
                
                for (auto* file : fileList) {
                    file->setBroken(true);  // Mark as 0 byte
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
};

#endif // NULLBYTESFINDER_HPP