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
#include "analysis/csrc/domain/data_process/system/soc_bandwidth_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using ProcessedFormat = std::vector<SocBandwidthData>;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./soc_path";
const std::string DB_PATH = File::PathJoin({BASE_PATH, "msprof.db"});
const std::string DEVICE = "device_0";
const std::string DB_NAME = "soc_profiler.db";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394DFSFSQCD"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "InterSoc";

using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double>>;
OriDataFormat DATA_A{{0, 0, 236368325745670},
                     {0, 0, 236368325747550}};
OriDataFormat DATA_B{{0, 0, 236365909133170},
                     {0, 0, 236366032012930}};
}
class SocBandwidthProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE, SQLITE})));
        CreateSocMetricData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), DATA_A);
        CreateSocMetricData(File::PathJoin({PROF_PATH_B, DEVICE, SQLITE, DB_NAME}), DATA_B);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
        };
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).reset();
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    static void CreateSocMetricData(const std::string& dbPath, OriDataFormat data)
    {
        std::shared_ptr<SocProfilerDB> database;
        MAKE_SHARED0_RETURN_VOID(database, SocProfilerDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    }
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE, SQLITE, DB_NAME});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    }
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = SocBandwidthProcessor(File::PathJoin({BASE_PATH, "test"}));
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    Analysis::Test::StubReserveFailureForVector<ProcessedFormat>();
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    }
    Analysis::Test::ResetReserveFailureForVector<ProcessedFormat>();
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(SocBandwidthProcessorUTest, TestLoadDataShouldReturnOriDataWhenDbIsNull)
{
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        DBInfo socProfilerDB("soc_profiler.db", "InterSoc");
        socProfilerDB.dbRunner = nullptr;
        EXPECT_EQ(processor.LoadData(socProfilerDB, "").size(), 0ul);
    }
}

TEST_F(SocBandwidthProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();

        MOCKER_CPP(&DataProcessor::SaveToDataInventory<SocBandwidthData>).stubs().will(returnValue(false));
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
        MOCKER_CPP(&DataProcessor::SaveToDataInventory<SocBandwidthData>).reset();
    }
}

TEST_F(SocBandwidthProcessorUTest, TestRunShouldReturnFalseWhenProcessSingleDeviceFailed)
{
    for (auto path: PROF_PATHS) {
        auto processor = SocBandwidthProcessor(path);
        auto dataInventory = DataInventory();

        MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SOC));
        MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
    }
}
