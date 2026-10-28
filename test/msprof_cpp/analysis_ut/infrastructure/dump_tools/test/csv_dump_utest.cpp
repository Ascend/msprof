/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include <vector>
#include <sstream>
#include <gtest/gtest.h>
#include "sample/sample_types.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"

using namespace testing;
using namespace Analysis;
using namespace Infra;

namespace {

std::stringstream& operator<<(std::stringstream& ss, const DumpToolSampleCsvStruct& sample)
{
    constexpr size_t tpSize = std::tuple_size<DumpToolSampleCsvTp>{};
    if (sample.columns.size() != tpSize) {
        return ss;
    }
    size_t i = 0;
    for (const auto& column : sample.columns) {
        ++i;
        ss << column;
        if (i != sample.columns.size()) {
            ss << ',';
        } else {
            ss << "\r\n";
        }
    }

    for (const auto& row : sample.data) {
        DumpInCsvFormat(ss, row);
    }
    return ss;
}

}

TEST(CsvDumpUTest, ShouldGetRightCsvWhenGivenTupleFormatData)
{
    // given
    DumpToolSampleCsvStruct sampleStruct = {
        {"u64", "i64", "string", "db", "fl", "i", "bl"},
        {
            {9823372036854775807ULL, 9223372036854775807LL, "test_string", 3.1415926535897932384626, 2.7182818,
             100, true},
            {200, -100, "test_string2", -3.1415926535897932384626, -2.7182818,
             -100, false}
        }
    };

    // when
    std::stringstream csvStream;
    csvStream << sampleStruct;  // 这里调用 std::stringstream& operator<<(std::stringstream& ss, ...

    // then
    std::string expectStr = "u64,i64,string,db,fl,i,bl\r\n"
                            "9823372036854775807,9223372036854775807,test_string,3.14159,2.71828,100,1\r\n"
                            "200,-100,test_string2,-3.14159,-2.71828,-100,0\r\n";
    EXPECT_EQ(expectStr, csvStream.str());
}

TEST(CsvDumpUTest, ShouldGetNullCsvStringWhenColumnSizeWrong)
{
    // given
    DumpToolSampleCsvStruct sampleStruct = {
            {"u64", "i64", "string", "db", "fl", "i"}, // 和正常相比，少了一个
            {{0, 0, "", 1.1, 1.1f, 0, true}}
    };

    // when
    std::stringstream csvStream;
    csvStream << sampleStruct;

    std::string expectStr = "";
    EXPECT_EQ(expectStr, csvStream.str());
}
TEST(CsvDumpUTest, TestWriteToFileWhenContentIsEmptyThenReturnFalse)
{
    std::string fileName = "test_file.txt";
    const char *content = nullptr;
    std::size_t len = 100;
    bool result = DumpTool::WriteToFile(fileName, content, len, FileCategory::MSPROF);
    EXPECT_FALSE(result);
}
TEST(CsvDumpUTest, TestWriteToFileWhenLenIsZeroThenReturnFalse)
{
    std::string fileName = "test_file.txt";
    const char *content = "Test Content";
    std::size_t len = 0;
    bool result = DumpTool::WriteToFile(fileName, content, len, FileCategory::MSPROF);
    EXPECT_FALSE(result);
}
TEST(CsvDumpUTest, TestWriteToFileWhenFileCategoryInvalidThenReturnFalse)
{
    std::string fileName = "test_file.txt";
    const char *content = "Test Content";
    std::size_t len = 0;
    bool result = DumpTool::WriteToFile(fileName, content, len, FileCategory::DEFAULT);
    EXPECT_FALSE(result);
}