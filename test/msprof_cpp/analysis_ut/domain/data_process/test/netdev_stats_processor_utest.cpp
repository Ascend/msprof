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
#include "analysis/csrc/domain/data_process/system/netdev_stats_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace {
const std::string NETDEV_STATS_DIR = "./netdev_stats";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "netdev_stats.db";
const std::string PROF_DIR = File::PathJoin({NETDEV_STATS_DIR, "./PROF_0"});
const std::string TABLE_NAME = "NetDevStatsOriginalData";
const std::vector<OriNetDevStatsData> NETDEV_STATS_DATA = {
    {980786977706530, 0, 0, 17796736595950, 17797559020889, 0, 0,
     274242073, 274303721, 0, 0, 0, 0, 1, 204259156, 973908035},
    {980786998329570, 0, 0, 17796882062126, 17797704121835, 0, 0,
     274277663, 274339400, 0, 0, 0, 0, 1, 204259156, 973908095},
    {980787018980370, 0, 0, 17797020908040, 17797843358981, 0, 0,
     274313612, 274375272, 0, 0, 0, 0, 1, 204259156, 973908155},
    {980787039603220, 0, 0, 17797182596918, 17798005028585, 0, 0,
     274331142, 274392820, 0, 0, 0, 0, 1, 204259156, 973908215},
    {980787060338640, 0, 0, 17797351380400, 17798173809359, 0, 0,
     274396608, 274458312, 0, 0, 0, 0, 1, 204259156, 973908215}
};
}
class NetDevStatsProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(NETDEV_STATS_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateNetDevStatsDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    static bool CreateNetDevStatsDB(const std::string& dbPath)
    {
        std::shared_ptr<NetDevStatsDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, NetDevStatsDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, NETDEV_STATS_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(NETDEV_STATS_DIR, 0));
        GlobalMockObject::verify();
    }
};

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
}

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnFalseWhenGetOriginalDataFailed)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;
    // QueryData failed
    MOCKER_CPP(&Analysis::Infra::Connection::QueryCmd).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&Analysis::Infra::Connection::QueryCmd).reset();
    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<NetDevStatsEventData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    Analysis::Test::ResetReserveFailureForVector<std::vector<NetDevStatsEventData>>();
    // SaveToDataInventory failed
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NetDevStatsEventData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NetDevStatsEventData>).reset();
}

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnFalseWhenComputeEventDataFailed)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&std::vector<NetDevStatsEventData>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&std::vector<NetDevStatsEventData>::empty).reset();
}

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnFalseWhenProcessSingleDeviceFailed)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(NetDevStatsProcessorUTest, TestRunShouldReturnTrueWhenCheckPathAndTableSuccess)
{
    auto processor = NetDevStatsProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(NOT_EXIST));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NETDEV_STATS));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}
