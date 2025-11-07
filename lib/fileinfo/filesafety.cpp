/**
 * @file filesafety.cpp
 * @brief Implementation of file system safety checks for deletion operations
 *
 * This file implements safety mechanisms to prevent accidental deletion of
 * critical system files, directories, and special filesystems.
 */

#include "filesafety.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/vfs.h>

/**
 * @brief Critical system paths that should never be deleted
 *
 * This set contains essential system directories whose deletion would break
 * the operating system or cause severe data loss. Includes root filesystem
 * directories like /, /boot, /dev, /etc, /lib, /proc, /sys, /usr, and /var.
 */
const std::unordered_set<std::string> FileSafety::CRITICAL_PATHS = {
    "/", "/boot", "/dev", "/etc", "/lib", "/lib64",
    "/proc", "/root", "/run", "/sys", "/usr", "/var",
    "/bin", "/sbin", "/opt", "/srv", "/tmp"
};

/**
 * @brief Checks whether a path is safe to delete
 *
 * Performs a series of safety checks in order of severity to determine if
 * the specified path can be safely deleted. The checks are performed in the
 * following order:
 * 1. System paths (blocked)
 * 2. User home directory (blocked)
 * 3. Virtual filesystems like /proc, /sys (blocked)
 * 4. Mount points (blocked)
 * 5. Removable media (warning only)
 *
 * @param path The filesystem path to check
 *
 * @return DeletionStatus indicating whether deletion is allowed, blocked,
 *         or requires warning
 *
 * @see DeletionStatus
 * @see getStatusMessage()
 */
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

/**
 * @brief Converts a DeletionStatus to a human-readable message
 *
 * Generates a descriptive error or warning message based on the deletion
 * status, including the path in the message for context.
 *
 * @param status The DeletionStatus to convert to a message
 * @param path The filesystem path being checked (included in the message)
 *
 * @return std::string A human-readable message describing the status
 *
 * @note Messages for blocked operations explain why the deletion is prevented
 */
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

/**
 * @brief Checks if a path is a critical system directory
 *
 * Determines whether the given path is in the CRITICAL_PATHS set, which
 * contains essential system directories that should never be deleted.
 *
 * @param path The filesystem path to check
 *
 * @return true if the path is a critical system directory, false otherwise
 *
 * @see CRITICAL_PATHS
 */
bool FileSafety::isSystemPath(const std::string& path) {
    return CRITICAL_PATHS.count(path) > 0;
}

/**
 * @brief Checks if a path is the user's home directory
 *
 * Compares the given path with the HOME environment variable to determine
 * if it points to the user's home directory.
 *
 * @param path The filesystem path to check
 *
 * @return true if the path matches the user's home directory, false otherwise
 *
 * @note Returns false if the HOME environment variable is not set
 */
bool FileSafety::isUserHome(const std::string& path) {
    const char* home = std::getenv("HOME");
    return home && path == std::string(home);
}

/**
 * @brief Checks if a path is a filesystem mount point
 *
 * Queries the system's mount information to determine if the given path
 * is a mount point for any filesystem. This includes both physical devices
 * and virtual filesystems.
 *
 * @param path The filesystem path to check
 *
 * @return true if the path is a mount point, false otherwise
 *
 * @see getMountPoints()
 * @note Reads mount information from /proc/mounts
 */
bool FileSafety::isMountPoint(const std::string& path) {
    auto mounts = getMountPoints();

    for (const auto& mount : mounts) {
        if (mount.mountpoint == path) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks if a path resides on a protected or virtual filesystem
 *
 * Uses statfs() to determine the filesystem type and checks if it matches
 * any protected filesystem types such as procfs, sysfs, tmpfs, ramfs, devpts,
 * securityfs, or cgroup filesystems. These filesystems are virtual or special-
 * purpose and should not allow regular file deletions.
 *
 * @param path The filesystem path to check
 *
 * @return true if the path is on a protected filesystem or if statfs() fails,
 *         false if on a regular filesystem
 *
 * @note Uses Linux filesystem magic numbers from /usr/include/linux/magic.h
 * @note Returns true on error (fail-safe behavior)
 *
 * Protected filesystem types checked:
 * - procfs (0x9fa0): Process information pseudo-filesystem
 * - sysfs (0x62656572): Kernel object information
 * - tmpfs (0x01021994): Temporary memory-based filesystem
 * - ramfs (0x858458f6): RAM-based filesystem
 * - devpts (0x3434): Pseudo-terminal devices
 * - securityfs (0x73636673): Security module filesystem
 * - cgroup (0x27e0eb): Control group v1 filesystem
 * - cgroup2 (0x63677270): Control group v2 filesystem
 */
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

/**
 * @brief Checks if a path is on removable media
 *
 * Determines whether the given path resides on removable media such as USB
 * drives, SD cards, or external hard drives. The function uses two methods:
 * 1. Checks if the mount point is under typical removable media paths
 *    (/media, /mnt, /run/media)
 * 2. For SCSI devices (/dev/sd*), queries sysfs to check the removable flag
 *
 * @param path The filesystem path to check
 *
 * @return true if the path is on removable media, false otherwise
 *
 * @see getMountPoints()
 * @see MountInfo
 *
 * @note For /dev/sd* devices, reads from /sys/block/sdX/removable to
 *       determine if the device is removable (1) or fixed (0)
 * @note This check generates a warning rather than blocking deletion
 */
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

/**
 * @brief Retrieves information about all currently mounted filesystems
 *
 * Parses /proc/mounts to extract information about all mounted filesystems
 * in the system. For each mount, determines whether it's the root filesystem
 * and whether it appears to be removable media based on the mount point path.
 *
 * @return std::vector<MountInfo> Vector containing information about each
 *         mounted filesystem. Returns empty vector if /proc/mounts cannot
 *         be opened.
 *
 * @see MountInfo
 *
 * @note Mount points under /media, /mnt, or /run/media are flagged as
 *       potentially removable
 * @note This function reads from /proc/mounts which requires read access
 *       to the proc filesystem
 *
 * Each MountInfo entry contains:
 * - device: The device path (e.g., /dev/sda1)
 * - mountpoint: Where the device is mounted (e.g., /home)
 * - fstype: Filesystem type (e.g., ext4, ntfs, vfat)
 * - is_root: Whether this is the root filesystem (/)
 * - is_removable: Whether the mount point suggests removable media
 */
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