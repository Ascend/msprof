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

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/system/llc_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace
{
const std::string LLC_DIR = "./llc";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "llc.db";
const std::string PROF_DIR = File::PathJoin({LLC_DIR, "./PROF_0"});
const std::string TABLE_NAME = "LLCMetrics";
// device_id, l3tid, timestamp, hitrate, throughput
using LLcDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double, double, double>>;
const LLcDataFormat LLC_DATA = {{0, 3, 1746926653520, 0.24780535338553, 46125.86192964499},
                                {0, 3, 1746947297520, 0.18461538461538, 109.36894497190467},
                                {0, 3, 1746965912520, 0.17953667953668, 108.06976900349181},
                                {0, 3, 1746984498520, 0.19827586206897, 97.30946680296998},
                                {0, 3, 1747003061520, 0.18028169014085, 74.70337499326618}};
}  // namespace
class LLcProcessorUTest : public testing::Test
{
   protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(LLC_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateLLCDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"llc_profiling", "read"},
            {"platform_version", "5"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    static bool CreateLLCDB(const std::string& dbPath)
    {
        std::shared_ptr<LLCDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, LLCDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, LLC_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(LLC_DIR, 0));
        GlobalMockObject::verify();
    }
};

TEST_F(LLcProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
}

TEST_F(LLcProcessorUTest, SummaryAverageUsesOneVotePerLlcId)
{
    const std::string profPath = File::PathJoin({LLC_DIR, "PROF_summary_average"});
    const std::string devicePath = File::PathJoin({profPath, DEVICE_SUFFIX});
    const std::string sqlitePath = File::PathJoin({devicePath, SQLITE});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_TRUE(File::CreateDir(sqlitePath));
    auto database = std::make_shared<LLCDB>();
    DBRunner runner(File::PathJoin({sqlitePath, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
    const LLcDataFormat rows = {
        {0, 0, 1746926653520, 0.1234567, 10.1234},
        {0, 0, 1746947297520, 0.1234567, 30.1234},
        {0, 1, 1746965912520, 0.8, 100},
        {0, 2, 1746970000000, 0.1234565, 60},
        {1, 0, 1746984498520, 0.9, 1000},
        {1, 2, 1747003061520, 0.5, 500},
    };
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, rows));

    LLcProcessor processor(profPath);
    DataInventory inventory;
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_LLC));
    const auto summary = inventory.GetPtr<std::vector<LLcSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(4UL, summary->size());
    EXPECT_TRUE(summary->at(0).isAverage);
    EXPECT_NEAR(34.8971066666667, summary->at(0).hitRate, 1e-12);
    EXPECT_NEAR(60.0411333333333, summary->at(0).throughput, 1e-12);
    EXPECT_EQ(0, summary->at(1).llcId);
    EXPECT_NEAR(12.34567, summary->at(1).hitRate, 1e-12);
    EXPECT_DOUBLE_EQ(20.1234, summary->at(1).throughput);
    EXPECT_EQ(1, summary->at(2).llcId);
    EXPECT_DOUBLE_EQ(80.0, summary->at(2).hitRate);
    EXPECT_EQ(2, summary->at(3).llcId);
    EXPECT_NEAR(12.34565, summary->at(3).hitRate, 1e-12);
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}

TEST_F(LLcProcessorUTest, ShouldPreserveLlcIdsAcrossUint8Boundary)
{
    const std::string profPath = File::PathJoin({LLC_DIR, "PROF_llc_id_boundary"});
    const std::string devicePath = File::PathJoin({profPath, DEVICE_SUFFIX});
    const std::string sqlitePath = File::PathJoin({devicePath, SQLITE});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_TRUE(File::CreateDir(sqlitePath));
    auto database = std::make_shared<LLCDB>();
    DBRunner runner(File::PathJoin({sqlitePath, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, LLcDataFormat{
        {0, 255, 1746926653520, 0.2, 10},
        {0, 256, 1746947297520, 0.4, 20},
    }));

    LLcProcessor processor(profPath);
    DataInventory inventory;
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_LLC));
    const auto summary = inventory.GetPtr<std::vector<LLcSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(3UL, summary->size());
    EXPECT_TRUE(summary->at(0).isAverage);
    EXPECT_EQ(255U, summary->at(1).llcId);
    EXPECT_EQ(256U, summary->at(2).llcId);
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}

TEST_F(LLcProcessorUTest, ShouldNotGenerateSummaryWhenOnlyAnotherDeviceHasMetrics)
{
    const std::string profPath = File::PathJoin({LLC_DIR, "PROF_other_device_only"});
    const std::string devicePath = File::PathJoin({profPath, DEVICE_SUFFIX});
    const std::string sqlitePath = File::PathJoin({devicePath, SQLITE});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_TRUE(File::CreateDir(sqlitePath));
    auto database = std::make_shared<LLCDB>();
    DBRunner runner(File::PathJoin({sqlitePath, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, LLcDataFormat{{1, 0, 1746926653520, 0.5, 100}}));

    LLcProcessor processor(profPath);
    DataInventory inventory;
    EXPECT_FALSE(processor.Run(inventory, PROCESSOR_NAME_LLC));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<LLcSummaryData>>());
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}

TEST_F(LLcProcessorUTest, ShouldFailWhenLlcMetricsQueryFails)
{
    const std::string profPath = File::PathJoin({LLC_DIR, "PROF_invalid_metrics"});
    const std::string devicePath = File::PathJoin({profPath, DEVICE_SUFFIX});
    const std::string sqlitePath = File::PathJoin({devicePath, SQLITE});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_TRUE(File::CreateDir(sqlitePath));
    DBRunner runner(File::PathJoin({sqlitePath, DB_SUFFIX}));
    ASSERT_TRUE(runner.UpdateData("CREATE TABLE LLCMetrics (device_id INTEGER)"));

    LLcProcessor processor(profPath);
    DataInventory inventory;
    EXPECT_FALSE(processor.Run(inventory, PROCESSOR_NAME_LLC));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<LLcSummaryData>>());
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}

TEST_F(LLcProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LLcData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LLcData>).reset();
}

TEST_F(LLcProcessorUTest, TestRunShouldReturnFalseWhenProcessSingleDeviceFailed)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(LLcProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;
    // LoadData failed
    MOCKER_CPP(&OriLLcData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&OriLLcData::empty).reset();
    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<LLcData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    Analysis::Test::ResetReserveFailureForVector<std::vector<LLcData>>();
}
