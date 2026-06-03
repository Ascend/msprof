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

#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ccu_processor_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string DEVICE_SQLITE_PATH = File::PathJoin({DEVICE_PATH, SQLITE});
const std::string HOST_SQLITE_PATH = File::PathJoin({PROF_PATH, HOST, SQLITE});
const std::string CCU_DB_PATH = File::PathJoin({DEVICE_SQLITE_PATH, "ccu.db"});
const std::string CCU_ADD_INFO_DB_PATH = File::PathJoin({HOST_SQLITE_PATH, "ccu_add_info.db"});

const std::string MISSION_TABLE = "OriginMission";
const std::string CHANNEL_TABLE = "OriginChannel";
const std::string WAIT_TABLE = "CCUWaitSignalInfo";
const std::string GROUP_TABLE = "CCUGroupInfo";

using MissionSeedData = std::vector<std::tuple<uint16_t, uint32_t, uint16_t, uint16_t, uint32_t, uint64_t, uint64_t,
    uint64_t, uint64_t>>;
using ChannelSeedData = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
using WaitSeedData = std::vector<std::tuple<uint16_t, uint32_t, uint16_t, uint16_t, uint32_t, uint16_t>>;
using GroupSeedData = std::vector<std::tuple<uint16_t, uint32_t, uint16_t, uint16_t, std::string, std::string,
    std::string, uint64_t>>;

MissionSeedData BuildMissionData()
{
    return {
        {1, 10, 100, 200, 5, 0, 1000, 0, 4000},
        {1, 10, 100, 200, 6, 2000, 0, 6000, 0},
        {1, 10, 100, 200, 9, 2500, 0, 8000, 0}
    };
}

ChannelSeedData BuildChannelData()
{
    return {
        {3, 3000, 0, 0, 88},
        {4, 7500, 0, 0, 120},
        {5, 9000, 0, 0, 999}
    };
}

WaitSeedData BuildWaitData()
{
    return {
        {1, 10, 200, 0, 255, 3},
        {1, 10, 200, 0, 255, 4}
    };
}

GroupSeedData BuildGroupData()
{
    return {
        {1, 10, 100, 0, "SUM", "FP16", "FP16", 1024}
    };
}

