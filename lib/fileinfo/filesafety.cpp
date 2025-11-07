#include "filesafety.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/vfs.h>

// Critical system paths that should never be deleted
const std::unordered_set<std::string> FileSafety::CRITICAL_PATHS = {
    "/", "/boot", "/dev", "/etc", "/lib", "/lib64",
    "/proc", "/root", "/run", "/sys", "/usr", "/var",
    "/bin", "/sbin", "/opt", "/srv", "/tmp"
};

FileSafety::DeletionStatus FileSafety::checkDeletion(const std::string& path) {
    // Check in order of severity
    
    // 1. System paths
    if (isSystemPath(path)) {
        return DeletionStatus::BlockedSystemPath;
    }
    
    // 2. User home
    if (isUserHome(path)) {
        return DeletionStatus::BlockedHome;
    }
    
    // 3. Virtual filesystems (proc, sys, etc.)
    if (isProtectedFilesystem(path)) {
        return DeletionStatus::BlockedVirtualFS;
    }
    
    // 4. Mount points
    if (isMountPoint(path)) {
        return DeletionStatus::BlockedMountPoint;
    }
    
    // 5. Removable media (warning, not blocked)
    if (isRemovableMedia(path)) {
        return DeletionStatus::WarningRemovableMedia;
    }
    
    return DeletionStatus::Allowed;
}

std::string FileSafety::getStatusMessage(DeletionStatus status, const std::string& path) {
    switch (status) {
        case DeletionStatus::Allowed:
            return "Deletion allowed";
        case DeletionStatus::BlockedSystemPath:
            return "Cannot delete system directory: " + path;
        case DeletionStatus::BlockedHome:
            return "Cannot delete your home directory: " + path;
        case DeletionStatus::BlockedMountPoint:
            return "Cannot delete mount point: " + path;
        case DeletionStatus::BlockedVirtualFS:
            return "Cannot delete virtual/system filesystem: " + path;
        case DeletionStatus::WarningRemovableMedia:
            return "This is on removable media: " + path;
        default:
            return "Unknown status";
    }
}

bool FileSafety::isSystemPath(const std::string& path) {
    return CRITICAL_PATHS.count(path) > 0;
}

bool FileSafety::isUserHome(const std::string& path) {
    const char* home = std::getenv("HOME");
    return home && path == std::string(home);
}

bool FileSafety::isMountPoint(const std::string& path) {
    auto mounts = getMountPoints();
    
    for (const auto& mount : mounts) {
        if (mount.mountpoint == path) {
            return true;
        }
    }
    
    return false;
}

bool FileSafety::isProtectedFilesystem(const std::string& path) {
    struct statfs fs_info;
    
    if (statfs(path.c_str(), &fs_info) != 0) {
        return true;  // On error, assume protected
    }
    
    // Magic numbers for protected filesystem types
    // See: /usr/include/linux/magic.h
    const long PROTECTED_FS[] = {
        0x9fa0,       // PROC_SUPER_MAGIC (procfs)
        0x62656572,   // SYSFS_MAGIC (sysfs)
        0x01021994,   // TMPFS_MAGIC (tmpfs)
        0x858458f6,   // RAMFS_MAGIC (ramfs)
        0x3434,       // DEVPTS_SUPER_MAGIC (devpts)
        0x73636673,   // SECURITYFS_MAGIC (securityfs)
        0x27e0eb,     // CGROUP_SUPER_MAGIC (cgroup)
        0x63677270,   // CGROUP2_SUPER_MAGIC (cgroup2)
    };
    
    for (auto magic : PROTECTED_FS) {
        if (fs_info.f_type == magic) {
            return true;
        }
    }
    
    return false;
}

bool FileSafety::isRemovableMedia(const std::string& path) {
    auto mounts = getMountPoints();
    
    for (const auto& mount : mounts) {
        // Check if path is on this mount
        if (path.find(mount.mountpoint) == 0) {
            if (mount.is_removable) {
                return true;
            }
            
            // Additional check: Query sysfs for removable flag
            if (mount.device.find("/dev/sd") == 0) {
                // e.g. /dev/sda1 → /sys/block/sda/removable
                std::string device_name = mount.device.substr(5, 3);  // "sda"
                std::string sysfs_path = "/sys/block/" + device_name + "/removable";
                
                std::ifstream removable_file(sysfs_path);
                if (removable_file.is_open()) {
                    int removable = 0;
                    removable_file >> removable;
                    if (removable == 1) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

std::vector<FileSafety::MountInfo> FileSafety::getMountPoints() {
    std::vector<MountInfo> mounts;
    std::ifstream mounts_file("/proc/mounts");
    
    if (!mounts_file.is_open()) {
        return mounts;  // Return empty on error
    }
    
    std::string line;
    while (std::getline(mounts_file, line)) {
        std::istringstream iss(line);
        MountInfo info;
        std::string options, dump, pass;
        
        iss >> info.device >> info.mountpoint >> info.fstype >> options >> dump >> pass;
        
        // Determine flags
        info.is_root = (info.mountpoint == "/");
        info.is_removable = (info.mountpoint.find("/media") == 0 || 
                             info.mountpoint.find("/mnt") == 0 ||
                             info.mountpoint.find("/run/media") == 0);
        
        mounts.push_back(info);
    }
    
    return mounts;
}