#include <gtest/gtest.h>
#include "filesafety.hpp"
#include <filesystem>
#include <fstream>

class FileSafetyTest : public ::testing::Test {
protected:
    std::filesystem::path test_dir;
    
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
    
    void TearDown() override {
        // Cleanup
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
};

// Test 1
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

// Test 2
TEST_F(FileSafetyTest, BlocksUserHome) {
    const char* home = std::getenv("HOME");
    ASSERT_NE(home, nullptr);
    
    EXPECT_EQ(FileSafety::checkDeletion(home), 
              FileSafety::DeletionStatus::BlockedHome);
}

// Test 3
TEST_F(FileSafetyTest, AllowsNormalFiles) {
    auto test_file = test_dir / "test.txt";
    std::ofstream(test_file) << "test";
    
    EXPECT_EQ(FileSafety::checkDeletion(test_file.string()), 
              FileSafety::DeletionStatus::Allowed);
}

// Test 4
TEST_F(FileSafetyTest, DetectsVirtualFilesystems) {
    EXPECT_EQ(FileSafety::checkDeletion("/proc/self"), 
              FileSafety::DeletionStatus::BlockedVirtualFS);
    EXPECT_EQ(FileSafety::checkDeletion("/sys/class"), 
              FileSafety::DeletionStatus::BlockedVirtualFS);
}

// Test 5
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

// Test 6
TEST_F(FileSafetyTest, StatusMessages) {
    auto msg = FileSafety::getStatusMessage(
        FileSafety::DeletionStatus::BlockedSystemPath, 
        "/etc"
    );
    
    EXPECT_NE(msg.find("system"), std::string::npos);
    EXPECT_NE(msg.find("/etc"), std::string::npos);
}

// Test 7
TEST_F(FileSafetyTest, IsSystemPath) {
    EXPECT_TRUE(FileSafety::isSystemPath("/"));
    EXPECT_TRUE(FileSafety::isSystemPath("/etc"));
    EXPECT_FALSE(FileSafety::isSystemPath("/home/user/test"));
}

// Test 8
TEST_F(FileSafetyTest, IsUserHome) {
    const char* home = std::getenv("HOME");
    if (home) {
        EXPECT_TRUE(FileSafety::isUserHome(home));
        EXPECT_FALSE(FileSafety::isUserHome("/tmp"));
    }
}