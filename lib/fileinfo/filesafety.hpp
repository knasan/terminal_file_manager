#ifndef FILESAFETY_HPP
#define FILESAFETY_HPP

#include <string>
#include <vector>
#include <unordered_set>

/**
 * @brief Safety checks for file operations
 */
class FileSafety {
public:
    enum class DeletionStatus {
        Allowed,
        BlockedSystemPath,
        BlockedHome,
        BlockedMountPoint,
        BlockedVirtualFS,
        WarningRemovableMedia
    };
    
    struct MountInfo {
        std::string device;
        std::string mountpoint;
        std::string fstype;
        bool is_removable;
        bool is_root;
    };
    
    /**
     * @brief Check if deletion is allowed for a path
     * @param path Full path to check
     * @return DeletionStatus indicating if/why deletion is blocked
     */
    static DeletionStatus checkDeletion(const std::string& path);
    
    /**
     * @brief Get human-readable message for deletion status
     */
    static std::string getStatusMessage(DeletionStatus status, const std::string& path);
    
    /**
     * @brief Check if path is a system directory
     */
    static bool isSystemPath(const std::string& path);
    
    /**
     * @brief Check if path is user's home directory
     */
    static bool isUserHome(const std::string& path);
    
    /**
     * @brief Check if path is a mount point
     */
    static bool isMountPoint(const std::string& path);
    
    /**
     * @brief Check if path is on a virtual/protected filesystem
     */
    static bool isProtectedFilesystem(const std::string& path);
    
    /**
     * @brief Check if path is on removable media (USB, etc.)
     */
    static bool isRemovableMedia(const std::string& path);
    
    /**
     * @brief Get all mount points from /proc/mounts
     */
    static std::vector<MountInfo> getMountPoints();

private:
    static const std::unordered_set<std::string> CRITICAL_PATHS;
};

#endif // FILESAFETY_HPP