/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis;
using namespace Analysis::Utils;

class FileUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
        FileWriter fw("test_file");  // 创建test_file文件
    }
    virtual void TearDown()
    {
        File::DeleteFile("test_file");  // 删除test_file文件
    }
};

TEST_F(FileUTest, TestChmod)
{
    const mode_t mode = 0640;
    EXPECT_FALSE(File::Chmod("", mode));
    EXPECT_FALSE(File::Chmod("test_file_not_exist", mode));
    EXPECT_TRUE(File::Chmod("test_file", mode));
}

TEST_F(FileUTest, TestExist)
{
    EXPECT_FALSE(File::Exist(""));
    EXPECT_FALSE(File::Exist("test_file_not_exist"));
    EXPECT_TRUE(File::Exist("test_file"));
}

TEST_F(FileUTest, TestIsSoftLink)
{
    EXPECT_EQ(0, symlink("test_file", "test_file_soft_link"));  // 创建软连接
    EXPECT_FALSE(File::IsSoftLink(""));
    EXPECT_FALSE(File::IsSoftLink("test_file_not_exist"));
    EXPECT_FALSE(File::IsSoftLink("test_file"));
    EXPECT_TRUE(File::IsSoftLink("test_file_soft_link"));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));  // 删除test_file_soft_link软连接
}

TEST_F(FileUTest, TestIsFile)
{
    EXPECT_FALSE(File::IsFile(""));
    EXPECT_FALSE(File::IsFile("test_file_not_exist"));
    EXPECT_TRUE(File::IsFile("test_file"));
}

TEST_F(FileUTest, TestSize)
{
    EXPECT_EQ(0, File::Size(""));
    EXPECT_EQ(0, File::Size("test_file_not_exist"));
    EXPECT_EQ(0, File::Size("test_file"));
}

TEST_F(FileUTest, TestCheckDir)
{
    // 测试空路径
    EXPECT_FALSE(File::CheckDir(""));
    // 测试路径过长
    const int pathDepth = 900;
    std::vector<std::string> paths(pathDepth, "test");
    std::string path = File::PathJoin(paths);
    EXPECT_FALSE(File::CheckDir(path));
    // 测试路径包含无效字符
    EXPECT_FALSE(File::CheckDir("/test/test\ndir/"));
    // 测试不存在的目录
    EXPECT_FALSE(File::CheckDir("test_dir"));
    // 测试软链接
    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_EQ(0, symlink("test_dir", "test_dir_soft_link"));
    EXPECT_TRUE(File::CheckDir("test_dir_soft_link"));
    // 测试正常情况
    EXPECT_TRUE(File::CheckDir("test_dir"));
    EXPECT_TRUE(File::RemoveDir("test_dir", 0));
    EXPECT_TRUE(File::DeleteFile("test_dir_soft_link"));
}

TEST_F(FileUTest, TestCreateDir)
{
    const std::string workDir = "file_utest_create_dir";
    const std::string testDir = File::PathJoin({workDir, "dir"});
    const std::string testFile = File::PathJoin({workDir, "file"});
    const std::string testLink = File::PathJoin({workDir, "file_soft_link"});
    File::RemoveDir(workDir, 0);
    EXPECT_TRUE(File::CreateDir(workDir));
    FileWriter fw(testFile);
    EXPECT_TRUE(fw.IsOpen());
    fw.Close();

    EXPECT_FALSE(File::CreateDir(""));
    EXPECT_TRUE(File::CreateDir(testDir));
    EXPECT_TRUE(File::CreateDir(testDir));
    EXPECT_EQ(0, symlink(testFile.c_str(), testLink.c_str()));
    EXPECT_FALSE(File::CreateDir(testLink));
    EXPECT_TRUE(File::DeleteFile(testLink));
    EXPECT_TRUE(File::RemoveDir(workDir, 0));
}

TEST_F(FileUTest, TestRemoveDir)
{
    // 测试删除不存在的文件夹
    EXPECT_FALSE(File::RemoveDir("test_dir_not_exist", 0));
    // 测试删除软连接
    EXPECT_EQ(0, symlink("test_link_file", "test_file_soft_link"));
    EXPECT_FALSE(File::RemoveDir("test_link_file", 0));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));
    // 测试删除嵌套文件夹
    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory_done"));
    EXPECT_TRUE(File::RemoveDir("test_dir", 0));
}

TEST_F(FileUTest, TestPathJoinShouldReturnJoinedPath)
{
    EXPECT_EQ("", File::PathJoin({}));
    EXPECT_EQ("/test_dir//test_subdir/test_file", File::PathJoin({"/test_dir/", "test_subdir", "test_file"}));
}

