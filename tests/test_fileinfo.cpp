#include <gtest/gtest.h>
#include "fileinfo.hpp"

TEST(FileInfoTest, BasicConstruction) {
    FileInfo info("/tmp/test.txt", 1024, false);
    
    EXPECT_EQ(info.getPath(), "/tmp/test.txt");
    EXPECT_EQ(info.getFileSize(), 1024);
    EXPECT_FALSE(info.isDirectory());
    EXPECT_FALSE(info.isParentDir());
}

TEST(FileInfoTest, DisplayName) {
    FileInfo file("/home/user/document.pdf", 1024, false);
    EXPECT_EQ(file.getDisplayName(), "document.pdf");
    
    FileInfo dir("/home/user/folder", 0, true);
    EXPECT_EQ(dir.getDisplayName(), "folder/");
    
    FileInfo parent("/home/user", 0, true, true);
    EXPECT_EQ(parent.getDisplayName(), "..");
}

TEST(FileInfoTest, SizeFormatting) {
    FileInfo zero("/tmp/empty.txt", 0, false);
    EXPECT_EQ(zero.getSizeFormatted(), "0 B");
    
    FileInfo kb("/tmp/file.txt", 1024, false);
    EXPECT_EQ(kb.getSizeFormatted(), "1.0 KB");
    
    FileInfo mb("/tmp/large.dat", 1048576, false);
    EXPECT_EQ(mb.getSizeFormatted(), "1.0 MB");
    
    FileInfo dir("/tmp/folder", 0, true);
    EXPECT_EQ(dir.getSizeFormatted(), "<DIR>");
}

TEST(FileInfoTest, ColorCodes) {
    FileInfo zero_byte("/tmp/empty.txt", 0, false);
    EXPECT_EQ(zero_byte.getColorCode(), 1);  // Red
    
    FileInfo directory("/tmp/folder", 0, true);
    EXPECT_EQ(directory.getColorCode(), 4);  // Blue
    
    FileInfo duplicate("/tmp/dup.txt", 100, false);
    duplicate.setDuplicate(true);
    EXPECT_EQ(duplicate.getColorCode(), 3);  // Yellow
}

TEST(FileInfoTest, HashOperations) {
    FileInfo info("/tmp/test.txt", 100, false);
    
    EXPECT_TRUE(info.getHash().empty());
    
    info.setHash("ABCD1234");
    EXPECT_EQ(info.getHash(), "ABCD1234");
}

TEST(FileInfoTest, DuplicateFlag) {
    FileInfo info("/tmp/test.txt", 100, false);
    
    EXPECT_FALSE(info.isDuplicate());
    
    info.setDuplicate(true);
    EXPECT_TRUE(info.isDuplicate());
}

TEST(FileInfoTest, ZeroFilesDetection) {
    FileInfo zero("/tmp/empty.txt", 0, false);
    EXPECT_TRUE(zero.zeroFiles());
    
    FileInfo normal("/tmp/file.txt", 100, false);
    EXPECT_FALSE(normal.zeroFiles());
    
    FileInfo dir("/tmp/folder", 0, true);
    EXPECT_FALSE(dir.zeroFiles());  // Directories don't count
}