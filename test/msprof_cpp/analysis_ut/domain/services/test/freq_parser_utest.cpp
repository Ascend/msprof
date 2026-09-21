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

#include <gtest/gtest.h>
#include <algorithm>
#include <limits>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/freq/include/freq_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

using namespace testing;
using namespace Analysis::Utils;

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
    const std::string FREQ_LPM_PATH = "./lpmFreqConv";
    const int FREQ_COUNT = 50;
    const int AIC_FREQ = 1000;
    const uint32_t FREQ_CAPACITY = 55;

    class ScopedFreqItem {
    public:
        using ItemFunc = decltype(ParserItemFactory::GetParseItem(FREQ_PARSER, DEFAULT_FREQ_LPM));
        explicit ScopedFreqItem(ItemFunc replacement)
            : item_(ParserItemFactory::GetContainer().at(FREQ_PARSER).at(DEFAULT_FREQ_LPM)), original_(item_)
        {
            item_ = std::move(replacement);
        }
        ~ScopedFreqItem() { item_ = std::move(original_); }
    private:
        ItemFunc& item_;
        ItemFunc original_;
    };
}

class FreqParserUtest : public Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(FREQ_LPM_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({FREQ_LPM_PATH, "data"})));
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(FREQ_LPM_PATH, 0));
    }
    FreqData CreateFreqData(uint32_t count = FREQ_COUNT)
    {
        FreqData freqData{};
        freqData.count = count;
        for (uint32_t i = 0; i < count && i < FREQ_CAPACITY; ++i) {
            freqData.lpmDataS[i].freq = i + 1;
            freqData.lpmDataS[i].sysCnt = i;
        }
        return freqData;
    }

    void ExpectTruncationSummary(const std::vector<TestLogMessage>& logs, const std::string& devicePath,
                                 size_t records, uint32_t firstCount)
    {
        std::vector<TestLogMessage> warnings;
        for (const auto& log : logs) {
            EXPECT_NE("[ERROR]", log.level);
            if (log.level == "[WARN]") {
                warnings.push_back(log);
            }
        }
        if (records == 0) {
            EXPECT_TRUE(warnings.empty());
            return;
        }
        ASSERT_EQ(1U, warnings.size());
        EXPECT_EQ("Freq parsing in " + devicePath + ": " + std::to_string(records) +
                  " records exceeded capacity 55; only the first 55 entries of each were processed. First count: " +
                  std::to_string(firstCount) + ".", warnings[0].message);
    }

protected:
    Infra::DataInventory dataInventory_;
};

class FreqParserCountUtest : public FreqParserUtest, public WithParamInterface<uint32_t> {};

TEST_P(FreqParserCountUtest, ShouldBoundOutputCountWithoutPerRecordWarnings)
{
    auto rawData = CreateFreqData(GetParam());
    HalFreqData targetData{};
    targetData.count = std::numeric_limits<uint64_t>::max();
    for (auto& item : targetData.freqLpmDataS) {
        item.sysCnt = 9999;
        item.freq = 9999;
    }
    StartTestLogCapture();
    const auto result = FreqLpmParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData),
                                        reinterpret_cast<uint8_t*>(&targetData), 0);
    const auto logs = StopTestLogCapture();

    const auto expectedCount = std::min(GetParam(), FREQ_CAPACITY);
    ASSERT_EQ(DEFAULT_CNT, result);
    ASSERT_EQ(expectedCount, targetData.count);
    EXPECT_EQ(GetParam(), rawData.count);
    for (uint32_t i = 0; i < expectedCount; ++i) {
        EXPECT_EQ(rawData.lpmDataS[i].sysCnt, targetData.freqLpmDataS[i].sysCnt);
        EXPECT_EQ(rawData.lpmDataS[i].freq, targetData.freqLpmDataS[i].freq);
    }
    for (uint32_t i = expectedCount; i < FREQ_CAPACITY; ++i) {
        EXPECT_EQ(9999U, targetData.freqLpmDataS[i].sysCnt);
        EXPECT_EQ(9999U, targetData.freqLpmDataS[i].freq);
    }
    EXPECT_TRUE(logs.empty());
}

INSTANTIATE_TEST_SUITE_P(CountBoundaries, FreqParserCountUtest,
                        Values(0U, 1U, 55U, 56U, std::numeric_limits<uint32_t>::max()));

