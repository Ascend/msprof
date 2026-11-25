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
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "reserve_mock_utils.h"
#include <nlohmann/json.hpp>

using namespace Analysis::Domain;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
using Analysis::Infra::Database;
using Analysis::Infra::SQL_NUMERIC_TYPE;
using Analysis::Infra::SQL_TEXT_TYPE;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ccu_mission_v62_info";
const std::string DEVICE_STR = File::PathJoin({BASE_PATH, "PROF_0", DEVICE_PREFIX + "0"});
const std::string SQLITE_PATH = File::PathJoin({DEVICE_STR, SQLITE});
const std::string DB_PATH = File::PathJoin({SQLITE_PATH, "ccu.db"});
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string LOOPGROUP_TABLE = "OriginLoopGroupV62";
const std::string SETCKB_TABLE = "OriginSetCKBV62";

// Sample LoopGroup: 2 records
const OriCCULoopGroupV62Data LOOPGROUP_DATA{
    {1, 100, 1000000, 2000000, 0},
    {2, 200, 3000000, 5000000, 0},
};

// Sample SetCKB: 1 mission record (type-1, zero-duration) + 1 loop record (type-0/2)
const OriCCUSetCKBV62Data SETCKB_DATA{
    // mission: instr_id=10, syscnt_type=1 (immediate), no udf
    {3, 10, 0, 0, 0xFFULL, 1000000, {}, 0, 1, 0, 0},
    // loop: instr_id=20 loop_id=1 loop_cnt=0, syscnt_type=0 (start)
    {4, 20, 1, 0, 0, 2000000, {}, 0, 0, 1, 0},
    // loop: instr_id=20 loop_id=1 loop_cnt=0, syscnt_type=2 (end), with udf
    {4, 20, 1, 0, 0, 6000000, "udf_val", 1, 2, 1, 0},
};

// Minimal DB class for CCU v6_2 tables used in unit tests
class CCUV62TestDB : public Database {
public:
    CCUV62TestDB()
    {
        dbName_ = "ccu.db";
        tableColNames_[LOOPGROUP_TABLE] = {
            {"task_id", SQL_NUMERIC_TYPE},
            {"loopgroup_instr_id", SQL_NUMERIC_TYPE},
            {"loopgroup_start_syscnt", SQL_NUMERIC_TYPE},
            {"loopgroup_stop_syscnt", SQL_NUMERIC_TYPE},
            {"die_id", SQL_NUMERIC_TYPE},
        };
        tableColNames_[SETCKB_TABLE] = {
            {"task_id", SQL_NUMERIC_TYPE},
            {"setckb_instr_id", SQL_NUMERIC_TYPE},
            {"loop_id", SQL_NUMERIC_TYPE},
            {"loop_cnt", SQL_NUMERIC_TYPE},
            {"setckb_ckbinfo", SQL_TEXT_TYPE},
            {"setckb_syscnt", SQL_NUMERIC_TYPE},
            {"udf_profiling_data", SQL_TEXT_TYPE},
            {"udf_flag", SQL_NUMERIC_TYPE},
            {"syscnt_type", SQL_NUMERIC_TYPE},
            {"lp_flg", SQL_NUMERIC_TYPE},
            {"die_id", SQL_NUMERIC_TYPE},
        };
    }
};

nlohmann::json MakeDeviceInfo()
{
    return {
        {"startCollectionTimeBegin", "0"},
        {"endCollectionTimeEnd", "999999999"},
        {"startClockMonotonicRaw", "0"},
        {"hostCntvct", "0"},
        {"devCntvct", "0"},
        {"CPU", {{{"Frequency", "1000.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "1000.000000"}}}},
        {"hostMonotonic", "0"},
        {"hostCntvctDiff", "0"}
    };
}
} // anonymous namespace

class CCUMissionV62ProcessorUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({BASE_PATH, "PROF_0"})));
        EXPECT_TRUE(File::CreateDir(DEVICE_STR));
        EXPECT_TRUE(File::CreateDir(SQLITE_PATH));

        std::shared_ptr<CCUV62TestDB> database;
        MAKE_SHARED0_RETURN_VOID(database, CCUV62TestDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, DB_PATH);

        auto loopCols = database->GetTableCols(LOOPGROUP_TABLE);
        dbRunner->CreateTable(LOOPGROUP_TABLE, loopCols);
        dbRunner->InsertData(LOOPGROUP_TABLE, LOOPGROUP_DATA);

        auto setckbCols = database->GetTableCols(SETCKB_TABLE);
        dbRunner->CreateTable(SETCKB_TABLE, setckbCols);
        dbRunner->InsertData(SETCKB_TABLE, SETCKB_DATA);
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        if (File::Check(BASE_PATH)) {
            EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        }
    }
};

