/**
 * @file test_filesafety.cpp
 * @brief Unit tests for the FileSafety class
 *
 * This file contains Google Test unit tests that verify filesystem safety
 * mechanisms including system path protection, virtual filesystem detection,
 * mount point detection, and user home directory protection.
 *
 * @see FileSafety
 */

#include <gtest/gtest.h>
#include "filesafety.hpp"
#include <filesystem>
#include <fstream>

/**
 * @class FileSafetyTest
 * @brief Test fixture for FileSafety unit tests
 *
 * Provides a test environment with a dedicated test directory created on a
 * normal filesystem (not tmpfs) to ensure realistic testing of filesystem
 * safety checks. The test directory is located at $HOME/.cache/filesafety_test.
 *
 * The fixture ensures:
 * - Clean test environment setup before each test
 * - Proper cleanup after each test
 * - Tests run on real filesystems, not virtual ones
 *
 * @see FileSafety
 */
class FileSafetyTest : public ::testing::Test {
protected:
    /** @brief Path to the test directory on a normal filesystem */
    std::filesystem::path test_dir;

    /**
     * @brief Sets up the test environment before each test
     *
     * Creates a fresh test directory at $HOME/.cache/filesafety_test on a
     * normal filesystem (avoiding tmpfs). Removes any existing test directory
     * to ensure a clean state.
     *
     * @note Uses $HOME/.cache to ensure tests run on a regular filesystem
     * @note Asserts that HOME environment variable is set
     */
    void SetUp() override {
        // Use normal filesystem, not tmpfs
        const char* home = std::getenv("HOME");
        ASSERT_NE(home, nullptr);

        test_dir = std::filesystem::path(home) / ".cache" / "filesafety_test";

        // Cleanup old test dir if exists
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }

        // Create fresh test directory
        std::filesystem::create_directories(test_dir);
    }

    /**
     * @brief Cleans up the test environment after each test
     *
     * Removes the test directory and all its contents to prevent test
     * pollution and disk space waste.
     */
    void TearDown() override {
        // Cleanup
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
};

/**
 * @test BlocksSystemPaths
 * @brief Verifies that critical system directories are blocked from deletion
 *
 * Tests the first level of safety checks by attempting to delete essential
 * system paths. All critical directories in CRITICAL_PATHS should return
 * BlockedSystemPath status.
 *
 * Tested paths:
 * - / (root filesystem)
 * - /etc (system configuration)
 * - /usr (system binaries and libraries)
 * - /proc (process information pseudo-filesystem)
 *
 * Expected behavior: All return DeletionStatus::BlockedSystemPath
 *
 * @see FileSafety::checkDeletion()
 * @see FileSafety::isSystemPath()
 * @see FileSafety::CRITICAL_PATHS
 */
TEST_F(FileSafetyTest, BlocksSystemPaths) {
    EXPECT_EQ(FileSafety::checkDeletion("/"),
              FileSafety::DeletionStatus::BlockedSystemPath);
    EXPECT_EQ(FileSafety::checkDeletion("/etc"),
              FileSafety::DeletionStatus::BlockedSystemPath);
    EXPECT_EQ(FileSafety::checkDeletion("/usr"),
              FileSafety::DeletionStatus::BlockedSystemPath);
    EXPECT_EQ(FileSafety::checkDeletion("/proc"),
              FileSafety::DeletionStatus::BlockedSystemPath);
}

/**
 * @test BlocksUserHome
 * @brief Verifies that the user's home directory is protected from deletion
 *
 * Tests that the home directory identified by the HOME environment variable
 * is blocked from deletion to prevent catastrophic data loss.
 *
 * Expected behavior: Returns DeletionStatus::BlockedHome
 *
 * @see FileSafety::checkDeletion()
 * @see FileSafety::isUserHome()
 */
TEST_F(FileSafetyTest, BlocksUserHome) {
    const char* home = std::getenv("HOME");
    ASSERT_NE(home, nullptr);

    EXPECT_EQ(FileSafety::checkDeletion(home),
              FileSafety::DeletionStatus::BlockedHome);
}

/**
 * @test AllowsNormalFiles
 * @brief Verifies that regular files on normal filesystems are allowed for deletion
 *
 * Tests that files in non-protected locations pass all safety checks and
 * are permitted for deletion. Uses a test file in $HOME/.cache to ensure
 * it's on a regular filesystem.
 *
 * Expected behavior: Returns DeletionStatus::Allowed
 *
 * @see FileSafety::checkDeletion()
 */
