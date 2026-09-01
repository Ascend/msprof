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

#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/freq/include/lpm_info_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/lpm_info_parser_item.h"
#include "analysis/csrc/domain/services/persistence/device/lpm_info_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Utils;

namespace {
const std::string INTEGRATION_PATH = "./lpmInfoIntegration";
const std::string INTEGRATION_DATA_PATH = "./lpmInfoIntegration/data";
const std::string INTEGRATION_SQLITE_PATH = "./lpmInfoIntegration/sqlite";
using QueryData = std::vector<std::tuple<uint64_t, uint32_t>>;

LpmInfoRawData CreateIntegrationData(LpmInfoType type, uint64_t sysCnt, uint32_t value)
{
    LpmInfoRawData data{};
    data.count = 1;
    data.type = static_cast<uint32_t>(type);
    data.lpmDataS[0].sysCnt = sysCnt;
    data.lpmDataS[0].value = value;
    return data;
}
}  // namespace

class LpmInfoIntegrationUTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(File::CreateDir(INTEGRATION_PATH));
        ASSERT_TRUE(File::CreateDir(INTEGRATION_DATA_PATH));
        ASSERT_TRUE(File::CreateDir(INTEGRATION_SQLITE_PATH));
        context_.isInitialized_ = true;
        context_.deviceContextInfo.deviceFilePath = INTEGRATION_PATH;
        context_.deviceContextInfo.deviceInfo.aicFrequency = 1850;
        context_.deviceContextInfo.deviceStart.cntVct = 100;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(File::RemoveDir(INTEGRATION_PATH, 0));
    }

    DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(LpmInfoIntegrationUTest, ShouldGenerateThreePythonCompatibleTablesFromRawData)
{
    std::vector<LpmInfoRawData> legacyData{
        CreateIntegrationData(LpmInfoType::AIC_FREQ, 120, 1800),
        CreateIntegrationData(LpmInfoType::AIC_VOLTAGE, 130, 785),
    };
    std::vector<LpmInfoRawData> newData{
        CreateIntegrationData(LpmInfoType::BUS_VOLTAGE, 140, 930),
    };
    ASSERT_TRUE(WriteBin(legacyData, INTEGRATION_DATA_PATH, "lpmFreqConv.data.0.slice_0"));
    ASSERT_TRUE(WriteBin(newData, INTEGRATION_DATA_PATH, "lpmInfoConv.data.0.slice_1"));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    QueryData freqResult;
    QueryData aicVoltageResult;
    QueryData busVoltageResult;
    DBRunner freqRunner(File::PathJoin({INTEGRATION_SQLITE_PATH, "freq.db"}));
    DBRunner voltageRunner(File::PathJoin({INTEGRATION_SQLITE_PATH, "voltage.db"}));
    ASSERT_TRUE(freqRunner.QueryData("SELECT syscnt, freq FROM FreqParse ORDER BY rowid", freqResult));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM AicVoltage ORDER BY rowid", aicVoltageResult));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM BusVoltage ORDER BY rowid", busVoltageResult));

    ASSERT_EQ(2U, freqResult.size());
    EXPECT_EQ(100U, std::get<0>(freqResult[0]));
    EXPECT_EQ(1850U, std::get<1>(freqResult[0]));
    EXPECT_EQ(120U, std::get<0>(freqResult[1]));
    EXPECT_EQ(1800U, std::get<1>(freqResult[1]));
    ASSERT_EQ(2U, aicVoltageResult.size());
    EXPECT_EQ(900U, std::get<1>(aicVoltageResult[0]));
    EXPECT_EQ(785U, std::get<1>(aicVoltageResult[1]));
    ASSERT_EQ(2U, busVoltageResult.size());
    EXPECT_EQ(900U, std::get<1>(busVoltageResult[0]));
    EXPECT_EQ(930U, std::get<1>(busVoltageResult[1]));
}

TEST_F(LpmInfoIntegrationUTest, ShouldPersistValidDataWhenOneRecordIsInvalid)
{
    LpmInfoRawData invalidData{};
    invalidData.count = LPM_INFO_DATA_COUNT + 1;
    std::vector<LpmInfoRawData> input{
        CreateIntegrationData(LpmInfoType::BUS_VOLTAGE, 140, 930),
        invalidData,
        CreateIntegrationData(LpmInfoType::AIC_VOLTAGE, 130, 785),
    };
    ASSERT_TRUE(WriteBin(input, INTEGRATION_DATA_PATH, "lpmInfoConv.data.0.slice_0"));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    EXPECT_FALSE(File::Exist(File::PathJoin({INTEGRATION_SQLITE_PATH, "freq.db"})));
    QueryData aicVoltageResult;
    QueryData busVoltageResult;
    DBRunner voltageRunner(File::PathJoin({INTEGRATION_SQLITE_PATH, "voltage.db"}));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM AicVoltage ORDER BY rowid", aicVoltageResult));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM BusVoltage ORDER BY rowid", busVoltageResult));
    ASSERT_EQ(2U, aicVoltageResult.size());
    EXPECT_EQ(785U, std::get<1>(aicVoltageResult[1]));
    ASSERT_EQ(2U, busVoltageResult.size());
    EXPECT_EQ(930U, std::get<1>(busVoltageResult[1]));
}
}  // namespace Domain
}  // namespace Analysis
