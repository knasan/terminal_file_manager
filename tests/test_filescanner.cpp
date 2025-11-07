/**
 * @file test_filescanner.cpp
 * @brief Unit tests for the FileScanner class
 *
 * This file contains comprehensive Google Test unit tests that verify FileScanner
 * functionality including directory scanning, sorting behavior, hash calculation,
 * recursive traversal, and edge case handling.
 *
 * ## Test Coverage
 *
 * ### Basic Scanning (5 tests)
 * - ScansEmptyDirectory: Empty directory handling
 * - ScansFilesInDirectory: Regular file detection
 * - ScansDirectories: Mixed files and directories
 * - DetectsFileSize: Accurate size reporting (1 byte, 1 KB)
 * - HandlesSpecialCharacters: Spaces, dashes, underscores in names
 *
 * ### Parent Directory Handling (4 tests)
 * - IncludesParentDirectory: Parent (..) inclusion when requested
 * - ExcludesParentDirectory: Parent (..) exclusion by default
 * - SortsParentDirectoryFirst: Parent appears first when included
 * - RecursiveScanDoesNotIncludeParent: No parent in recursive mode
 *
 * ### Sorting Behavior (4 tests)
 * - SortsDirectoriesBeforeFiles: Directories precede files
 * - SortsAlphabetically: Alphabetical order within categories
 * - SortingOrderComplete: Full hierarchical sort validation
 *   (Order: parent -> dirs alphabetically -> files alphabetically)
 *
 * ### Hash Calculation (5 tests)
 * - CalculatesHashes: Hash generation for non-empty files
 * - DoesNotHashEmptyFiles: Empty files skip hashing
 * - DoesNotHashDirectories: Directories don't get hashed
 * - IdenticalFilesHaveSameHash: Consistent hashing for same content
 * - DifferentFilesHaveDifferentHashes: Unique hashes for different content
 *
 * ### Recursive Scanning (2 tests)
 * - RecursiveScan: Deep directory traversal (3 levels)
 * - RecursiveScanDoesNotIncludeParent: Parent handling in recursive mode
 *
 * ### Edge Cases (1 test)
 * - HandlesNonExistentDirectory: Graceful handling of invalid paths
 *
 * @note All tests use FNV1A hash calculator for consistency
 * @note Tests run in isolated temporary directories with automatic cleanup
 *
 * @see FileScanner
 * @see FileInfo
 * @see FNV1A
 */

#include <gtest/gtest.h>
#include "filescanner.hpp"
#include "fnv1a.hpp"
#include <filesystem>
#include <fstream>

/**
 * @class FileScannerTest
 * @brief Test fixture for FileScanner unit tests
 *
 * Provides a test environment with a temporary test directory and helper
 * methods for creating test files and directories. Uses FNV1A hash calculator
 * for all tests to ensure consistent hashing behavior.
 *
 * The fixture provides:
 * - Isolated temporary test directory for each test
 * - Automatic cleanup after each test
 * - Helper methods for creating test files and directories
 * - Pre-configured FNV1A hasher instance
 *
 * @see FileScanner
 * @see FNV1A
 */
class FileScannerTest : public ::testing::Test {
protected:
    /** @brief Path to temporary test directory */
    std::filesystem::path test_dir;

    /** @brief FNV1A hash calculator instance used for all tests */
    FNV1A hasher;

    /**
     * @brief Sets up the test environment before each test
     *
     * Creates a fresh temporary test directory at system temp location.
     * Ensures each test starts with a clean, isolated environment.
     */
    void SetUp() override {
        // Create temp test directory
        test_dir = std::filesystem::temp_directory_path() / "filescanner_test";
        std::filesystem::create_directories(test_dir);
    }

    /**
     * @brief Cleans up the test environment after each test
     *
     * Removes the test directory and all its contents to prevent
     * test pollution and disk space waste.
     */
    void TearDown() override {
        // Cleanup
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    /**
     * @brief Helper method to create a test file with specified content
     *
     * Creates a file in the test directory with the given name and content.
     * Supports subdirectory paths (e.g., "subdir/file.txt").
     *
     * @param name Filename or relative path from test_dir
     * @param content String content to write to the file
     */
    void createFile(const std::string& name, const std::string& content) {
        auto path = test_dir / name;
        std::ofstream file(path);
        file << content;
    }

    /**
     * @brief Helper method to create a test directory
     *
     * Creates a directory (including parent directories) in the test directory.
     * Supports nested paths (e.g., "dir1/dir2/dir3").
     *
     * @param name Directory name or relative path from test_dir
     */
    void createDir(const std::string& name) {
        std::filesystem::create_directories(test_dir / name);
    }
};

/**
 * @test ScansEmptyDirectory
 * @brief Verifies scanner handles empty directories correctly
 *
 * Tests that scanning an empty directory returns an empty result vector.
 * This is the baseline test for scanner functionality.
 *
 * @see FileScanner::scanDirectory()
 */
TEST_F(FileScannerTest, ScansEmptyDirectory) {
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);

