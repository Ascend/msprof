/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/ccu_add_info_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string HOST_PATH = "./ccu_add_info_dumper_utest";
const std::string DB_PATH = "./ccu_add_info_dumper_utest/sqlite/ccu_add_info.db";
}  // namespace

class CcuAddInfoDBDumperUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (File::Exist(HOST_PATH))
        {
            ASSERT_TRUE(File::RemoveDir(HOST_PATH, 0));
        }
        ASSERT_TRUE(File::CreateDir(HOST_PATH));
    }

    void TearDown() override
    {
        ASSERT_TRUE(File::RemoveDir(HOST_PATH, 0));
    }
};

TEST_F(CcuAddInfoDBDumperUTest, TestDumpDataShouldCreatePythonCompatibleTables)
{
    CcuInfoData data;
    data.taskRecords.push_back({0, 1, "10", "20", 0, 2, 1, 7, 1, 2, 3});
    data.waitSignalRecords.push_back({0, "11", "20", 0, 2, 1, 1, 8, 1, 39, 2, 210, 3, 4, 5});
    data.groupRecords.push_back({0, "12", "20", 0, 2, 1, 1, 9, 1, 119, 2,
                                 "SUM", "INT32", "INT32", 8192, 4, 5});
    data.groupRecords.push_back({0, "13", "20", 0, 2, 1, 1, 10, 1, 120, 2,
                                 "13", "13", "13", 4096, 6, 7});

    CcuAddInfoDBDumper dumper(HOST_PATH);
    ASSERT_TRUE(dumper.DumpData(data));

    DBRunner runner(DB_PATH);
    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t>> taskRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM CCUTaskInfo ORDER BY rowid", taskRows));
    ASSERT_EQ(1U, taskRows.size());
    EXPECT_EQ("10", std::get<2>(taskRows[0]));
    EXPECT_EQ(7U, std::get<7>(taskRows[0]));
    EXPECT_EQ(3U, std::get<10>(taskRows[0]));

    std::vector<std::tuple<uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t>> waitRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM CCUWaitSignalInfo ORDER BY rowid", waitRows));
    ASSERT_EQ(1U, waitRows.size());
    EXPECT_EQ(210U, std::get<11>(waitRows[0]));
    EXPECT_EQ(4U, std::get<13>(waitRows[0]));
    EXPECT_EQ(5U, std::get<14>(waitRows[0]));

    std::vector<std::tuple<std::string, std::string, std::string>> enumStorageTypes;
    ASSERT_TRUE(runner.QueryData(
        "SELECT typeof(reduce_op_type), typeof(input_data_type), typeof(output_data_type) "
        "FROM CCUGroupInfo ORDER BY rowid",
        enumStorageTypes));
    ASSERT_EQ(2U, enumStorageTypes.size());
    EXPECT_EQ("text", std::get<0>(enumStorageTypes[0]));
    EXPECT_EQ("text", std::get<1>(enumStorageTypes[0]));
    EXPECT_EQ("text", std::get<2>(enumStorageTypes[0]));
    EXPECT_EQ("integer", std::get<0>(enumStorageTypes[1]));
    EXPECT_EQ("integer", std::get<1>(enumStorageTypes[1]));
    EXPECT_EQ("integer", std::get<2>(enumStorageTypes[1]));
}

TEST_F(CcuAddInfoDBDumperUTest, TestDumpEmptyDataShouldNotCreateDatabase)
{
    CcuInfoData data;
    CcuAddInfoDBDumper dumper(HOST_PATH);
    EXPECT_TRUE(dumper.DumpData(data));
    EXPECT_FALSE(File::Exist(DB_PATH));
}

TEST_F(CcuAddInfoDBDumperUTest, TestZeroRowSourceShouldCreateTableBeforeCompletionMarker)
{
    const std::string sourcePath = File::PathJoin({HOST_PATH, "ccu_task_source"});
    CcuInfoData data;
    data.taskSourceParsed = true;
    data.completedFiles.push_back(sourcePath);
    CcuAddInfoDBDumper dumper(HOST_PATH);

    ASSERT_TRUE(dumper.DumpData(data));
    DBRunner runner(DB_PATH);
    EXPECT_TRUE(runner.CheckTableExists("CCUTaskInfo"));
    EXPECT_TRUE(File::Exist(sourcePath + ".complete"));
}

TEST_F(CcuAddInfoDBDumperUTest, TestZeroRowRetryShouldRejectExistingRowsWithoutCreatingMarkers)
{
    CcuInfoData original;
    original.taskRecords.push_back({0, 1, "10", "20", 0, 2, 1, 7, 1, 2, 3});
    CcuAddInfoDBDumper dumper(HOST_PATH);
    ASSERT_TRUE(dumper.DumpData(original));

    const std::string sourcePath = File::PathJoin({HOST_PATH, "zero_row_source"});
    CcuInfoData retry;
    retry.taskSourceParsed = true;
    retry.completedFiles.push_back(sourcePath);
    EXPECT_FALSE(dumper.DumpData(retry));
    EXPECT_FALSE(File::Exist(sourcePath + ".complete"));
    DBRunner runner(DB_PATH);
    std::vector<std::tuple<uint64_t>> count;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM CCUTaskInfo", count));
    ASSERT_EQ(1U, count.size());
    EXPECT_EQ(1U, std::get<0>(count[0]));
}

TEST_F(CcuAddInfoDBDumperUTest, TestCompletedSourceWithoutTableShouldFail)
{
    CcuInfoData data;
    data.taskSourceCompleted = true;
    CcuAddInfoDBDumper dumper(HOST_PATH);

    EXPECT_FALSE(dumper.DumpData(data));
    DBRunner runner(DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("CCUTaskInfo"));
}

TEST_F(CcuAddInfoDBDumperUTest, TestCompletedSourcePreflightShouldRunBeforeWrites)
{
    CcuInfoData data;
    data.taskRecords.push_back({0, 1, "10", "20", 0, 2, 1, 7, 1, 2, 3});
    data.groupSourceCompleted = true;
    CcuAddInfoDBDumper dumper(HOST_PATH);

    EXPECT_FALSE(dumper.DumpData(data));
    DBRunner runner(DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("CCUTaskInfo"));
}

TEST_F(CcuAddInfoDBDumperUTest, TestIntegerOverflowPreflightShouldRunBeforeWrites)
{
    CcuInfoData data;
    data.taskRecords.push_back({0, 1, "10", "20", 0, 2, 1, 7, 1, 2, 3});
    data.groupRecords.push_back({0, "12", "20", 0, 2, 1, 1, 9, 1, 119, 2, "SUM", "INT32", "INT32",
                                 static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1, 4, 5});
    CcuAddInfoDBDumper dumper(HOST_PATH);

    EXPECT_FALSE(dumper.DumpData(data));
    DBRunner runner(DB_PATH);
    EXPECT_FALSE(runner.CheckTableExists("CCUTaskInfo"));
    EXPECT_FALSE(runner.CheckTableExists("CCUGroupInfo"));
}