TEST_F(FileSafetyTest, AllowsNormalFiles) {
    auto test_file = test_dir / "test.txt";
    std::ofstream(test_file) << "test";

    EXPECT_EQ(FileSafety::checkDeletion(test_file.string()),
              FileSafety::DeletionStatus::Allowed);
}

/**
 * @test DetectsVirtualFilesystems
 * @brief Verifies detection of virtual and pseudo-filesystems
 *
 * Tests that paths on virtual filesystems (procfs, sysfs) are correctly
 * identified and blocked. These filesystems contain kernel information
 * and should never allow regular deletions.
 *
 * Tested virtual filesystems:
 * - /proc/self (procfs)
 * - /sys/class (sysfs)
 *
 * Expected behavior: Returns DeletionStatus::BlockedVirtualFS
 *
 * @see FileSafety::checkDeletion()
 * @see FileSafety::isProtectedFilesystem()
 */
TEST_F(FileSafetyTest, DetectsVirtualFilesystems) {
    EXPECT_EQ(FileSafety::checkDeletion("/proc/self"),
              FileSafety::DeletionStatus::BlockedVirtualFS);
    EXPECT_EQ(FileSafety::checkDeletion("/sys/class"),
              FileSafety::DeletionStatus::BlockedVirtualFS);
}

/**
 * @test GetsMountPoints
 * @brief Verifies retrieval of system mount point information
 *
 * Tests that getMountPoints() successfully parses /proc/mounts and returns
 * mount information. Validates that the root filesystem (/) is always present
 * and correctly marked with is_root flag.
 *
 * Expected behavior:
 * - Returns non-empty vector of mount points
 * - Root filesystem (/) is present in the list
 * - Root mount point has is_root flag set to true
 *
 * @see FileSafety::getMountPoints()
 * @see FileSafety::MountInfo
 */
TEST_F(FileSafetyTest, GetsMountPoints) {
    auto mounts = FileSafety::getMountPoints();

    EXPECT_FALSE(mounts.empty());

    // Root should always be mounted
    bool has_root = false;
    for (const auto& mount : mounts) {
        if (mount.mountpoint == "/") {
            has_root = true;
            EXPECT_TRUE(mount.is_root);
            break;
        }
    }
    EXPECT_TRUE(has_root);
}

/**
 * @test StatusMessages
 * @brief Verifies generation of human-readable status messages
 *
 * Tests that getStatusMessage() produces appropriate descriptive messages
 * for different deletion statuses. Messages should include both the status
 * reason and the specific path being checked.
 *
 * Test case: BlockedSystemPath for /etc
 * Expected: Message contains "system" and "/etc"
 *
 * @see FileSafety::getStatusMessage()
 * @see FileSafety::DeletionStatus
 */
TEST_F(FileSafetyTest, StatusMessages) {
    auto msg = FileSafety::getStatusMessage(
        FileSafety::DeletionStatus::BlockedSystemPath,
        "/etc"
    );

    EXPECT_NE(msg.find("system"), std::string::npos);
    EXPECT_NE(msg.find("/etc"), std::string::npos);
}

/**
 * @test IsSystemPath
 * @brief Verifies system path detection logic
 *
 * Tests the isSystemPath() helper method which checks if a path is in the
 * CRITICAL_PATHS set. This is the first check in the safety chain.
 *
 * Test cases:
 * - / (root): Should return true
 * - /etc (system config): Should return true
 * - /home/user/test (user file): Should return false
 *
 * @see FileSafety::isSystemPath()
 * @see FileSafety::CRITICAL_PATHS
 */
TEST_F(FileSafetyTest, IsSystemPath) {
    EXPECT_TRUE(FileSafety::isSystemPath("/"));
    EXPECT_TRUE(FileSafety::isSystemPath("/etc"));
    EXPECT_FALSE(FileSafety::isSystemPath("/home/user/test"));
}

/**
 * @test IsUserHome
 * @brief Verifies user home directory detection
 *
 * Tests the isUserHome() method which checks if a path matches the HOME
 * environment variable. This prevents accidental deletion of the user's
 * entire home directory.
 *
 * Expected behavior:
 * - $HOME path returns true
 * - Other paths (like /tmp) return false
 *
 * @see FileSafety::isUserHome()
 * @note Test is conditional on HOME environment variable being set
 */
TEST_F(FileSafetyTest, IsUserHome) {
    const char* home = std::getenv("HOME");
    if (home) {
        EXPECT_TRUE(FileSafety::isUserHome(home));
        EXPECT_FALSE(FileSafety::isUserHome("/tmp"));
    }
}