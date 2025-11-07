/**
 * @file test_fileinfo.cpp
 * @brief Unit tests for the FileInfo class
 *
 * This file contains Google Test unit tests that verify the functionality
 * of the FileInfo class including construction, display name generation,
 * size formatting, color coding, hash operations, and duplicate detection.
 *
 * @see FileInfo
 */

#include <gtest/gtest.h>
#include "fileinfo.hpp"
#include "utils.hpp"

/**
 * @test BasicConstruction
 * @brief Verifies basic FileInfo object construction and getter methods
 *
 * Tests that a FileInfo object is correctly initialized with path, size,
 * and directory flag, and that all basic getter methods return the expected
 * values.
 *
 * Expected behavior:
 * - Path matches the provided path string
 * - File size matches the provided size
 * - isDirectory() returns false for regular files
 * - isParentDir() returns false for regular files
 *
 * @see FileInfo::FileInfo()
 * @see FileInfo::getPath()
 * @see FileInfo::getFileSize()
 * @see FileInfo::isDirectory()
 * @see FileInfo::isParentDir()
 */
TEST(FileInfoTest, BasicConstruction) {
    FileInfo info("/tmp/test.txt", 1024, false);

    EXPECT_EQ(info.getPath(), "/tmp/test.txt");
    EXPECT_EQ(info.getFileSize(), 1024);
    EXPECT_FALSE(info.isDirectory());
    EXPECT_FALSE(info.isParentDir());
}

/**
 * @test DisplayName
 * @brief Verifies display name extraction and formatting for different file types
 *
 * Tests the getDisplayName() method's behavior with:
 * - Regular files: Returns basename only (e.g., "document.pdf")
 * - Directories: Returns basename with trailing slash (e.g., "folder/")
 * - Parent directory: Returns ".." regardless of actual path
 *
 * This ensures proper display formatting in file browser interfaces.
 *
 * @see FileInfo::getDisplayName()
 */
TEST(FileInfoTest, DisplayName) {
    FileInfo file("/home/user/document.pdf", 1024, false);
    EXPECT_EQ(file.getDisplayName(), "document.pdf");

    FileInfo dir("/home/user/folder", 0, true);
    EXPECT_EQ(dir.getDisplayName(), "folder/");

    FileInfo parent("/home/user", 0, true, true);
    EXPECT_EQ(parent.getDisplayName(), "..");
}

/**
 * @test SizeFormatting
 * @brief Verifies human-readable size formatting for files and directories
 *
 * Tests the formatBytes() utility function integration with FileInfo objects,
 * verifying correct formatting across different file sizes and special handling
 * for directories.
 *
 * Test cases:
 * - 0 byte file: "0 B"
 * - 1 KB file (1024 bytes): "1.0 KB"
 * - 1 MB file (1048576 bytes): "1.0 MB"
 * - Directory: "<DIR>" (special display string)
 *
 * @see formatBytes()
 * @see FileInfo::getFileSize()
 */
TEST(FileInfoTest, SizeFormatting) {
    FileInfo zero("/tmp/empty.txt", 0, false);
    EXPECT_EQ(formatBytes(zero.getFileSize()), "0 B");

    FileInfo kb("/tmp/file.txt", 1024, false);
    EXPECT_EQ(formatBytes(kb.getFileSize()), "1.0 KB");

    FileInfo mb("/tmp/large.dat", 1048576, false);
    EXPECT_EQ(formatBytes(mb.getFileSize()), "1.0 MB");

    FileInfo dir("/tmp/folder", 0, true);
    EXPECT_EQ(formatBytes(dir.getFileSize()), "<DIR>");
}

/**
 * @test ColorCodes
 * @brief Verifies color code assignment for different file types and states
 *
 * Tests the getColorCode() method which returns numeric color codes for
 * terminal display based on file characteristics.
 *
 * Color code mapping:
 * - 1 (Red): Zero-byte files
 * - 4 (Blue): Directories
 * - 3 (Yellow): Duplicate files
 *
 * This ensures proper visual distinction in terminal-based file browsers.
 *
 * @see FileInfo::getColorCode()
 * @see FileInfo::setDuplicate()
 */
TEST(FileInfoTest, ColorCodes) {
    FileInfo zero_byte("/tmp/empty.txt", 0, false);
    EXPECT_EQ(zero_byte.getColorCode(), 1);  // Red

    FileInfo directory("/tmp/folder", 0, true);
    EXPECT_EQ(directory.getColorCode(), 4);  // Blue

    FileInfo duplicate("/tmp/dup.txt", 100, false);
    duplicate.setDuplicate(true);
    EXPECT_EQ(duplicate.getColorCode(), 3);  // Yellow
}

/**
 * @test HashOperations
 * @brief Verifies hash value storage and retrieval operations
 *
 * Tests the hash-related methods of FileInfo, ensuring that:
 * - Newly created FileInfo objects have empty hash strings
 * - Hash values can be set and retrieved correctly
 *
 * Hash values are used for duplicate file detection by comparing
 * content checksums.
 *
 * @see FileInfo::getHash()
 * @see FileInfo::setHash()
 */
TEST(FileInfoTest, HashOperations) {
    FileInfo info("/tmp/test.txt", 100, false);

    EXPECT_TRUE(info.getHash().empty());

    info.setHash("ABCD1234");
    EXPECT_EQ(info.getHash(), "ABCD1234");
}

/**
 * @test DuplicateFlag
 * @brief Verifies duplicate flag operations
 *
 * Tests the duplicate flag getter and setter methods, ensuring that:
 * - Newly created files are not marked as duplicates by default
 * - The duplicate flag can be set and retrieved correctly
 *
 * The duplicate flag is typically set during duplicate file detection
 * and affects the color coding and display of files.
 *
 * @see FileInfo::isDuplicate()
 * @see FileInfo::setDuplicate()
 * @see FileInfo::getColorCode()
 */
TEST(FileInfoTest, DuplicateFlag) {
    FileInfo info("/tmp/test.txt", 100, false);

    EXPECT_FALSE(info.isDuplicate());

    info.setDuplicate(true);
    EXPECT_TRUE(info.isDuplicate());
}

/**
 * @test ZeroFilesDetection
 * @brief Verifies detection of zero-byte files
 *
 * Tests the zeroFiles() method which identifies regular files with zero size.
 *
 * Detection rules:
 * - Regular files with 0 bytes: Returns true
 * - Regular files with non-zero size: Returns false
 * - Directories (even with size 0): Returns false
 *
 * Zero-byte files are displayed with a red color code to highlight
 * potentially incomplete or problematic files.
 *
 * @see FileInfo::zeroFiles()
 * @see FileInfo::getColorCode()
 */
TEST(FileInfoTest, ZeroFilesDetection) {
    FileInfo zero("/tmp/empty.txt", 0, false);
    EXPECT_TRUE(zero.zeroFiles());

    FileInfo normal("/tmp/file.txt", 100, false);
    EXPECT_FALSE(normal.zeroFiles());

    FileInfo dir("/tmp/folder", 0, true);
    EXPECT_FALSE(dir.zeroFiles());  // Directories don't count
}