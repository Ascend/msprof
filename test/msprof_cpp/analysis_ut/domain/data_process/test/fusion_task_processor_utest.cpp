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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/domain/data_process/ai_task/fusion_task_processor.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/application/database/db_constant.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Application;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ft_processor_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string DEVICE_SQLITE_PATH = File::PathJoin({DEVICE_PATH, SQLITE});
const std::string FT_DB_PATH = File::PathJoin({DEVICE_SQLITE_PATH, "fusion_task.db"});
const std::string FT_TABLE = "FusionTask";

// OriFusionTaskFormat: (stream_id, task_id, acc_id, task_type, start_time, end_time, task_time,
//                        fusion_task_type, mission_id, ccu_die_id)
// DB values are in nanoseconds (Python time_from_syscnt), processor passes through + GetLocalTime
using FusionSeedData = OriFusionTaskFormat;

FusionSeedData BuildSeedData()
{
    return {
        {0, 100, 3, "AI_CORE", 1000.0, 2000.0, 1000.0, "AICORE", 5, 0},
        {0, 200, 7, "CCU_SQE", 3000.0, 4000.0, 1000.0, "CCU", 3, 0},
    };
}

nlohmann::json BuildContextRecord()
{
    return {
        {"startCollectionTimeBegin", "0"},
        {"endCollectionTimeEnd", "999999"},
        {"startClockMonotonicRaw", "0"},
        {"hostCntvct", "0"},
        {"devCntvct", "0"},
        {"CPU", {{{"Frequency", "1000.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "1000.000000"}}}},
        {"hostMonotonic", "0"},
        {"hostCntvctDiff", "0"}
    };
}
}

class FusionTaskProcessorUTest : public testing::Test {
protected:
    void SetUp() override
    {
        RebuildBaseDir();
        CreateFusionTaskDb(BuildSeedData(), true);
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(BuildContextRecord()));
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
        MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
        MOCKER_CPP(&DataProcessor::SaveToDataInventory<FusionTaskTimelineData>).reset();
        MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
        if (File::Check(BASE_PATH)) {
            EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        }
    }

    static void RebuildBaseDir()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_SQLITE_PATH));
    }

    static void CreateFusionTaskDb(const FusionSeedData &seedData, bool createTable)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, FT_DB_PATH);
        if (createTable) {
            std::vector<TableColumn> cols {
                {"stream_id", SQL_INTEGER_TYPE}, {"task_id", SQL_INTEGER_TYPE},
                {"acc_id", SQL_INTEGER_TYPE}, {"task_type", SQL_TEXT_TYPE},
                {"start_time", SQL_REAL_TYPE}, {"end_time", SQL_REAL_TYPE},
                {"task_time", SQL_REAL_TYPE}, {"fusion_task_type", SQL_TEXT_TYPE},
                {"mission_id", SQL_INTEGER_TYPE}, {"ccu_die_id", SQL_INTEGER_TYPE},
            };
            EXPECT_TRUE(dbRunner->CreateTable(FT_TABLE, cols));
            if (!seedData.empty()) {
                EXPECT_TRUE(dbRunner->InsertData(FT_TABLE, seedData));
            }
        }
    }
};

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));

    auto ftData = dataInventory.GetPtr<std::vector<FusionTaskTimelineData>>();
    ASSERT_NE(nullptr, ftData);
    ASSERT_EQ(2ul, ftData->size());

    // DB stores absolute ns (Python time_from_syscnt) → GetLocalTime → ns (zero offset in test)
    const auto &aicoreData = (*ftData)[0];
    EXPECT_EQ(0u, aicoreData.streamId);
    EXPECT_EQ(100u, aicoreData.taskId);
    EXPECT_EQ(3u, aicoreData.accId);
    EXPECT_EQ("AI_CORE", aicoreData.taskType);
    EXPECT_EQ("AICORE", aicoreData.fusionTaskType);
    EXPECT_EQ(1000ul, aicoreData.startTime);   // 1000ns
    EXPECT_EQ(2000ul, aicoreData.endTime);      // 2000ns
    EXPECT_EQ(1000.0, aicoreData.duration);     // 1000ns
    EXPECT_FALSE(aicoreData.isCcu);
    EXPECT_FALSE(aicoreData.hasCcuDie);

    // Second row: CCU task
    const auto &ccuData = (*ftData)[1];
    EXPECT_EQ(200u, ccuData.taskId);
    EXPECT_EQ("CCU", ccuData.fusionTaskType);
    EXPECT_TRUE(ccuData.isCcu);
    EXPECT_EQ(3u, ccuData.missionId);
    EXPECT_EQ(3000ul, ccuData.startTime);
    EXPECT_EQ(4000ul, ccuData.endTime);
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnTrueWhenTableEmpty)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, FT_DB_PATH);
    EXPECT_TRUE(dbRunner->DeleteData("DELETE FROM " + FT_TABLE));

    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
    auto ftData = dataInventory.GetPtr<std::vector<FusionTaskTimelineData>>();
    EXPECT_EQ(nullptr, ftData);
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnTrueWhenTableNotExist)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, FT_DB_PATH);
    EXPECT_TRUE(dbRunner->DropTable(FT_TABLE));

    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
    auto ftData = dataInventory.GetPtr<std::vector<FusionTaskTimelineData>>();
    EXPECT_EQ(nullptr, ftData);
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnFalseWhenSaveDataFailed)
{
    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<FusionTaskTimelineData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
}

TEST_F(FusionTaskProcessorUTest, TestRunShouldReturnFalseWhenNoDeviceDir)
{
    if (File::Check(DEVICE_PATH)) {
        File::RemoveDir(DEVICE_PATH, DEPTH);
    }
    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));
}

TEST_F(FusionTaskProcessorUTest, TestFormatDataWithZeroDuration)
{
    FusionSeedData seedWithZeroDur = {{0, 100, 3, "AI_CORE", 100.0, 100.0, 0.0, "AICORE", 0, 0}};
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, FT_DB_PATH);
    EXPECT_TRUE(dbRunner->DeleteData("DELETE FROM " + FT_TABLE));
    EXPECT_TRUE(dbRunner->InsertData(FT_TABLE, seedWithZeroDur));

    DataInventory dataInventory;
    FusionTaskProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_TASK));

    auto ftData = dataInventory.GetPtr<std::vector<FusionTaskTimelineData>>();
    ASSERT_NE(nullptr, ftData);
    ASSERT_EQ(1ul, ftData->size());
    EXPECT_EQ(0.0, (*ftData)[0].duration);
}
