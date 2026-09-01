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

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/freq/include/lpm_info_parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/lpm_info_parser_item.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Utils;

namespace {
const std::string LPM_INFO_PATH = "./lpmInfoParser";
const std::string DATA_PATH = "./lpmInfoParser/data";

LpmInfoRawData CreateRawData(LpmInfoType type, const std::vector<std::pair<uint64_t, uint32_t>>& values)
{
    LpmInfoRawData data{};
    data.count = values.size();
    data.type = static_cast<uint32_t>(type);
    for (size_t i = 0; i < values.size(); ++i) {
        data.lpmDataS[i].sysCnt = values[i].first;
        data.lpmDataS[i].value = values[i].second;
    }
    return data;
}

bool WriteBytes(const std::string& name, const uint8_t* data, size_t size)
{
    std::ofstream output(File::PathJoin({DATA_PATH, name}), std::ios::out | std::ios::binary);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(data), size);
    return output.good();
}

size_t CountLogMessage(const std::vector<TestLogMessage>& logs, const std::string& message)
{
    return static_cast<size_t>(std::count_if(logs.begin(), logs.end(), [&message](const TestLogMessage& log) {
        return log.message == message;
    }));
}

size_t CountLogLevel(const std::vector<TestLogMessage>& logs, const std::string& level)
{
    return static_cast<size_t>(std::count_if(logs.begin(), logs.end(), [&level](const TestLogMessage& log) {
        return log.level == level;
    }));
}
}  // namespace

class LpmInfoParserUTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(File::CreateDir(LPM_INFO_PATH));
        ASSERT_TRUE(File::CreateDir(DATA_PATH));
        context_.isInitialized_ = true;
        context_.deviceContextInfo.deviceFilePath = LPM_INFO_PATH;
        context_.deviceContextInfo.deviceInfo.aicFrequency = 1850;
        context_.deviceContextInfo.deviceStart.cntVct = 100;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(File::RemoveDir(LPM_INFO_PATH, 0));
    }

    DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(LpmInfoParserUTest, ShouldParseThreeTypesFromLegacyAndNewFiles)
{
    std::vector<LpmInfoRawData> legacyData{
        CreateRawData(LpmInfoType::AIC_FREQ, {{50, 800}, {120, 1800}}),
        CreateRawData(LpmInfoType::AIC_VOLTAGE, {{60, 725}, {130, 785}}),
    };
    std::vector<LpmInfoRawData> newData{
        CreateRawData(LpmInfoType::BUS_VOLTAGE, {{70, 850}, {140, 930}}),
    };
    ASSERT_TRUE(WriteBin(legacyData, DATA_PATH, "lpmFreqConv.data.0.slice_0"));
    ASSERT_TRUE(WriteBin(newData, DATA_PATH, "lpmInfoConv.data.0.slice_1"));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2U, result->freqData.size());
    EXPECT_EQ(100U, result->freqData[0].sysCnt);
    EXPECT_EQ(800U, result->freqData[0].value);
    EXPECT_EQ(120U, result->freqData[1].sysCnt);
    EXPECT_EQ(1800U, result->freqData[1].value);
    ASSERT_EQ(2U, result->aicVoltageData.size());
    EXPECT_EQ(725U, result->aicVoltageData[0].value);
    EXPECT_EQ(785U, result->aicVoltageData[1].value);
    ASSERT_EQ(2U, result->busVoltageData.size());
    EXPECT_EQ(850U, result->busVoltageData[0].value);
    EXPECT_EQ(930U, result->busVoltageData[1].value);
}

TEST_F(LpmInfoParserUTest, ShouldParseRecordSplitAcrossFiles)
{
    auto rawData = CreateRawData(LpmInfoType::AIC_FREQ, {{120, 1800}});
    auto* bytes = reinterpret_cast<uint8_t*>(&rawData);
    const size_t firstPartSize = 100;
    ASSERT_TRUE(WriteBytes("lpmInfoConv.data.0.slice_0", bytes, firstPartSize));
    ASSERT_TRUE(WriteBytes("lpmInfoConv.data.0.slice_1", bytes + firstPartSize, sizeof(rawData) - firstPartSize));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2U, result->freqData.size());
    EXPECT_EQ(1850U, result->freqData[0].value);
    EXPECT_EQ(1800U, result->freqData[1].value);
}

TEST_F(LpmInfoParserUTest, ShouldUseLatestValueBeforeDeviceStartWhenInputIsUnordered)
{
    std::vector<LpmInfoRawData> input{
        CreateRawData(LpmInfoType::AIC_FREQ, {{90, 1900}, {120, 1800}}),
        CreateRawData(LpmInfoType::AIC_FREQ, {{95, 1950}, {70, 1700}}),
    };
    ASSERT_TRUE(WriteBin(input, DATA_PATH, "lpmInfoConv.data.0.slice_0"));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2U, result->freqData.size());
    EXPECT_EQ(100U, result->freqData[0].sysCnt);
    EXPECT_EQ(1950U, result->freqData[0].value);
    EXPECT_EQ(120U, result->freqData[1].sysCnt);
    EXPECT_EQ(1800U, result->freqData[1].value);
}