nlohmann::json BuildContextRecord()
{
    return {
        // Use a tiny, self-consistent time base so fake syscnt data (1000~8000) won't be filtered out.
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

const CCUMissionTimelineData *FindByType(const std::vector<CCUMissionTimelineData> &datas, const std::string &type)
{
    for (const auto &data : datas) {
        if (data.timeType == type) {
            return &data;
        }
    }
    return nullptr;
}
}

class CcuMissionProcessorUTest : public testing::Test {
protected:
    void SetUp() override
    {
        RebuildBaseDir();
        CreateCcuDb(BuildMissionData(), BuildChannelData(), true, true);
        CreateCcuAddInfoDb(BuildWaitData(), BuildGroupData(), true, true);
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(BuildContextRecord()));
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
        MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
        MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
        MOCKER_CPP(&DataProcessor::SaveToDataInventory<CCUMissionTimelineData>).reset();
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
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(HOST_SQLITE_PATH));
    }

    static void CreateCcuDb(const MissionSeedData &missionData, const ChannelSeedData &channelData,
                            bool createMissionTable, bool createChannelTable)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_DB_PATH);
        if (createMissionTable) {
            std::vector<TableColumn> missionCols {
                {"stream_id", SQL_INTEGER_TYPE}, {"task_id", SQL_INTEGER_TYPE}, {"lp_instr_id", SQL_INTEGER_TYPE},
                {"setckebit_instr_id", SQL_INTEGER_TYPE}, {"rel_id", SQL_INTEGER_TYPE},
                {"setckebit_start_time", SQL_INTEGER_TYPE}, {"lp_start_time", SQL_INTEGER_TYPE},
                {"rel_end_time", SQL_INTEGER_TYPE}, {"lp_end_time", SQL_INTEGER_TYPE}
            };
            EXPECT_TRUE(dbRunner->CreateTable(MISSION_TABLE, missionCols));
            if (!missionData.empty()) {
                EXPECT_TRUE(dbRunner->InsertData(MISSION_TABLE, missionData));
            }
        }
        if (createChannelTable) {
            std::vector<TableColumn> channelCols {
                {"channel_id", SQL_INTEGER_TYPE}, {"timestamp", SQL_INTEGER_TYPE}, {"max_bw", SQL_INTEGER_TYPE},
                {"min_bw", SQL_INTEGER_TYPE}, {"avg_bw", SQL_INTEGER_TYPE}
            };
            EXPECT_TRUE(dbRunner->CreateTable(CHANNEL_TABLE, channelCols));
            if (!channelData.empty()) {
                EXPECT_TRUE(dbRunner->InsertData(CHANNEL_TABLE, channelData));
            }
        }
    }

    static void CreateCcuAddInfoDb(const WaitSeedData &waitData, const GroupSeedData &groupData,
                                   bool createWaitTable, bool createGroupTable)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_ADD_INFO_DB_PATH);
        if (createWaitTable) {
            std::vector<TableColumn> waitCols {
                {"stream_id", SQL_INTEGER_TYPE}, {"task_id", SQL_INTEGER_TYPE}, {"instr_id", SQL_INTEGER_TYPE},
                {"die_id", SQL_INTEGER_TYPE}, {"mask", SQL_INTEGER_TYPE}, {"channel_id", SQL_INTEGER_TYPE}
            };
            EXPECT_TRUE(dbRunner->CreateTable(WAIT_TABLE, waitCols));
            if (!waitData.empty()) {
                EXPECT_TRUE(dbRunner->InsertData(WAIT_TABLE, waitData));
            }
        }
        if (createGroupTable) {
            std::vector<TableColumn> groupCols {
                {"stream_id", SQL_INTEGER_TYPE}, {"task_id", SQL_INTEGER_TYPE}, {"instr_id", SQL_INTEGER_TYPE},
                {"die_id", SQL_INTEGER_TYPE}, {"reduce_op_type", SQL_TEXT_TYPE}, {"input_data_type", SQL_TEXT_TYPE},
                {"output_data_type", SQL_TEXT_TYPE}, {"data_size", SQL_INTEGER_TYPE}
            };
            EXPECT_TRUE(dbRunner->CreateTable(GROUP_TABLE, groupCols));
            if (!groupData.empty()) {
                EXPECT_TRUE(dbRunner->InsertData(GROUP_TABLE, groupData));
            }
        }
    }
};

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));

    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    ASSERT_NE(nullptr, ccuData);
    ASSERT_EQ(2ul, ccuData->size());

    const auto *loopData = FindByType(*ccuData, CCU_TIME_TYPE_LOOP_GROUP);
    ASSERT_NE(nullptr, loopData);
    EXPECT_EQ(100, loopData->instructionId);
    EXPECT_TRUE(loopData->hasDataSize);
    EXPECT_EQ(1024u, loopData->dataSize);
    EXPECT_TRUE(loopData->hasReduceInfo);
    EXPECT_EQ("SUM", loopData->reduceOpType);

    const auto *waitData = FindByType(*ccuData, CCU_TIME_TYPE_WAIT);
    ASSERT_NE(nullptr, waitData);
    EXPECT_EQ(200, waitData->instructionId);
    EXPECT_EQ(9u, waitData->notifyRankId);
    EXPECT_TRUE(waitData->hasMask);
    EXPECT_EQ(255u, waitData->mask);
    EXPECT_TRUE(waitData->hasDelayChannel);
    EXPECT_EQ(4, waitData->maxDelayChannel);
    EXPECT_EQ(120u, waitData->maxChannelDelay);
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnFalseWhenGetSyscntConversionParamsFailed)
{
    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnTrueWhenMissionTableNotExist)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_DB_PATH);
    EXPECT_TRUE(dbRunner->DropTable(MISSION_TABLE));

    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));
    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    EXPECT_EQ(nullptr, ccuData);
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnTrueWhenOriginChannelTableNotExist)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_DB_PATH);
    EXPECT_TRUE(dbRunner->DropTable(CHANNEL_TABLE));

    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));

    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    ASSERT_NE(nullptr, ccuData);
    const auto *waitData = FindByType(*ccuData, CCU_TIME_TYPE_WAIT);
    ASSERT_NE(nullptr, waitData);
    EXPECT_FALSE(waitData->hasDelayChannel);
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnTrueWhenLoopGroupWithoutGroupInfo)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_ADD_INFO_DB_PATH);
    EXPECT_TRUE(dbRunner->DropTable(GROUP_TABLE));

    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));

    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    ASSERT_NE(nullptr, ccuData);
    const auto *loopData = FindByType(*ccuData, CCU_TIME_TYPE_LOOP_GROUP);
    ASSERT_NE(nullptr, loopData);
    EXPECT_FALSE(loopData->hasDataSize);
    EXPECT_FALSE(loopData->hasReduceInfo);
    EXPECT_FALSE(loopData->hasBandwidth);
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnTrueWhenLoopGroupWithReservedReduceOpType)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, CCU_ADD_INFO_DB_PATH);
    GroupSeedData reservedGroup {{1, 10, 100, 0, "RESERVED", "FP16", "FP16", 1024}};
    EXPECT_TRUE(dbRunner->DeleteData("DELETE FROM " + GROUP_TABLE));
    EXPECT_TRUE(dbRunner->InsertData(GROUP_TABLE, reservedGroup));

    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));

    auto ccuData = dataInventory.GetPtr<std::vector<CCUMissionTimelineData>>();
    ASSERT_NE(nullptr, ccuData);
    const auto *loopData = FindByType(*ccuData, CCU_TIME_TYPE_LOOP_GROUP);
    ASSERT_NE(nullptr, loopData);
    EXPECT_TRUE(loopData->hasDataSize);
    EXPECT_FALSE(loopData->hasReduceInfo);
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnFalseWhenSaveDataFailed)
{
    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CCUMissionTimelineData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));
}

TEST_F(CcuMissionProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    DataInventory dataInventory;
    CCUMissionProcessor processor(PROF_PATH);
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION));
}
