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
#include "analysis/csrc/domain/data_process/system/hbm_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Test;

namespace {
const int DEPTH = 0;
const std::string HBM_DIR = "./hbm";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hbm.db";
const std::string PROF_DIR = File::PathJoin({HBM_DIR, "./PROF_0"});
const std::string TABLE_NAME = "HBMbwData";
// device_id, timestamp, bandwidth, hbmid, event_type
using HbmDataFormat = std::vector<std::tuple<uint32_t, double, double, uint8_t, std::string>>;
using SummaryData = std::vector<std::tuple<uint32_t, double, double>>;
const HbmDataFormat HBM_DATA = {
    {2, 71735309797040, 20.82875495833125, 0, "read"},
    {2, 71735309797040, 20.40716971570149, 1, "read"},
    {2, 71735309797040, 20.12305792175534, 2, "read"},
    {2, 71735309797040, 20.75849075122629, 3, "read"},
    {2, 71735309797040, 111.44514239951948, 0, "write"},
    {2, 71735309797040, 113.74247647529906, 1, "write"},
    {2, 71735309797040, 112.54798495451473, 2, "write"},
    {2, 71735309797040, 114.35041461503329, 3, "write"},
};
}
class HBMProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(HBM_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateHBMDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
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

    static bool CreateHBMDB(const std::string& dbPath)
    {
        std::shared_ptr<HBMDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, HBMDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, HBM_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HBM_DIR, 0));
        GlobalMockObject::verify();
    }
};

TEST_F(HBMProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
}

TEST_F(HBMProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HbmData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HbmData>).reset();

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(HBMProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 时间相关信息不存在
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    // LoadData failed
    MOCKER_CPP(&OriHbmData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&OriHbmData::empty).reset();
    // Reserve failed
    StubReserveFailureForVector<std::vector<HbmData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    ResetReserveFailureForVector<std::vector<HbmData>>();
}

TEST_F(HBMProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessSummaryDataFailed)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&SummaryData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&SummaryData::empty).reset();
}