TEST_F(FileUTest, TestGetFilesWithPrefixShouldReturn2SubFilesWhenGetFileMatchPrefix)
{
    EXPECT_EQ(0, File::GetFilesWithPrefix("", "").size());

    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory_done"));
    const size_t fileNum = 2;
    EXPECT_EQ(fileNum, File::GetFilesWithPrefix("test_dir", "directory").size());
    EXPECT_EQ(0, rmdir("test_dir/directory_done"));
    EXPECT_EQ(0, rmdir("test_dir/directory"));
    EXPECT_EQ(0, rmdir("test_dir/dir"));
    EXPECT_EQ(0, rmdir("test_dir"));
}

TEST_F(FileUTest, TestFilterFileWithSuffixShouldReturn2FilesWhenFilterSuffix)
{
    std::vector<std::string> files = {
        "test1",
        "test2",
        "test3",
        "test1.done",
        "test2.done",
    };
    const size_t fileNumNotFiltered = 5;
    EXPECT_EQ(fileNumNotFiltered, File::FilterFileWithSuffix(files, "").size());
    const size_t fileNumFiltered = 3;
    EXPECT_EQ(fileNumFiltered, File::FilterFileWithSuffix(files, "done").size());
}

TEST_F(FileUTest, TestFileCheck)
{
    EXPECT_FALSE(File::Check(""));
    const int pathDepth = 900;
    std::vector<std::string> paths(pathDepth, "test");
    std::string path = File::PathJoin(paths);
    EXPECT_FALSE(File::Check(path));
    EXPECT_EQ(0, symlink("test_file", "test_file_soft_link"));  // 创建软连接
    EXPECT_TRUE(File::Check("test_file_soft_link"));
    EXPECT_TRUE(File::Check("test_file"));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));  // 删除test_file_soft_link软连接
}

TEST_F(FileUTest, TestFileCheckShouldReturnFalseWhenFileSizeTooLarge)
{
    const uint32_t maxReadBytes = 64 * 1024 * 1024;
    MOCKER_CPP(&File::Size).stubs()
        .will(returnValue(maxReadBytes + 1));
    EXPECT_FALSE(File::Check("test_file", maxReadBytes));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnFalseWhenNotFile)
{
    EXPECT_FALSE(FileReader::Check(""));
    EXPECT_FALSE(FileReader::Check("test_file_not_exist"));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnFalseWhenFileNoReadAccess)
{
    MOCKER_CPP(&File::Access).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(FileReader::Check("test_file"));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnTrueWhenFileCheckSuccess)
{
    EXPECT_TRUE(FileReader::Check("test_file"));
}

TEST_F(FileUTest, TestFileReaderOpenShouldNotOpenWhenEmptyPath)
{
    FileReader fd;
    fd.Open("");
    EXPECT_FALSE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldOpenWhenAlreadyOpened)
{
    MOCKER_CPP(&FileReader::IsOpen).stubs()
        .will(returnValue(true));
    FileReader fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldNotOpenWhenOpenedFailed)
{
    MOCKER_CPP(&FileReader::Check).stubs()
        .will(returnValue(true));
    FileReader fd;
    fd.Open("test_file_not_exist");
    EXPECT_FALSE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldOpenWhenOpenedSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestReadBinaryShouldReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    std::stringstream ss;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadBinary(ss));
}

TEST_F(FileUTest, TestReadBinaryShouldReturnOKWhenReadSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    std::stringstream ss;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadBinary(ss));
}

TEST_F(FileUTest, TestReadTextShouldReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    std::vector<std::string> text;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadText(text));
}

TEST_F(FileUTest, TestReadTextShouldReturnOKWhenReadSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    std::vector<std::string> text;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadText(text));
}

TEST_F(FileUTest, TestReadJsonShouldReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadJson(content));
}

TEST_F(FileUTest, TestReadJsonShouldReturnErrorWhenParseJsonFailed)
{
    FileReader fd;
    fd.Open("test_file");
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadJson(content));
}

