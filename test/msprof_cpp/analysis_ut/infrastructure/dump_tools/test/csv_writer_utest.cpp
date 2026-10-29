/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <set>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis::Utils;
using namespace Analysis::Infra;

namespace
{
const int DEPTH = 0;
const std::string BASE_PATH = "./dump_tools_csv_writer_utest";
const std::string OUTPUT_DIR = File::PathJoin({BASE_PATH, Analysis::Common::OUTPUT_PATH});
}  // namespace

class CsvWriterUTest : public testing::Test
{
   protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH))
        {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(OUTPUT_DIR));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    virtual void SetUp() { GlobalMockObject::verify(); }
};

TEST_F(CsvWriterUTest, ShouldWriteCsvFileSuccess)
{
    const std::set<int> maskCols = {2, 3};
    const std::vector<std::string> headers = {"pos0", "pos1", "pos2", "pos3", "pos4"};
    std::string text = CsvWriter::FormatHeader(headers, maskCols);
    EXPECT_EQ(text, "pos0,pos1,pos4\n");
    // A masked column 0 must not leave a leading comma behind.
    EXPECT_EQ(CsvWriter::FormatHeader({"c0", "c1", "c2", "c3"}, {0, 2}), "c1,c3\n");
    std::string masked;
    CsvWriter::AppendRow(masked, {"v0", "v1", "v2", "v3"}, {0, 2});
    EXPECT_EQ(masked, "v1,v3\n");
    CsvWriter::AppendRow(text, {"data0", "data1:timestamp 123456789123456789\t", "data2", "data3", "7"}, maskCols);
    const std::string path = File::PathJoin({OUTPUT_DIR, "testFile.csv"});
    FileWriter writer;
    writer.Open(path, std::ios::out | std::ios::app);
    ASSERT_TRUE(writer.IsOpen());
    writer.WriteText(text);
    std::string more;
    CsvWriter::AppendRow(more, {"a", "b", "c", "d", "e"}, maskCols);
    writer.WriteText(more);
    writer.Close();

    FileReader fd(path);
    std::vector<std::string> lines;
    EXPECT_EQ(fd.ReadText(lines), Analysis::ANALYSIS_OK);
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "pos0,pos1,pos4");
    EXPECT_EQ(lines[1], "data0,data1:timestamp 123456789123456789\t,7");
    EXPECT_EQ(lines[2], "a,b,e");
}