TEST_F(FreqParserUtest, ShouldPreserveItemSizeMismatchErrorBeforeCountHandling)
{
    // Verify the item API's existing error contract, not error propagation through Parser::Run.
    auto rawData = CreateFreqData(56);
    for (const auto size : {sizeof(rawData) - 1, sizeof(rawData) + 1}) {
        HalFreqData targetData{};
        targetData.count = 7;
        StartTestLogCapture();
        const auto result = FreqLpmParseItem(reinterpret_cast<uint8_t*>(&rawData), size,
                                            reinterpret_cast<uint8_t*>(&targetData), 0);
        const auto logs = StopTestLogCapture();
        EXPECT_EQ(PARSER_ERROR_SIZE_MISMATCH, result);
        EXPECT_EQ(7U, targetData.count);
        ASSERT_EQ(1U, logs.size());
        EXPECT_EQ("[ERROR]", logs[0].level);
        EXPECT_EQ("The TrunkSize of Freq is not equal with the FreqData struct", logs[0].message);
    }
}

class FreqParserTruncationUtest : public FreqParserUtest, public WithParamInterface<bool> {};

TEST_F(FreqParserUtest, ShouldMapItemResultToParserStatus)
{
    FreqParser parser;
    auto rawData = CreateFreqData(1);
    for (const auto size : {sizeof(rawData), sizeof(rawData) - 1, sizeof(rawData) + 1}) {
        HalFreqData target{};
        StartTestLogCapture();
        const auto result = parser.ParseDataItem(reinterpret_cast<uint8_t*>(&rawData), size,
                                                reinterpret_cast<uint8_t*>(&target));
        const auto logs = StopTestLogCapture();
        if (size == sizeof(rawData)) {
            EXPECT_EQ(ANALYSIS_OK, result);
            EXPECT_EQ(1U, target.count);
            EXPECT_TRUE(logs.empty());
        } else {
            EXPECT_EQ(PARSER_ERROR_SIZE_MISMATCH, result);
            ASSERT_EQ(1U, logs.size());
            EXPECT_EQ("[ERROR]", logs[0].level);
            EXPECT_EQ("The TrunkSize of Freq is not equal with the FreqData struct", logs[0].message);
        }
    }
}

class FreqParserFailureUtest : public FreqParserUtest, public WithParamInterface<bool> {};