    EXPECT_TRUE(results.empty());
}

/**
 * @test ScansFilesInDirectory
 * @brief Verifies detection and cataloging of regular files
 *
 * Tests that the scanner correctly identifies all files in a directory,
 * properly classifies them as non-directories, and reports non-zero sizes.
 *
 * @see FileScanner::scanDirectory()
 * @see FileInfo::isDirectory()
 * @see FileInfo::getFileSize()
 */
TEST_F(FileScannerTest, ScansFilesInDirectory) {
    createFile("file1.txt", "content1");
    createFile("file2.txt", "content2");

    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);

    EXPECT_EQ(results.size(), 2);

    // Check files are present
    bool found_file1 = false;
    bool found_file2 = false;

    for (const auto& info : results) {
        std::string filename = std::filesystem::path(info.getPath()).filename().string();
        if (filename == "file1.txt") found_file1 = true;
        if (filename == "file2.txt") found_file2 = true;

        EXPECT_FALSE(info.isDirectory());
        EXPECT_GT(info.getFileSize(), 0);
    }

    EXPECT_TRUE(found_file1);
    EXPECT_TRUE(found_file2);
}

TEST_F(FileScannerTest, ScansDirectories) {
    createDir("subdir1");
    createDir("subdir2");
    createFile("file.txt", "test");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 3);
    
    int dir_count = 0;
    int file_count = 0;
    
    for (const auto& info : results) {
        if (info.isDirectory()) {
            dir_count++;
            EXPECT_EQ(info.getFileSize(), 0);  // Dirs have size 0
        } else {
            file_count++;
            EXPECT_GT(info.getFileSize(), 0);
        }
    }
    
    EXPECT_EQ(dir_count, 2);
    EXPECT_EQ(file_count, 1);
}

TEST_F(FileScannerTest, IncludesParentDirectory) {
    createFile("file.txt", "test");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, true);
    
    // Should have: ".." + "file.txt" = 2
    EXPECT_EQ(results.size(), 2);
    
    // First entry should be parent (..)
    EXPECT_TRUE(results[0].isParentDir());
    EXPECT_EQ(results[0].getDisplayName(), "..");
}

TEST_F(FileScannerTest, ExcludesParentDirectory) {
    createFile("file.txt", "test");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    // Should have only: "file.txt" = 1
    EXPECT_EQ(results.size(), 1);
    EXPECT_FALSE(results[0].isParentDir());
}

TEST_F(FileScannerTest, SortsParentDirectoryFirst) {
    createFile("aaa.txt", "test");
    createFile("bbb.txt", "test");
    createDir("dir");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, true);
    
    // First should be parent (..)
    EXPECT_TRUE(results[0].isParentDir());
    EXPECT_EQ(results[0].getDisplayName(), "..");
}

TEST_F(FileScannerTest, SortsDirectoriesBeforeFiles) {
    createFile("file.txt", "test");
    createDir("directory");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 2);
    
    // Directories should come before files (after parent)
    EXPECT_TRUE(results[0].isDirectory());
    EXPECT_FALSE(results[1].isDirectory());
}

TEST_F(FileScannerTest, SortsAlphabetically) {
    createFile("zebra.txt", "test");
    createFile("apple.txt", "test");
    createFile("banana.txt", "test");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 3);
    
    // Should be sorted: apple, banana, zebra
    auto name0 = std::filesystem::path(results[0].getPath()).filename().string();
    auto name1 = std::filesystem::path(results[1].getPath()).filename().string();
    auto name2 = std::filesystem::path(results[2].getPath()).filename().string();
    
    EXPECT_EQ(name0, "apple.txt");
    EXPECT_EQ(name1, "banana.txt");
    EXPECT_EQ(name2, "zebra.txt");
}

TEST_F(FileScannerTest, CalculatesHashes) {
    createFile("file1.txt", "hello world");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 1);
    
    // Hash should be calculated for non-empty file
    EXPECT_FALSE(results[0].getHash().empty());
    EXPECT_GT(results[0].getHash().length(), 0);
}

TEST_F(FileScannerTest, DoesNotHashEmptyFiles) {
    createFile("empty.txt", "");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].getFileSize(), 0);
    
    // Empty files should not have hash
    EXPECT_TRUE(results[0].getHash().empty());
}

TEST_F(FileScannerTest, DoesNotHashDirectories) {
    createDir("testdir");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].isDirectory());
    
    // Directories should not have hash
    EXPECT_TRUE(results[0].getHash().empty());
}

