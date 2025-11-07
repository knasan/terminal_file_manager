/**
 * @file test_duplicatefinder.cpp
 * @brief Unit tests for the DuplicateFinder class
 *
 * This file contains Google Test unit tests that verify the functionality
 * of duplicate file detection, wasted space calculation, and edge case handling
 * in the DuplicateFinder class.
 *
 * @see DuplicateFinder
 * @see FileInfo
 */

#include <gtest/gtest.h>
#include "duplicatefinder.hpp"
#include "fileinfo.hpp"
#include "utils.hpp"

/**
 * @class DuplicateFinderTest
 * @brief Test fixture for DuplicateFinder unit tests
 *
 * Provides a common test environment with a vector of FileInfo objects
 * that is cleared before each test case. This fixture ensures test isolation
 * and provides a consistent starting state.
 *
 * @see DuplicateFinder
 */
class DuplicateFinderTest : public ::testing::Test {
protected:
    /** @brief Vector of FileInfo objects used across test cases */
    std::vector<FileInfo> files;

    /**
     * @brief Sets up the test environment before each test
     *
     * Clears the files vector to ensure each test starts with a clean state.
     */
    void SetUp() override {
        files.clear();
    }
};

/**
 * @test FindsNoDuplicatesInEmptyList
 * @brief Verifies that duplicate detection handles empty input correctly
 *
 * Tests the edge case where findDuplicates() is called with an empty vector.
 * Expected behavior: Returns an empty vector of duplicate groups.
 */
TEST_F(DuplicateFinderTest, FindsNoDuplicatesInEmptyList) {
    auto groups = DuplicateFinder::findDuplicates(files);
    EXPECT_TRUE(groups.empty());
}

/**
 * @test FindsNoDuplicatesWithUniqueFiles
 * @brief Verifies that unique files are not incorrectly identified as duplicates
 *
 * Tests the scenario where all files have different hash values.
 * Expected behavior:
 * - Returns empty duplicate groups vector
 * - Files are not marked with duplicate flag
 */
TEST_F(DuplicateFinderTest, FindsNoDuplicatesWithUniqueFiles) {
    files.emplace_back("/tmp/file1.txt", 100, false);
    files[0].setHash("AAAA");

    files.emplace_back("/tmp/file2.txt", 200, false);
    files[1].setHash("BBBB");

    auto groups = DuplicateFinder::findDuplicates(files);
    EXPECT_TRUE(groups.empty());

    // Files should not be marked as duplicates
    EXPECT_FALSE(files[0].isDuplicate());
    EXPECT_FALSE(files[1].isDuplicate());
}

/**
 * @test FindsDuplicates
 * @brief Verifies basic duplicate file detection with identical hashes
 *
 * Tests the core functionality where two files have the same hash value.
 * Expected behavior:
 * - Returns one duplicate group containing both files
 * - Group hash matches the file hash ("AAAA")
 * - Both files are marked with duplicate flag
 */
TEST_F(DuplicateFinderTest, FindsDuplicates) {
    files.emplace_back("/tmp/file1.txt", 100, false);
    files[0].setHash("AAAA");

    files.emplace_back("/tmp/file2.txt", 100, false);
    files[1].setHash("AAAA");  // Same hash!

    auto groups = DuplicateFinder::findDuplicates(files);

    EXPECT_EQ(groups.size(), 1);
    EXPECT_EQ(groups[0].files.size(), 2);
    EXPECT_EQ(groups[0].hash, "AAAA");

    // Both should be marked as duplicates
    EXPECT_TRUE(files[0].isDuplicate());
    EXPECT_TRUE(files[1].isDuplicate());
}

/**
 * @test CalculatesWastedSpace
 * @brief Verifies correct calculation of wasted disk space from duplicates
 *
 * Tests the wasted space calculation with three identical files.
 * The calculation keeps one copy and counts the rest as wasted space.
 *
 * Test scenario: 3 files × 100 bytes each = 300 bytes total
 * Wasted space calculation: (3 - 1) × 100 = 200 bytes
 *
 * Expected behavior:
 * - One duplicate group with wastedSpace = 200 bytes
 * - calculateWastedSpace() returns total wasted space of 200 bytes
 */
TEST_F(DuplicateFinderTest, CalculatesWastedSpace) {
    files.emplace_back("/tmp/file1.txt", 100, false);
    files[0].setHash("AAAA");

    files.emplace_back("/tmp/file2.txt", 100, false);
    files[1].setHash("AAAA");

    files.emplace_back("/tmp/file3.txt", 100, false);
    files[2].setHash("AAAA");

    auto groups = DuplicateFinder::findDuplicates(files);

    EXPECT_EQ(groups.size(), 1);
    // 3 files of 100 bytes, keep 1 → 200 bytes wasted
    EXPECT_EQ(groups[0].wastedSpace, 200);

    long long total = DuplicateFinder::calculateWastedSpace(groups);
    EXPECT_EQ(total, 200);
}

/**
 * @test IgnoresDirectories
 * @brief Verifies that directories are excluded from duplicate detection
 *
 * Tests that directory entries (even with matching hashes) are properly
 * filtered out and not considered in duplicate detection.
 *
 * Expected behavior: Returns empty groups even when directory and file
 * share the same hash value.
 */
TEST_F(DuplicateFinderTest, IgnoresDirectories) {
    files.emplace_back("/tmp/dir", 0, true);  // Directory
    files[0].setHash("AAAA");

    files.emplace_back("/tmp/file.txt", 100, false);
    files[1].setHash("AAAA");

    auto groups = DuplicateFinder::findDuplicates(files);

    // Directory should be ignored
    EXPECT_TRUE(groups.empty());
}

/**
 * @test IgnoresEmptyHashes
 * @brief Verifies that files without hash values are excluded from detection
 *
 * Tests that files with empty or unset hash values are properly filtered
 * out and do not produce false positive duplicate groups.
 *
 * Expected behavior: Returns empty groups when no files have valid hashes,
 * even if multiple files lack hashes.
 */
TEST_F(DuplicateFinderTest, IgnoresEmptyHashes) {
    files.emplace_back("/tmp/file1.txt", 100, false);
    // No hash set

    files.emplace_back("/tmp/file2.txt", 100, false);
    // No hash set

    auto groups = DuplicateFinder::findDuplicates(files);
    EXPECT_TRUE(groups.empty());
}

/**
 * @test FormatsBytesCorrectly
 * @brief Verifies human-readable byte formatting across different scales
 *
 * Tests the formatBytes() utility function with various byte values to
 * ensure correct conversion to appropriate units (B, KB, MB, GB) with
 * proper decimal precision.
 *
 * Test cases:
 * - 0 bytes → "0 B"
 * - 500 bytes → "500.0 B"
 * - 1024 bytes (1 KiB) → "1.0 KB"
 * - 1536 bytes (1.5 KiB) → "1.5 KB"
 * - 1048576 bytes (1 MiB) → "1.0 MB"
 * - 1073741824 bytes (1 GiB) → "1.0 GB"
 *
 * @see formatBytes()
 */
TEST_F(DuplicateFinderTest, FormatsBytesCorrectly) {
    EXPECT_EQ(formatBytes(0), "0 B");
    EXPECT_EQ(formatBytes(500), "500.0 B");
    EXPECT_EQ(formatBytes(1024), "1.0 KB");
    EXPECT_EQ(formatBytes(1536), "1.5 KB");
    EXPECT_EQ(formatBytes(1048576), "1.0 MB");
    EXPECT_EQ(formatBytes(1073741824), "1.0 GB");
}