TEST_P(FreqParserFailureUtest, ShouldClearFailedRecordsAndRecoverOnNextRun)
{
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    std::vector<FreqData> records{CreateFreqData(2), CreateFreqData(3), CreateFreqData(4)};
    ASSERT_TRUE(WriteBin(records, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0"));
    FreqParser parser;
    DataInventory firstInventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(firstInventory, context));
    ASSERT_EQ(records.size(), parser.halUniData_.size());
    for (size_t i = 0; i < records.size(); ++i) {
        ASSERT_EQ(records[i].count, parser.halUniData_[i].count);
    }

    const auto original = ParserItemFactory::GetParseItem(FREQ_PARSER, DEFAULT_FREQ_LPM);
    size_t calls = 0;
    const bool failAll = GetParam();
    DataInventory failedInventory;
    std::vector<TestLogMessage> logs;
    uint32_t result;
    {
        // Inject only the length argument. The real item generates the error and the real Run handles it.
        ScopedFreqItem injection([&](uint8_t* input, uint32_t size, uint8_t* output, uint16_t expandStatus) {
            EXPECT_EQ(sizeof(FreqData), size);
            const bool reject = failAll || calls == 1;
            ++calls;
            return original(input, reject ? size - 1 : size, output, expandStatus);
        });
        StartTestLogCapture();
        result = parser.Run(failedInventory, context);
        logs = StopTestLogCapture();
    }
    ASSERT_EQ(records.size(), calls);
    EXPECT_EQ(PARSER_PARSE_DATA_ERROR, result);
    auto data = failedInventory.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(failAll ? 1U : 7U, data->size());
    EXPECT_EQ(AIC_FREQ, data->front().freq);
    size_t index = 1;
    for (size_t i = 0; i < records.size(); ++i) {
        const bool rejected = failAll || i == 1;
        EXPECT_EQ(rejected ? 0U : records[i].count, parser.halUniData_[i].count);
        if (!rejected) {
            for (uint32_t j = 0; j < records[i].count; ++j) {
                EXPECT_EQ(records[i].lpmDataS[j].sysCnt, data->at(index).sysCnt);
                EXPECT_EQ(records[i].lpmDataS[j].freq, data->at(index).freq);
                ++index;
            }
        }
    }
    const size_t failures = failAll ? records.size() : 1;
    EXPECT_EQ(failures, std::count_if(logs.begin(), logs.end(), [](const TestLogMessage& log) {
        return log.level == "[ERROR]" && log.message == "The TrunkSize of Freq is not equal with the FreqData struct";
    }));
    EXPECT_EQ(2 * failures + 1, std::count_if(logs.begin(), logs.end(), [](const TestLogMessage& log) {
        return log.level == "[ERROR]";
    }));
    EXPECT_EQ(0, std::count_if(logs.begin(), logs.end(), [](const TestLogMessage& log) {
        return log.level == "[WARN]";
    }));

    DataInventory recoveredInventory;
    StartTestLogCapture();
    const auto recovered = parser.Run(recoveredInventory, context);
    logs = StopTestLogCapture();
    EXPECT_EQ(ANALYSIS_OK, recovered);
    data = recoveredInventory.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_NE(nullptr, data);
    EXPECT_EQ(10U, data->size());
    ExpectTruncationSummary(logs, FREQ_LPM_PATH, 0, 0);
}

INSTANTIATE_TEST_SUITE_P(MixedAndAllFailures, FreqParserFailureUtest, Values(false, true));

TEST_P(FreqParserTruncationUtest, ShouldContinueAfterOversizedRecord)
{
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    std::vector<FreqData> records{CreateFreqData(), CreateFreqData(56), CreateFreqData()};
    for (size_t record = 0; record < records.size(); ++record) {
        for (uint32_t i = 0; i < FREQ_CAPACITY; ++i) {
            records[record].lpmDataS[i].sysCnt += 1000 * record;
            records[record].lpmDataS[i].freq += 100 * record;
        }
    }
    const auto dataPath = File::PathJoin({FREQ_LPM_PATH, "data"});
    if (GetParam()) {
        std::vector<FreqData> firstSlice{records[0], records[1]};
        std::vector<FreqData> nextSlice{records[2]};
        ASSERT_TRUE(WriteBin(firstSlice, dataPath, "lpmFreqConv.data.0.slice_0"));
        ASSERT_TRUE(WriteBin(nextSlice, dataPath, "lpmFreqConv.data.0.slice_1"));
    } else {
        ASSERT_TRUE(WriteBin(records, dataPath, "lpmFreqConv.data.0.slice_0"));
    }
    FreqParser parser;
    StartTestLogCapture();
    const auto result = parser.Run(dataInventory_, context);
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, result);
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(1U + 2 * FREQ_COUNT + FREQ_CAPACITY, data->size());
    EXPECT_EQ(AIC_FREQ, data->at(0).freq);
    size_t index = 1;
    for (const auto& record : records) {
        for (uint32_t i = 0; i < std::min(record.count, FREQ_CAPACITY); ++i) {
            EXPECT_EQ(record.lpmDataS[i].sysCnt, data->at(index).sysCnt);
            EXPECT_EQ(record.lpmDataS[i].freq, data->at(index).freq);
            ++index;
        }
    }
    ExpectTruncationSummary(logs, FREQ_LPM_PATH, 1, 56);
}

INSTANTIATE_TEST_SUITE_P(SingleAndMultipleFiles, FreqParserTruncationUtest, Values(false, true));

