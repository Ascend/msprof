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
#include "analysis/csrc/domain//data_process/ai_task/hccl_statistic_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;
using ProcessedFormat = std::vector<HcclStatisticData>;

const std::string BASE_PATH = "./hccl_statistic";
const std::string DEVICE_SUFFIX = "device_0";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "PROF_1"});
const std::string DB_SUFFIX = "hccl_single_device.db";
const std::string TABLE_NAME = "HcclOpReport";
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};

const OriHcclDataFormat HCCL_DATA = {
    {"hcom_allReduce_",     "842",   1227513040,  13420,   1457853.966746, 483539540, 3.937677},
    {"hcom_allGather_",     "15969", 15251797640, 29880,   955087.835181,  8417640,   48.925465},
    {"hcom_broadcast_",     "801",   109981840,   52120,   137305.667915,  2484120,   0.352805},
    {"hcom_reduceScatter_", "13480", 14584244120, 1000920, 1081917.219585, 1763860,   46.784054}
};

class HcclStatisticProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, 0);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateHcclMetricData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), HCCL_DATA);
        CreateHcclMetricData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), HCCL_DATA);
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, 0));
    }
    static void CreateHcclMetricData(const std::string& dbPath, OriHcclDataFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnTrueWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}


TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = HcclStatisticProcessor({File::PathJoin({BASE_PATH, "test"})});
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    StubReserveFailureForVector<ProcessedFormat>();
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    ResetReserveFailureForVector<ProcessedFormat>();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenGetDeviceIdByDevicePathFailed)
{
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(INVALID_DEVICE_ID)));
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HcclStatisticData>)
    .stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HcclStatisticData>).reset();
}

TEST_F(HcclStatisticProcessorUTest, TestRunShouldReturnFalseWhenLoadDataFailed)
{
    OriHcclDataFormat oriData;
    MOCKER_CPP(&HcclStatisticProcessor::LoadData)
    .stubs().will(returnValue(oriData));
    for (auto path: PROF_PATHS) {
        auto processor = HcclStatisticProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&HcclStatisticProcessor::LoadData).reset();
}
