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
#include "analysis/csrc/domain/data_process/system/llc_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string LLC_DIR = "./llc";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "llc.db";
const std::string PROF_DIR = File::PathJoin({LLC_DIR, "./PROF_0"});
const std::string TABLE_NAME = "LLCMetrics";
// device_id, l3tid, timestamp, hitrate, throughput
using LLcDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double, double, double>>;
using SummaryData = std::vector<std::tuple<uint32_t, double, double>>;
const LLcDataFormat LLC_DATA = {
    {0, 3, 1746926653520, 0.24780535338553, 46125.86192964499},
    {0, 3, 1746947297520, 0.18461538461538, 109.36894497190467},
    {0, 3, 1746965912520, 0.17953667953668, 108.06976900349181},
    {0, 3, 1746984498520, 0.19827586206897, 97.30946680296998},
    {0, 3, 1747003061520, 0.18028169014085, 74.70337499326618}
};
}
class LLcProcessorUTest : public testing::Test {
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

TEST_F(LLcProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LLcData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LLcData>).reset();
}

TEST_F(LLcProcessorUTest, TestRunShouldReturnTrueWhenIsFirstChipV1)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&Context::IsFirstChipV1).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&Context::IsFirstChipV1).reset();
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
    // 时间相关信息不存在
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    // LoadData failed
    MOCKER_CPP(&OriLLcData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&OriLLcData::empty).reset();
    // Reserve failed
    Analysis::Test::StubReserveFailureForVector<std::vector<LLcData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    Analysis::Test::ResetReserveFailureForVector<std::vector<LLcData>>();
}

TEST_F(LLcProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessSummaryDataFailed)
{
    auto processor = LLcProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&SummaryData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LLC));
    MOCKER_CPP(&SummaryData::empty).reset();
}