TEST_F(FreqParserUtest, ShouldSummarizeOversizedRecordsAndRetainTheirBoundedData)
{
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    std::vector<FreqData> records{CreateFreqData(56), CreateFreqData(std::numeric_limits<uint32_t>::max())};
    ASSERT_TRUE(WriteBin(records, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0"));
    FreqParser parser;
    StartTestLogCapture();
    const auto result = parser.Run(dataInventory_, context);
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, result);
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(1U + 2 * FREQ_CAPACITY, data->size());
    EXPECT_EQ(AIC_FREQ, data->at(0).freq);
    for (size_t i = 1; i < data->size(); ++i) {
        EXPECT_EQ((i - 1) % FREQ_CAPACITY, data->at(i).sysCnt);
        EXPECT_EQ((i - 1) % FREQ_CAPACITY + 1, data->at(i).freq);
    }
    ExpectTruncationSummary(logs, FREQ_LPM_PATH, 2, 56);
}

TEST_F(FreqParserUtest, ShouldEmitOneWarningForManyOversizedRecords)
{
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    const size_t recordCount = 1000;
    std::vector<FreqData> records(recordCount, CreateFreqData(56));
    ASSERT_TRUE(WriteBin(records, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0"));
    FreqParser parser;
    StartTestLogCapture();
    const auto result = parser.Run(dataInventory_, context);
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, result);
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(1U + recordCount * FREQ_CAPACITY, data->size());
    EXPECT_EQ(AIC_FREQ, data->front().freq);
    EXPECT_EQ(FREQ_CAPACITY - 1, data->back().sysCnt);
    EXPECT_EQ(FREQ_CAPACITY, data->back().freq);
    ExpectTruncationSummary(logs, FREQ_LPM_PATH, recordCount, 56);
}

TEST_F(FreqParserUtest, ShouldResetSummaryAcrossDevicesAndRepeatedRuns)
{
    const std::vector<uint32_t> counts{56, std::numeric_limits<uint32_t>::max(), 55};
    const std::vector<size_t> recordCounts{2, 3, 1};
    std::vector<std::string> devicePaths;
    for (size_t device = 0; device < counts.size(); ++device) {
        const auto devicePath = File::PathJoin({FREQ_LPM_PATH, "device_" + std::to_string(device)});
        ASSERT_TRUE(File::CreateDir(devicePath));
        std::vector<FreqData> records(recordCounts[device], CreateFreqData(counts[device]));
        ASSERT_TRUE(WriteBin(records, File::PathJoin({devicePath, "data"}), "lpmFreqConv.data.0.slice_0"));
        devicePaths.push_back(devicePath);
    }
    FreqParser parser;
    for (const auto device : {0U, 0U, 1U, 2U}) {
        DeviceContext context;
        context.isInitialized_ = true;
        context.deviceContextInfo.deviceFilePath = devicePaths[device];
        context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
        context.deviceContextInfo.deviceStart.cntVct = 0;
        DataInventory inventory;
        StartTestLogCapture();
        const auto result = parser.Run(inventory, context);
        const auto logs = StopTestLogCapture();
        ASSERT_EQ(ANALYSIS_OK, result);
        auto data = inventory.GetPtr<std::vector<HalFreqLpmData>>();
        ASSERT_NE(nullptr, data);
        EXPECT_EQ(1U + recordCounts[device] * FREQ_CAPACITY, data->size());
        const size_t oversizedRecords = counts[device] > FREQ_CAPACITY ? recordCounts[device] : 0;
        ExpectTruncationSummary(logs, devicePaths[device], oversizedRecords, counts[device]);
    }
}

TEST_F(FreqParserUtest, ShouldReturnFreqLpmDataWhenParserRun)
{
    int expectSize = 2 * FREQ_COUNT + 1;
    std::vector<int> expectCount{FREQ_COUNT};
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(expectSize, data->size());
    ASSERT_EQ(0, data->data()[0].sysCnt);
    ASSERT_EQ(0, data->data()[1].sysCnt);
}

TEST_F(FreqParserUtest, ShouldReturnFreqLpmDataWhenMultiFile)
{
    int expectSize = 4 * FREQ_COUNT + 1;
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_1");
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(expectSize, data->size());
    ASSERT_EQ(0, data->data()[0].sysCnt);
    ASSERT_EQ(0, data->data()[1].sysCnt);
}

TEST_F(FreqParserUtest, ShouldReturnNoDataWhenNoFile)
{
    FreqParser freqParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(0ul, data->size());
}

TEST_F(FreqParserUtest, ShouldParseErrorWhenResizeException)
{
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    MOCKER_CPP(&Resize<HalFreqData>).stubs().will(returnValue(false));
    ASSERT_EQ(Analysis::PARSER_PARSE_DATA_ERROR, freqParser.Run(dataInventory_, context));
    MOCKER_CPP(&Resize<HalFreqData>).reset();
}
}
