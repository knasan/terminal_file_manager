#include <gtest/gtest.h>
#include "duplicatefinder.hpp"
#include "fileinfo.hpp"
#include "utils.hpp"

class DuplicateFinderTest : public ::testing::Test {
protected:
    std::vector<FileInfo> files;
    
    void SetUp() override {
        files.clear();
    }
};

TEST_F(DuplicateFinderTest, FindsNoDuplicatesInEmptyList) {
    auto groups = DuplicateFinder::findDuplicates(files);
    EXPECT_TRUE(groups.empty());
}

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

TEST_F(DuplicateFinderTest, IgnoresDirectories) {
    files.emplace_back("/tmp/dir", 0, true);  // Directory
    files[0].setHash("AAAA");
    
    files.emplace_back("/tmp/file.txt", 100, false);
    files[1].setHash("AAAA");
    
    auto groups = DuplicateFinder::findDuplicates(files);
    
    // Directory should be ignored
    EXPECT_TRUE(groups.empty());
}

TEST_F(DuplicateFinderTest, IgnoresEmptyHashes) {
    files.emplace_back("/tmp/file1.txt", 100, false);
    // No hash set
    
    files.emplace_back("/tmp/file2.txt", 100, false);
    // No hash set
    
    auto groups = DuplicateFinder::findDuplicates(files);
    EXPECT_TRUE(groups.empty());
}

TEST_F(DuplicateFinderTest, FormatsBytesCorrectly) {
    EXPECT_EQ(formatBytes(0), "0 B");
    EXPECT_EQ(formatBytes(500), "500.0 B");
    EXPECT_EQ(formatBytes(1024), "1.0 KB");
    EXPECT_EQ(formatBytes(1536), "1.5 KB");
    EXPECT_EQ(formatBytes(1048576), "1.0 MB");
    EXPECT_EQ(formatBytes(1073741824), "1.0 GB");
}
