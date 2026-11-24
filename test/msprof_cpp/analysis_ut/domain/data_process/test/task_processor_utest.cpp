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
#include <algorithm>
#include <set>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/domain/data_process/system/block_detail_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "reserve_mock_utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace
{
const int DEPTH = 0;
const std::string TASK_PATH = "./task_path";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "ascend_task.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLK2"});
const std::string TABLE_NAME = "AscendTask";

using DbDataType = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double, double,
        std::string, std::string, uint32_t, double>>;

DbDataType DATA_A{{4294967295, -1, 37, 1, 3, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "MIX_AIC", 1, 0},
                  {4294967295, -1, 37, 2, 5, 0, 8719911184665.1, 680.013671875, "FFTS_PLUS", "Write Value", 2, 0},
                  {4294967295, -1, 37, 3, 5, 0, 8719911182265.1, 680.013671875, "UNKNOWN", "11", 3, 125},
                  {4294967295, -1, 37, 4, 5, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "88", 4, 281},
                  {4294967295, -1, 37, 5, 7, 0, 8719911184965.1, 680.013671875, "KERNEL_AICORE", "AI_CORE", 5, 378}};
// device_task_type 全为 UNKNOWN，会被 LoadData 中 "device_task_type != 'UNKNOWN'" 的条件过滤掉
DbDataType DATA_B{{4294967295, -1, 37, 1, 3, 0, 8719911184665.1, 680.013671875, "KERNEL_AICORE", "UNKNOWN", 1, 0},
                  {4294967295, -1, 37, 2, 5, 0, 8719911182265.1, 680.013671875, "FFTS_PLUS", "UNKNOWN", 2, 0}};
}  // namespace

class TaskProcessorUTest : public testing::Test
{
   protected:
    static void SetUpTestCase()
    {
        if (File::Check(TASK_PATH))
        {
            File::RemoveDir(TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateAscendTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateAscendTask(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_B);
    }
    static void TearDownTestCase() { EXPECT_TRUE(File::RemoveDir(TASK_PATH, DEPTH)); }
    virtual void TearDown() { GlobalMockObject::verify(); }
    static void CreateAscendTask(const std::string& dbPath, DbDataType& data)
    {
        std::shared_ptr<AscendTaskDB> database;
        MAKE_SHARED0_RETURN_VOID(database, AscendTaskDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(TaskProcessorUTest, ShouldSelectSqeTypesByChip)
{
    TaskProcessor processor(PROF_PATH_A);
    using Analysis::Domain::Environment::Chip;
    const std::map<std::string, std::string> expected{
        {"1", "AIV_SQE"}, {"2", "FUSION"}, {"4", "AICPU_H"}, {"5", "AICPU_D"},
        {"9", "UBDMA"}, {"10", "ASYNCDMA"}, {"15", "CMO_SQE"}, {"16", "CCU_SQE"},
        {"20", "CONDITION_SQE"}, {"21", "END_SQE"}};
    for (const auto chip : {Chip::CHIP_V6_1_0, Chip::CHIP_V6_1_1, Chip::CHIP_V6_2_0}) {
        const auto version = static_cast<uint16_t>(chip);
        for (const auto& type : expected) {
            EXPECT_EQ(type.second, processor.GetTaskType("UNKNOWN", type.first, version));
        }
        EXPECT_EQ("Other", processor.GetTaskType("UNKNOWN", "17", version));
        EXPECT_EQ("KERNEL_AICORE", processor.GetTaskType("KERNEL_AICORE", "1", version));
        EXPECT_EQ("Write Value", processor.GetTaskType("FFTS_PLUS", "Write Value", version));
    }
    EXPECT_EQ("AI_CPU", processor.GetTaskType("UNKNOWN", "1", static_cast<uint16_t>(Chip::CHIP_V4_1_0)));
    EXPECT_EQ("SDMA_SQE", processor.GetTaskType("UNKNOWN", "9", static_cast<uint16_t>(Chip::CHIP_V3_1_0)));
}

TEST_F(TaskProcessorUTest, ShouldReadLegacyTaskTableWithoutWriteLatency)
{
    auto path = File::PathJoin({TASK_PATH, "legacy_task.db"});
    DBInfo info("legacy_task.db", "AscendTask");
    ASSERT_TRUE(info.ConstructDBRunner(path));
    AscendTaskDB schema;
    auto columns = schema.GetTableCols("AscendTask");
    columns.erase(std::remove_if(columns.begin(), columns.end(),
                                [](const Analysis::Infra::TableColumn &c) { return c.name == "write_latency"; }),
                  columns.end());
    ASSERT_TRUE(info.dbRunner->CreateTable("AscendTask", columns));
    using LegacyRows = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t,
                                             double, double, std::string, std::string, uint32_t>>;
    LegacyRows input{{0, -1, 37, 1, 3, 0, 1000, 200, "KERNEL_AICORE", "AI_CORE", 1}};
    ASSERT_TRUE(info.dbRunner->InsertData("AscendTask", input));
    const auto rows = TaskProcessor(PROF_PATH_A).LoadData(info, path);
    ASSERT_EQ(1ul, rows.size());
    EXPECT_DOUBLE_EQ(1000, std::get<0>(rows.front()));
    EXPECT_DOUBLE_EQ(0, std::get<11>(rows.front()));
}

TEST_F(TaskProcessorUTest, ShouldReadLegacyPmuTableWithoutFakeEndTime)
{
    auto path = File::PathJoin({TASK_PATH, "legacy_pmu.db"});
    DBInfo info("legacy_pmu.db", "V6BlockPmu");
    ASSERT_TRUE(info.ConstructDBRunner(path));
    std::vector<Analysis::Infra::TableColumn> columns;
    for (const auto &name : {"stream_id", "task_id", "subtask_id", "batch_id", "start_time", "duration",
                             "core_type", "core_id"})
    {
        columns.emplace_back(name, "INTEGER");
    }
    ASSERT_TRUE(info.dbRunner->CreateTable(info.tableName, columns));
    std::vector<std::tuple<int, int, int, int, int, int, int, int>> input{{7, 42, 5, 3, 1000, 200, 0, 1}};
    ASSERT_TRUE(info.dbRunner->InsertData(info.tableName, input));
    const auto rows = BlockDetailProcessor(PROF_PATH_A).LoadData(path, info);
    ASSERT_EQ(1ul, rows.size());
    EXPECT_EQ(1000ul, std::get<4>(rows.front()));
    EXPECT_EQ(0ul, std::get<8>(rows.front()));
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenOriginalDataAllFilteredByLoadData)
{
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    auto processor = TaskProcessor(PROF_PATH_B);
    // LoadData 带有 device_task_type != 'UNKNOWN' 的过滤条件，过滤后为空属于正常场景，不应中断分析
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AscendTaskData>>());
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    auto processor = TaskProcessor(PROF_PATH_A);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenCheckPathNotExists)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Analysis::Utils::File::Exist).stubs().will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Utils::File::Exist).reset();
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    Analysis::Test::StubReserveFailureForVector<std::vector<AscendTaskData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    Analysis::Test::ResetReserveFailureForVector<std::vector<AscendTaskData>>();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenGetSyscntConversionParamsFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTurnData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTurnData>).reset();
}
