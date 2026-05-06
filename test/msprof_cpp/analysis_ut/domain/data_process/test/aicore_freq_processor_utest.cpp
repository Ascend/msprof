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
#include "analysis/csrc/domain/data_process/system/aicore_freq_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Test;

// syscnt, freq
using FreqDataFormat = std::vector<std::tuple<uint64_t, double>>;
// deviceId, timestampNs, freq
using ProcessedDataVecFormat = std::vector<AicoreFreqData>;
// deviceId, timestampNs, freq
using QueryDataFormat = std::vector<std::tuple<uint32_t, uint64_t, double>>;

const std::string DATA_DIR = "./freq";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DATA_DIR, MSPROF});
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});

const FreqDataFormat FREQ_DATA = {
    {484500000000000, 200},
    {484576969200418, 1650},
    {484576973402096, 1650},
    {484576973456197, 800},
    {484576973512465, 1650},
    {484577067389576, 1650},
};

class AicoreFreqProcessorUTest : public testing::Test {
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
        std::shared_ptr<FreqDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, FreqDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("FreqParse", database->GetTableCols("FreqParse")));
        EXPECT_TRUE(dbRunner->InsertData("FreqParse", FREQ_DATA));
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
            {"platform_version", "5"},
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

TEST_F(AicoreFreqProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "AICORE_FREQ";
    EXPECT_TRUE(processor.Run(dataInventory, processorName));

    // 有一条数据在采集范围外，将被过滤后，再新增两条开始结束记录
    uint16_t expectNum = FREQ_DATA.size() - 1 + 2;
    auto res = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    EXPECT_EQ(expectNum, res->size());
}

TEST_F(AicoreFreqProcessorUTest, TestRunShouldNotAddEndTimeRecordWhenEndTimeIsDefault)
{
    // endTimeNs == startTimeNs + DEFAULT_DURATION_TIME_NS时不追加结束时间记录
    if (!File::Exist(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE, "freq.db"}))) {
        EXPECT_TRUE(CreateFreqDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1715760307197379"},
        {"startClockMonotonicRaw", "9691377159398230"},
        {"hostMonotonic", "9691377161797070"},
        {"platform_version", "5"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "50"}, {"aic_frequency", "1650"}}}},
        {"devCntvct", "484576969200418"},
        {"hostCntvctDiff", "0"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    auto processor = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));

    // 无结束时间记录，仅: 过滤后的数据 + 开始时间记录
    uint16_t expectNum = FREQ_DATA.size() + 1;
    auto res = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    EXPECT_EQ(expectNum, res->size());
}

TEST_F(AicoreFreqProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    if (File::Exist(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE, "freq.db"}))) {
        EXPECT_TRUE(File::DeleteFile(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE, "freq.db"})));
    }
    auto processor = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "AICORE_FREQ";
    EXPECT_TRUE(processor.Run(dataInventory, processorName));

    uint16_t expectNum = 2;
    auto res = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    EXPECT_EQ(expectNum, res->size());
}

TEST_F(AicoreFreqProcessorUTest, TestProcessShouldReturnTrueWhenChipNotStars)
{
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(1));
    auto processor = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "AICORE_FREQ";
    EXPECT_TRUE(processor.Run(dataInventory, processorName));
}

TEST_F(AicoreFreqProcessorUTest, TestProcessShouldReturnFalseWhenProcessFailed)
{
    auto processor = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    auto processor2 = AicoreFreqProcessor(PROF);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    auto processor3 = AicoreFreqProcessor(PROF);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<AicoreFreqData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<AicoreFreqData>).reset();

    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(AicoreFreqProcessorUTest, TestLoadDataShouldReturnOriDataWhenDbIsNull)
{
    auto processor = AicoreFreqProcessor(PROF);
    DBInfo dbInfo("freq.db", "FreqParse");
    dbInfo.dbRunner = nullptr;
    EXPECT_EQ(processor.LoadData("", dbInfo).size(), 0ul);
}

TEST_F(AicoreFreqProcessorUTest, TestFormatDataShouldReturnFalseWhenFormatDataFailed)
{
    // GetPmuFreq failed
    MOCKER_CPP(&Context::GetPmuFreq).stubs().will(returnValue(false));
    auto processor1 = AicoreFreqProcessor(PROF);
    auto dataInventory = DataInventory();
    std::string processorName = "AICORE_FREQ";
    EXPECT_FALSE(processor1.Run(dataInventory, processorName));
    MOCKER_CPP(&Context::GetPmuFreq).reset();

    // Reserve failed
    StubReserveFailureForVector<ProcessedDataVecFormat>();
    auto processor2 = AicoreFreqProcessor(PROF);
    EXPECT_FALSE(processor2.Run(dataInventory, processorName));
    ResetReserveFailureForVector<ProcessedDataVecFormat>();
}