TEST_F(LpmInfoParserUTest, ShouldReturnEmptyDataWhenNoFileExists)
{
    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->freqData.empty());
    EXPECT_TRUE(result->aicVoltageData.empty());
    EXPECT_TRUE(result->busVoltageData.empty());
}

TEST_F(LpmInfoParserUTest, ShouldKeepValidRecordsWhenOneRecordIsInvalid)
{
    auto invalidData = CreateRawData(LpmInfoType::AIC_FREQ, {});
    invalidData.count = LPM_INFO_DATA_COUNT + 1;
    std::vector<LpmInfoRawData> input{
        CreateRawData(LpmInfoType::BUS_VOLTAGE, {{140, 930}}),
        invalidData,
        CreateRawData(LpmInfoType::AIC_VOLTAGE, {{130, 785}}),
    };
    ASSERT_TRUE(WriteBin(input, DATA_PATH, "lpmInfoConv.data.0.slice_0"));

    LpmInfoParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->freqData.empty());
    ASSERT_EQ(2U, result->aicVoltageData.size());
    EXPECT_EQ(785U, result->aicVoltageData[1].value);
    ASSERT_EQ(2U, result->busVoltageData.size());
    EXPECT_EQ(930U, result->busVoltageData[1].value);
}

TEST_F(LpmInfoParserUTest, ShouldAggregateParseFailuresAndKeepValidRecords)
{
    LpmInfoRawData invalidCountData{};
    invalidCountData.count = LPM_INFO_DATA_COUNT + 1;
    LpmInfoRawData invalidTypeData{};
    invalidTypeData.type = static_cast<uint32_t>(LpmInfoType::BUS_VOLTAGE) + 1;
    std::vector<LpmInfoRawData> input{
        invalidCountData,
        CreateRawData(LpmInfoType::BUS_VOLTAGE, {{140, 930}}),
        invalidTypeData,
        invalidCountData,
        CreateRawData(LpmInfoType::AIC_VOLTAGE, {{130, 785}}),
    };
    ASSERT_TRUE(WriteBin(input, DATA_PATH, "lpmInfoConv.data.0.slice_0"));

    StartTestLogCapture();
    LpmInfoParser parser;
    const auto status = parser.Run(dataInventory_, context_);
    const auto logs = StopTestLogCapture();

    ASSERT_EQ(ANALYSIS_OK, status);
    const std::string summary = "Parse LpmInfo data failed, failed count is 3, total count is 5, valid count is 2";
    EXPECT_EQ(1U, CountLogMessage(logs, summary));
    EXPECT_EQ(4U, CountLogLevel(logs, "[ERROR]"));
    EXPECT_EQ(2U, CountLogMessage(logs, "The count of LpmInfo data exceeds the maximum, count is 56"));
    EXPECT_EQ(1U, CountLogMessage(logs, "The type of LpmInfo data is invalid, type is 3"));
    auto result = dataInventory_.GetPtr<HalLpmInfoData>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2U, result->aicVoltageData.size());
    EXPECT_EQ(785U, result->aicVoltageData[1].value);
    ASSERT_EQ(2U, result->busVoltageData.size());
    EXPECT_EQ(930U, result->busVoltageData[1].value);
}

TEST_F(LpmInfoParserUTest, ShouldAggregateParseFailuresAndReturnErrorWhenAllRecordsAreInvalid)
{
    LpmInfoRawData rawData{};
    rawData.count = LPM_INFO_DATA_COUNT + 1;
    std::vector<LpmInfoRawData> input{rawData, rawData, rawData};
    ASSERT_TRUE(WriteBin(input, DATA_PATH, "lpmInfoConv.data.0.slice_0"));

    StartTestLogCapture();
    LpmInfoParser parser;
    const auto status = parser.Run(dataInventory_, context_);
    const auto logs = StopTestLogCapture();

    EXPECT_EQ(PARSER_PARSE_DATA_ERROR, status);
    const std::string summary = "Parse LpmInfo data failed, failed count is 3, total count is 3, valid count is 0";
    EXPECT_EQ(1U, CountLogMessage(logs, summary));
    EXPECT_EQ(6U, CountLogLevel(logs, "[ERROR]"));
    EXPECT_EQ(3U, CountLogMessage(logs, "The count of LpmInfo data exceeds the maximum, count is 56"));
    EXPECT_EQ(nullptr, dataInventory_.GetPtr<HalLpmInfoData>());
}
}  // namespace Domain
}  // namespace Analysis
