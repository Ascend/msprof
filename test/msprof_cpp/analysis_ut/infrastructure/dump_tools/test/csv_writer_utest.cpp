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
#include <set>
#include <vector>
#include <string>
#include <algorithm>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dump_tools/csv_tool/include/csv_writer.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/application/summary/summary_constant.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Application;

namespace {
    const int DEPTH = 0;
    const std::string BASE_PATH = "./dump_tools_csv_writer_utest";
    const std::string OUTPUT_DIR = File::PathJoin({BASE_PATH, Analysis::Common::OUTPUT_PATH});
}

class CsvWriterUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
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
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
};


TEST_F(CsvWriterUTest, ShouldWriteCsvFileSuccess)
{
    std::set<int> maskCols = {2, 3}; // 设置第2 3个位置是非法的表头 不需要写入
    std::string fileName = "testFile";
    CsvWriter csvWriter = CsvWriter();
    std::vector<std::string> headers = {"pos0", "pos1", "pos2", "pos3", "pos4"};
    std::string expectedHeader = "pos0,pos1,pos4";
    std::vector<std::vector<std::string>> res;
    size_t resSize = 2000000; // 极限边界场景 只生成2个csv
    EXPECT_TRUE(Reserve(res, resSize));
    std::vector<std::string> record = {"data0", "data1:timestamp 123456789123456789\t", "data2", "data3"};
    std::string expectedRecord = "data0,data1:timestamp 123456789123456789\t,";
    for (auto i = 0; i < resSize; i++) {
        std::vector<std::string> lineRecord = record;
        lineRecord.emplace_back(std::to_string(i));
        res.emplace_back(lineRecord);
    }
    csvWriter.WriteCsv(File::PathJoin({OUTPUT_DIR, fileName}), headers, res, maskCols);
    auto filesList = File::GetOriginData(OUTPUT_DIR, {fileName}, {});
    // 升序排序保证后面读文件有序
    std::sort(filesList.begin(), filesList.end());
    EXPECT_EQ(filesList.size(), (res.size() + CSV_LIMIT - 1) / CSV_LIMIT);
    size_t lineSize = 0; // 只对数据条数计数
    for (auto file : filesList) {
        FileReader fd(file);
        std::vector<std::string> text;
        EXPECT_EQ(fd.ReadText(text), Analysis::ANALYSIS_OK);
        ASSERT_FALSE(text.empty());
        ASSERT_EQ(expectedHeader, text[0]);
        for (auto i = 1; i < text.size(); i++) {
            auto expectedLineRecord = expectedRecord;
            expectedLineRecord.append(std::to_string(lineSize));
            ASSERT_EQ(expectedLineRecord, text[i]);
            lineSize++;
        }
    }
    EXPECT_EQ(lineSize, resSize); // 总数据量
}