TEST_F(CCUMissionV62ProcessorUTest, ShouldReturnTrueAndProduceTimelineDataWhenProcessSuccess)
{
    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));

    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));

    auto res = dataInventory.GetPtr<std::vector<CCUV62TimelineData>>();
    ASSERT_NE(nullptr, res);
    // 2 LoopGroup + 1 mission SetCKB + 1 loop SetCKB = 4 events
    EXPECT_EQ(4ul, res->size());

    // LoopGroup events: timeType = "LoopGroup"
    auto loopgroupEvents = 0;
    auto setckbEvents = 0;
    for (const auto &d : *res) {
        if (d.timeType == CCU_TIME_TYPE_LOOP_GROUP) {
            loopgroupEvents++;
        } else if (d.timeType == CCU_TIME_TYPE_SETCKB) {
            setckbEvents++;
        }
    }
    EXPECT_EQ(2, loopgroupEvents);
    EXPECT_EQ(2, setckbEvents);
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldMatchHostInstructionsAndUseAverageDelayFromTheSameDie)
{
    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));

    const auto hostPath = File::PathJoin({PROF_PATH, "host"});
    const auto hostSqlite = File::PathJoin({hostPath, SQLITE});
    ASSERT_TRUE(File::CreateDir(hostPath));
    ASSERT_TRUE(File::CreateDir(hostSqlite));
    DBRunner hostDB(File::PathJoin({hostSqlite, "ccu_add_info.db"}));
    ASSERT_TRUE(hostDB.CreateTable("CCUGroupInfo", {
        {"stream_id", SQL_NUMERIC_TYPE}, {"task_id", SQL_NUMERIC_TYPE}, {"instr_id", SQL_NUMERIC_TYPE},
        {"die_id", SQL_NUMERIC_TYPE}, {"reduce_op_type", SQL_TEXT_TYPE}, {"input_data_type", SQL_TEXT_TYPE},
        {"output_data_type", SQL_TEXT_TYPE}, {"data_size", SQL_NUMERIC_TYPE},
    }));
    const OriCCUGroupData groups{
        {99, 1, 100, 0, "SUM", "FP32", "FP32", 4096},
        {99, 1, 100, 0, "SUM", "FP32", "FP32", 4096},
        {99, 2, 200, 0, "SUM", "FP32", "FP32", 1},
        {99, 2, 200, 0, "SUM", "FP32", "FP32", 2},
    };
    ASSERT_TRUE(hostDB.InsertData("CCUGroupInfo", groups));
    ASSERT_TRUE(hostDB.CreateTable("CCUWaitSignalInfo", {
        {"stream_id", SQL_NUMERIC_TYPE}, {"task_id", SQL_NUMERIC_TYPE}, {"instr_id", SQL_NUMERIC_TYPE},
        {"die_id", SQL_NUMERIC_TYPE}, {"mask", SQL_NUMERIC_TYPE}, {"channel_id", SQL_NUMERIC_TYPE},
    }));
    ASSERT_TRUE(hostDB.InsertData("CCUWaitSignalInfo", OriCCUWaitSignalData{
        {99, 3, 10, 0, 0, 17}, {99, 4, 20, 0, 255, 17}, {99, 4, 20, 0, 255, 21},
        {98, 4, 20, 1, 7, 17},
    }));

    DBRunner deviceDB(DB_PATH);
    ASSERT_TRUE(deviceDB.CreateTable("OriginChannelV62", {
        {"channel_id", SQL_NUMERIC_TYPE}, {"timestamp", SQL_TEXT_TYPE}, {"max_delay", SQL_NUMERIC_TYPE},
        {"avg_delay", SQL_NUMERIC_TYPE}, {"die_id", SQL_NUMERIC_TYPE},
    }));
    const std::vector<std::tuple<uint16_t, std::string, uint64_t, uint64_t, uint16_t>> channels{
        {17, "500000", 50, 10, 0},
        {21, "5000000", 500, 10, 0},
        {17, "6000000", 50, 100, 0},
        {17, "5000000", 50, 1000, 1},
    };
    ASSERT_TRUE(deviceDB.InsertData("OriginChannelV62", channels));
    ASSERT_TRUE(deviceDB.InsertData(SETCKB_TABLE, OriCCUSetCKBV62Data{
        {4, 20, 9, 2, 0, 4000000, {}, 0, 0, 1, 1},
        {4, 20, 9, 2, 0, 7000000, {}, 0, 2, 1, 1},
        {4, 20, 8, 1, 0, 4000000, {}, 0, 0, 1, 1},
        {4, 20, 8, 1, 0, 7000000, {}, 0, 2, 1, 1},
        {5, 21, 0, 0, 0, 2000000, {}, 0, 0, 0, 0},
        {5, 21, 0, 0, 0, 6000000, {}, 0, 2, 0, 1},
    }));

    DataInventory inventory;
    CCUMissionV62Processor processor(PROF_PATH);
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_CCU_MISSION_V62));
    auto events = inventory.GetPtr<std::vector<CCUV62TimelineData>>();
    ASSERT_NE(events, nullptr);
    ASSERT_EQ(events->size(), 5u);
    EXPECT_EQ((*events)[0].dataSize, 4096u);
    EXPECT_TRUE((*events)[0].hasBandwidth);
    EXPECT_EQ((*events)[0].reduceOpType, "SUM");
    EXPECT_FALSE((*events)[1].hasDataSize);
    EXPECT_TRUE((*events)[2].hasMask);
    EXPECT_EQ((*events)[2].mask, 0u);
    EXPECT_FALSE((*events)[2].hasDelayChannel);
    EXPECT_EQ((*events)[3].mask, 255u);
    EXPECT_TRUE((*events)[3].hasDelayChannel);
    EXPECT_EQ((*events)[3].maxDelayChannel, 17u);
    EXPECT_EQ((*events)[3].maxChannelDelay, 10u);
    EXPECT_EQ((*events)[4].dieId, 1u);
    EXPECT_EQ((*events)[4].mask, 7u);
    EXPECT_EQ((*events)[4].maxChannelDelay, 1000u);
    EXPECT_EQ((*events)[4].earliestLoopId, 8u);
    EXPECT_EQ((*events)[4].latestLoopId, 8u);
    EXPECT_EQ((*events)[4].longestLoopId, 8u);
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldReturnTrueWhenV62TablesNotExist)
{
    // Drop both tables before run
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, DB_PATH);
        dbRunner->DropTable(LOOPGROUP_TABLE);
        dbRunner->DropTable(SETCKB_TABLE);
    }

    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));

    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));
    auto res = dataInventory.GetPtr<std::vector<CCUV62TimelineData>>();
    EXPECT_EQ(nullptr, res);
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldKeepSetCKBDataWhenLoopGroupReserveFails)
{
    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));
    Analysis::Test::StubReserveFailureForVector<std::vector<CCUV62TimelineData>>();

    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));
    auto res = dataInventory.GetPtr<std::vector<CCUV62TimelineData>>();
    ASSERT_NE(nullptr, res);
    EXPECT_EQ(2ul, res->size());
    Analysis::Test::ResetReserveFailureForVector<std::vector<CCUV62TimelineData>>();
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldReturnTrueWhenSaveToDataInventorySetNull)
{
    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CCUV62TimelineData>)
        .stubs().will(returnValue(false));

    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldReturnFalseWhenConstructDBRunnerFailed)
{
    auto deviceInfo = MakeDeviceInfo();
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(deviceInfo));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));

    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));
}

TEST_F(CCUMissionV62ProcessorUTest, ShouldReturnTrueWhenNoDevicePath)
{
    // PROF_PATH has no device_* subdirectory
    EXPECT_TRUE(File::RemoveDir(DEVICE_STR, DEPTH));
    DataInventory dataInventory;
    auto processor = CCUMissionV62Processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CCU_MISSION_V62));
}
