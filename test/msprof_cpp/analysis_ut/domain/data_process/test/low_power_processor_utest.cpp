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
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/data_process/system/low_power_processor.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

using LowPowerDataFormat = std::vector<std::tuple<uint64_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t,
uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, double, uint16_t, uint16_t,
uint16_t, uint16_t, uint16_t, uint16_t,  uint16_t, uint16_t, uint16_t>>;
using ProcessedDataVecFormat = std::vector<LowPowerData>;

const std::string DATA_DIR = "./lowpower";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DATA_DIR, MSPROF});
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});

const LowPowerDataFormat FREQ_DATA = {
    {484500000000000, 0, 2, 22, 23, 11, 13, 0, 0, 14, 6, 0, 200, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576969200418, 0, 18, 22, 31, 11, 13, 0, 32, 10, 6, 16, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973402096, 0, 29, 36, 23, 11, 13, 0, 0, 14, 6, 18, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973456197, 0, 11, 263, 3, 11, 8, 0, 0, 14, 17, 23, 800, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973512465, 0, 12, 42, 3, 11, 2, 0, 0, 14, 90, 139, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484577067389576, 0, 73, 332, 22, 11, 45, 0, 0, 120, 96, 118, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484500000000000, 1, 22, 222, 26, 11, 13, 0, 0, 14, 6, 182, 300, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576969200418, 1, 27, 221, 6, 11, 13, 0, 0, 14, 60, 186, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973402096, 1, 79, 24, 38, 11, 13, 0, 0, 14, 60, 183, 1650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973456197, 1, 211, 22, 23, 11, 13, 0, 0, 14, 76, 108, 1000, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484576973512465, 1, 17, 62, 93, 11, 13, 0, 0, 14, 66, 128, 2000, 119, 231, 123, 135, 145, 147, 89, 765, 43},
    {484577067389576, 1, 7, 18, 3, 11, 13, 0, 0, 14, 61, 118, 2650, 119, 231, 123, 135, 145, 147, 89, 765, 43},
};

class LowPowerProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DATA_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateFreqDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreateFreqDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<LowPowerDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, LowPowerDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("LowPower", database->GetTableCols("LowPower")));
        EXPECT_TRUE(dbRunner->InsertData("LowPower", FREQ_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1715760307197379"},
            {"endCollectionTimeEnd", "1715760313397397"},
            {"startClockMonotonicRaw", "9691377159398230"},
            {"hostMonotonic", "9691377161797070"},
            {"platform_version", "15"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "50"}, {"aic_frequency", "1650"}}}},
            {"devCntvct", "484576969200418"},
            {"hostCntvctDiff", "0"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

TEST_F(LowPowerProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = LowPowerProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "LOW_POWER";
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, processorName));

    uint16_t expectNum = FREQ_DATA.size();
    auto res = dataInventory.GetPtr<std::vector<LowPowerData>>();
    EXPECT_EQ(expectNum, res->size());
}

TEST_F(LowPowerProcessorUTest, TestProcessShouldReturnTrueWhenChipNotStars)
{
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(true));
    auto processor = LowPowerProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "LOW_POWER";
    EXPECT_TRUE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&Context::IsChipV6).reset();
}

TEST_F(LowPowerProcessorUTest, TestProcessShouldReturnFalseWhenProcessFailed)
{
    auto processor = LowPowerProcessor(PROF);
    auto dataInventory = DataInventory();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LOW_POWER));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    auto processor2 = LowPowerProcessor(PROF);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LOW_POWER));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(UINT16_MAX));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LOW_POWER));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LowPowerData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LOW_POWER));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<LowPowerData>).reset();

    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_LOW_POWER));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    MOCKER_CPP(&Context::IsChipV6).reset();
}

TEST_F(LowPowerProcessorUTest, TestLoadDataShouldReturnOriDataWhenDbIsNull)
{
    auto processor = LowPowerProcessor(PROF);
    DBInfo dbInfo("lowpower.db", "LowPower");
    dbInfo.dbRunner = nullptr;
    EXPECT_EQ(processor.LoadData("", dbInfo).size(), 0ul);
}