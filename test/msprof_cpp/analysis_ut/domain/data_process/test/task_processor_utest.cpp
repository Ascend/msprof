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
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string TASK_PATH = "./task_path";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "ascend_task.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string TABLE_NAME = "AscendTask";

using DbDataType = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double, double,
        std::string, std::string, uint32_t>>;

DbDataType DATA_A{{4294967295, -1, 37, 1, 3, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "MIX_AIC", 1},
                  {4294967295, -1, 37, 2, 5, 0, 8719911184665.1, 680.013671875, "FFTS_PLUS", "Write Value", 2},
                  {4294967295, -1, 37, 3, 5, 0, 8719911182265.1, 680.013671875, "UNKNOWN", "11", 3},
                  {4294967295, -1, 37, 4, 5, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "88", 4},
                  {4294967295, -1, 37, 5, 7, 0, 8719911184965.1, 680.013671875, "KERNEL_AICORE", "AI_CORE", 5}};
}

class TaskProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(TASK_PATH)) {
            File::RemoveDir(TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateAscendTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(TASK_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
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

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    auto processor = TaskProcessor(PROF_PATH_A);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenCheckPathNotExists)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&Analysis::Utils::File::Exist)
    .stubs()
    .will(returnValue(false));
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
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenGetSyscntConversionParamsFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetSyscntConversionParams).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTurnData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTurnData>).reset();
}