TEST_F(FileUTest, TestReadJsonShouldReturnOKWhenParseJsonSuccess)
{
    FileReader fd;
    FileWriter jsonWriter("test_file");
    ASSERT_TRUE(jsonWriter.IsOpen());
    jsonWriter.WriteText("{}");
    jsonWriter.Close();
    fd.Open("test_file");
    ASSERT_TRUE(fd.IsOpen());
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadJson(content));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnTrueWhenFileNotExist)
{
    EXPECT_TRUE(FileWriter::Check(""));
    EXPECT_TRUE(FileWriter::Check("test_file_not_exist"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnFalseWhenPathIsDir)
{
    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_FALSE(FileWriter::Check("test_dir"));
    EXPECT_EQ(0, rmdir("test_dir"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnFalseWhenFileNoWriteAccess)
{
    MOCKER_CPP(&File::Exist).stubs()
        .will(returnValue(true));
    MOCKER_CPP(&File::Access).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(FileWriter::Check("test_file"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnTrueWhenFileCheckSuccess)
{
    EXPECT_TRUE(FileWriter::Check("test_file"));
}

TEST_F(FileUTest, TestFileWriterOpenShouldNotOpenWhenEmptyPath)
{
    FileWriter fw;
    fw.Open("");
    EXPECT_FALSE(fw.IsOpen());
}

TEST_F(FileUTest, TestFileWriterOpenShouldOpenWhenAlreadyOpened)
{
    MOCKER_CPP(&FileWriter::IsOpen).stubs()
        .will(returnValue(true));
    FileWriter fw;
    fw.Open("test_file");
    EXPECT_TRUE(fw.IsOpen());
}

TEST_F(FileUTest, TestFileWriterOpenShouldOpenWhenOpenedSuccess)
{
    FileWriter fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestWriteText)
{
    FileWriter fd("test_file");
    ASSERT_TRUE(fd.IsOpen());
    fd.WriteText("test");
}

TEST_F(FileUTest, TestWriteTextBack)
{
    FileWriter fd("test_file");
    ASSERT_TRUE(fd.IsOpen());
    fd.WriteText("test");
    fd.WriteTextBack("test", 1);  // 往回覆盖1位
    fd.WriteTextBack("test", 100);  // 往回覆盖100位
}

TEST_F(FileUTest, TestGetOriginDataShouldReturn2ValidFileWhen2ValidAnd2Invalid)
{
    // 创建test_file文件
    FileWriter fw0("/tmp/prefix_test_slice_0");
    FileWriter fw1("/tmp/prefix2_test_slice_1");
    FileWriter fw2("/tmp/prefix_test_slice_0.done");
    FileWriter fw3("/tmp/prefix_test_slice_0.complete");
    fw0.Close();
    fw1.Close();
    fw2.Close();
    fw3.Close();

    int expectRes = 2;
    auto files = File::GetOriginData("/tmp", {"prefix_test_slice", "prefix2_test_slice"}, {"done", "complete"});
    EXPECT_EQ(expectRes, files.size());

    File::DeleteFile("/tmp/prefix_test_slice_0");
    File::DeleteFile("/tmp/prefix2_test_slice_1");
    File::DeleteFile("/tmp/prefix_test_slice_0.done");
    File::DeleteFile("/tmp/prefix_test_slice_0.complete");
}

TEST_F(FileUTest, TestSortFilesByAgingANdSliceNumShouldReturnSortedFilesWhenInputIsOutOfOrder)
{
    std::vector<std::string> files = {"PROF/aging.file.slice_1", "PROF/unaging.file.slice_1",
                                      "PROF/aging.file.slice_12", "PROF/unaging.file.slice_12",
                                      "PROF/aging.file.slice_6", "PROF/unaging.file.slice_6"};
    std::vector<std::string> expectFiles = {"PROF/unaging.file.slice_1", "PROF/unaging.file.slice_6",
                                            "PROF/unaging.file.slice_12", "PROF/aging.file.slice_1",
                                            "PROF/aging.file.slice_6", "PROF/aging.file.slice_12"};
    auto newFiles = File::SortFilesByAgingAndSliceNum(files);
    EXPECT_EQ(newFiles.size(), files.size());
    for (uint32_t i = 0; i < expectFiles.size(); i++) {
        EXPECT_EQ(newFiles[i], expectFiles[i]);
    }
}

TEST_F(FileUTest, TestSortFilesByAgingANdSliceNumShouldReturnSortedFilesWhenInputIsOutOfOrder2)
{
    std::vector<std::string> files = {"./PROF/host/data/unaging.additional.tensor_info.slice_0",
                                      "./PROF/host/data/aging.additional.tensor_info.slice_0",
                                      "./PROF/host/data/unaging.additional.tensor_info.slice_1",
                                      "./PROF/host/data/aging.additional.tensor_info.slice_1"};
    std::vector<std::string> expectFiles = {"./PROF/host/data/unaging.additional.tensor_info.slice_0",
                                            "./PROF/host/data/unaging.additional.tensor_info.slice_1",
                                            "./PROF/host/data/aging.additional.tensor_info.slice_0",
                                            "./PROF/host/data/aging.additional.tensor_info.slice_1"};
    auto newFiles = File::SortFilesByAgingAndSliceNum(files);
    EXPECT_EQ(newFiles.size(), files.size());
    for (uint32_t i = 0; i < expectFiles.size(); i++) {
        EXPECT_EQ(newFiles[i], expectFiles[i]);
    }
}