TEST_F(FileScannerTest, RecursiveScan) {
    createFile("root_file.txt", "root");
    createDir("subdir");
    createFile("subdir/sub_file.txt", "sub");
    createDir("subdir/deepdir");
    createFile("subdir/deepdir/deep_file.txt", "deep");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), true, false);
    
    // Should find all files recursively
    // root_file.txt, subdir, subdir/sub_file.txt, subdir/deepdir, subdir/deepdir/deep_file.txt
    EXPECT_GE(results.size(), 3);  // At least 3 files
    
    // Check deep file is found
    bool found_deep = false;
    for (const auto& info : results) {
        if (info.getPath().find("deep_file.txt") != std::string::npos) {
            found_deep = true;
            break;
        }
    }
    EXPECT_TRUE(found_deep);
}

TEST_F(FileScannerTest, RecursiveScanDoesNotIncludeParent) {
    createFile("file.txt", "test");
    createDir("subdir");
    createFile("subdir/sub.txt", "sub");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), true, true);
    
    // Parent (..) should NOT be included in recursive scan
    for (const auto& info : results) {
        EXPECT_FALSE(info.isParentDir());
    }
}

TEST_F(FileScannerTest, HandlesSpecialCharacters) {
    createFile("file with spaces.txt", "test");
    createFile("file-with-dashes.txt", "test");
    createFile("file_with_underscores.txt", "test");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 3);
    
    for (const auto& info : results) {
        EXPECT_FALSE(info.getPath().empty());
    }
}

TEST_F(FileScannerTest, DetectsFileSize) {
    createFile("small.txt", "x");          // 1 byte
    createFile("medium.txt", std::string(1024, 'x'));  // 1 KB
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 2);
    
    // Find each file and check size
    for (const auto& info : results) {
        std::string filename = std::filesystem::path(info.getPath()).filename().string();
        if (filename == "small.txt") {
            EXPECT_EQ(info.getFileSize(), 1);
        } else if (filename == "medium.txt") {
            EXPECT_EQ(info.getFileSize(), 1024);
        }
    }
}

TEST_F(FileScannerTest, IdenticalFilesHaveSameHash) {
    createFile("file1.txt", "identical content");
    createFile("file2.txt", "identical content");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 2);
    
    // Both files should have the same hash
    EXPECT_EQ(results[0].getHash(), results[1].getHash());
    EXPECT_FALSE(results[0].getHash().empty());
}

TEST_F(FileScannerTest, DifferentFilesHaveDifferentHashes) {
    createFile("file1.txt", "content A");
    createFile("file2.txt", "content B");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, false);
    
    EXPECT_EQ(results.size(), 2);
    
    // Files should have different hashes
    EXPECT_NE(results[0].getHash(), results[1].getHash());
}

TEST_F(FileScannerTest, HandlesNonExistentDirectory) {
    FileScanner scanner(hasher);
    
    // Should not throw, but return empty or handle gracefully
    EXPECT_NO_THROW({
        auto results = scanner.scanDirectory("/nonexistent/path", false, false);
        // Depending on implementation, might be empty or throw
    });
}

TEST_F(FileScannerTest, SortingOrderComplete) {
    // Create mixed content
    createFile("zebra.txt", "test");
    createDir("apple_dir");
    createFile("banana.txt", "test");
    createDir("cherry_dir");
    
    FileScanner scanner(hasher);
    auto results = scanner.scanDirectory(test_dir.string(), false, true);
    
    // Expected order:
    // 1. .. (parent)
    // 2. apple_dir (directory, alphabetically first)
    // 3. cherry_dir (directory, alphabetically second)
    // 4. banana.txt (file, alphabetically)
    // 5. zebra.txt (file, alphabetically)
    
    EXPECT_EQ(results.size(), 5);
    
    EXPECT_TRUE(results[0].isParentDir());
    EXPECT_TRUE(results[1].isDirectory());
    EXPECT_TRUE(results[2].isDirectory());
    EXPECT_FALSE(results[3].isDirectory());
    EXPECT_FALSE(results[4].isDirectory());
    
    auto name1 = std::filesystem::path(results[1].getPath()).filename().string();
    auto name2 = std::filesystem::path(results[2].getPath()).filename().string();
    auto name3 = std::filesystem::path(results[3].getPath()).filename().string();
    auto name4 = std::filesystem::path(results[4].getPath()).filename().string();
    
    EXPECT_EQ(name1, "apple_dir");
    EXPECT_EQ(name2, "cherry_dir");
    EXPECT_EQ(name3, "banana.txt");
    EXPECT_EQ(name4, "zebra.txt");
}