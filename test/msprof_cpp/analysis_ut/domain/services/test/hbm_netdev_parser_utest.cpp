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

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_hbm.h"
#include "analysis/csrc/domain/entities/hal/include/hal_netdev_stats.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/hbm/include/hbm_parser.h"
#include "analysis/csrc/domain/services/parser/netdev_stats/include/netdev_stats_parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/hbm_parser_item.h"
#include "analysis/csrc/domain/services/persistence/device/hbm_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/netdev_stats_persistence.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string ROOT_DIR = "./hbm_netdev_parser_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});
using HbmSample = std::tuple<uint64_t, uint64_t, uint32_t, uint32_t>;
using HbmBandwidth = std::tuple<double, double, uint32_t, std::string>;

template <typename ValueType>
void WriteLittleEndian(std::vector<uint8_t> &buffer, size_t offset, ValueType value)
{
    for (size_t index = 0; index < sizeof(ValueType); ++index)
    {
        buffer[offset + index] = static_cast<uint8_t>((value >> (index * 8U)) & 0xffU);
    }
}

std::vector<uint8_t> CreateHbmV1Record(uint64_t timestamp, uint64_t count, uint32_t eventId, uint32_t hbmId)
{
    std::vector<uint8_t> record(24, 0);
    WriteLittleEndian(record, 0, timestamp);
    WriteLittleEndian(record, 8, count);
    WriteLittleEndian(record, 16, eventId);
    WriteLittleEndian(record, 20, hbmId);
    return record;
}

std::vector<uint8_t> CreateHbmV2Record(uint64_t timestamp, uint64_t count, uint32_t eventId, uint32_t hbmId)
{
    std::vector<uint8_t> record(32, 0);
    WriteLittleEndian(record, 8, eventId);
    WriteLittleEndian(record, 12, hbmId);
    WriteLittleEndian(record, 16, count);
    WriteLittleEndian(record, 24, timestamp);
    return record;
}

DeviceContext CreateContext(const std::string &version = "1.0", const std::string &events = "read,write")
{
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = ROOT_DIR;
    context.deviceContextInfo.collectionVersion = version;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    context.deviceContextInfo.deviceInfo.deviceId = 3;
    context.deviceContextInfo.deviceInfo.hwtsFrequency = 1000;
    context.deviceContextInfo.cpuInfo.frequency = 1000;
    context.deviceContextInfo.hostStartLog.clockMonotonicRaw = 100000;
    context.deviceContextInfo.startInfo.collectionTimeBegin = 200;
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 50000;
    context.deviceContextInfo.sampleInfo.hbmProfilingEvents = events;
    return context;
}

class HbmNetDevParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    void CheckRejectedNetDevRecord(const std::string &record)
    {
        Utils::FileWriter writer(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_0"}));
        writer.WriteText(record);
        writer.Close();
        NetDevStatsParser parser;
        Infra::DataInventory inventory;
        EXPECT_EQ(ANALYSIS_ERROR, parser.Run(inventory, CreateContext()));
        EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalNetDevStatsData>>());
    }

    void CheckRejectedHbmTime(uint64_t samplingTimestamp, const DeviceContext &context)
    {
        auto record = CreateHbmV1Record(samplingTimestamp, 88, 0, 2);
        ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
        HbmParser parser;
        HbmPersistence persistence;
        Infra::DataInventory inventory;
        ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
        EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, context));
    }

    void CheckHbmBandwidth(const std::vector<HbmSample> &samples, const std::vector<HbmBandwidth> &expected,
                           const std::string &events = "read")
    {
        std::vector<uint8_t> records;
        for (const auto &sample : samples)
        {
            const auto record = CreateHbmV2Record(std::get<0>(sample), std::get<1>(sample), std::get<2>(sample),
                                                  std::get<3>(sample));
            records.insert(records.end(), record.begin(), record.end());
        }
        ASSERT_TRUE(WriteBin(records, DATA_DIR, "hbm.data.0.slice_0"));
        HbmParser parser;
        HbmPersistence persistence;
        Infra::DataInventory inventory;
        const auto context = CreateContext("2.0", events);
        ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
        ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));
        Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
        std::vector<std::tuple<uint64_t>> originalCount;
        ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM HBMOriginalData", originalCount));
        ASSERT_EQ(1UL, originalCount.size());
        EXPECT_EQ(samples.size(), std::get<0>(originalCount[0]));
        std::vector<HbmBandwidth> rows;
        ASSERT_TRUE(runner.QueryData("SELECT timestamp,bandwidth,hbmId,event_type FROM HBMbwData "
                                    "ORDER BY timestamp,event_type,hbmId,bandwidth", rows));
        EXPECT_EQ(expected, rows);
    }
};

TEST_F(HbmNetDevParserUTest, ShouldDecodeBothHbmVersionsAndRejectWrongSize)
{
    std::vector<uint8_t> v1 = CreateHbmV1Record(1, 2, 3, 4);
    HalHbmData data;
    ASSERT_EQ(ANALYSIS_OK, HbmV1ParseItem(v1.data(), v1.size(), reinterpret_cast<uint8_t *>(&data), 0));
    EXPECT_EQ(1U, data.timestamp);
    EXPECT_EQ(2U, data.count);
    EXPECT_EQ(3U, data.eventId);
    EXPECT_EQ(4U, data.hbmId);
    EXPECT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              HbmV1ParseItem(v1.data(), v1.size() - 1, reinterpret_cast<uint8_t *>(&data), 0));

    std::vector<uint8_t> v2 = CreateHbmV2Record(10, 20, 0, 7);
    ASSERT_EQ(ANALYSIS_OK, HbmV2ParseItem(v2.data(), v2.size(), reinterpret_cast<uint8_t *>(&data), 0));
    EXPECT_EQ(10U, data.timestamp);
    EXPECT_EQ(20U, data.count);
    EXPECT_EQ(0U, data.eventId);
    EXPECT_EQ(7U, data.hbmId);
}

TEST_F(HbmNetDevParserUTest, ShouldParseHbmAcrossSlicesAndCreatePythonCompatibleTables)
{
    std::vector<uint8_t> firstRecord = CreateHbmV1Record(10, 100, 0, 1);
    std::vector<uint8_t> secondRecord = CreateHbmV1Record(20, 200, 1, 1);
    std::vector<uint8_t> first(firstRecord.begin(), firstRecord.begin() + 7);
    std::vector<uint8_t> second(firstRecord.begin() + 7, firstRecord.end());
    second.insert(second.end(), secondRecord.begin(), secondRecord.end());
    ASSERT_TRUE(WriteBin(second, DATA_DIR, "hbm.data.0.slice_10"));
    ASSERT_TRUE(WriteBin(first, DATA_DIR, "hbm.data.0.slice_2"));
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalHbmData>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    EXPECT_EQ(10U, parsed->at(0).timestamp);
    EXPECT_EQ(200U, parsed->at(1).count);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));

    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    EXPECT_TRUE(runner.CheckTableExists("HBMOriginalData"));
    EXPECT_TRUE(runner.CheckTableExists("HBMbwData"));
    using Row = std::tuple<uint32_t, uint32_t, double, uint64_t, std::string, uint32_t>;
    std::vector<Row> rows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM HBMOriginalData ORDER BY timestamp", rows));
    ASSERT_EQ(2UL, rows.size());
    EXPECT_EQ(3U, std::get<0>(rows[0]));
    EXPECT_EQ(0U, std::get<1>(rows[0]));
    EXPECT_DOUBLE_EQ(260000.0, std::get<2>(rows[0]));
    EXPECT_EQ("read", std::get<4>(rows[0]));
    EXPECT_EQ("write", std::get<4>(rows[1]));
    const Infra::TableColumns expectedOriginalColumns{{"device_id", Infra::SQL_INTEGER_TYPE},
                                                      {"replayid", Infra::SQL_INTEGER_TYPE},
                                                      {"timestamp", Infra::SQL_REAL_TYPE},
                                                      {"counts", Infra::SQL_INTEGER_TYPE},
                                                      {"event_type", Infra::SQL_TEXT_TYPE},
                                                      {"hbmid", Infra::SQL_INTEGER_TYPE}};
    EXPECT_EQ(expectedOriginalColumns, runner.GetTableColumns("HBMOriginalData"));
    const Infra::TableColumns expectedBwColumns{{"device_id", Infra::SQL_INTEGER_TYPE},
                                                {"timestamp", Infra::SQL_REAL_TYPE},
                                                {"bandwidth", Infra::SQL_REAL_TYPE},
                                                {"hbmid", Infra::SQL_INTEGER_TYPE},
                                                {"event_type", Infra::SQL_TEXT_TYPE}};
    EXPECT_EQ(expectedBwColumns, runner.GetTableColumns("HBMbwData"));
}

TEST_F(HbmNetDevParserUTest, ShouldParseHbmV2TimestampWithoutConversion)
{
    std::vector<uint8_t> record = CreateHbmV2Record(123456, 88, 0, 2);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("2.0", "read")));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext("2.0", "read")));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<double>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT timestamp FROM HBMOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_DOUBLE_EQ(123456.0, std::get<0>(rows[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldConvertLargeHbmTimestampWithSharedTimeUtilities)
{
    std::vector<uint8_t> record = CreateHbmV1Record(817077201, 88, 0, 2);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    DeviceContext context = CreateContext("1.0", "read");
    context.deviceContextInfo.hostStartLog.clockMonotonicRaw = 1500000067760436;
    context.deviceContextInfo.deviceStart.clockMonotonicRaw = 1499900273878287;
    context.deviceContextInfo.startInfo.collectionTimeBegin = 1700000126614242;
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 1500000531969374;
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<double>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT timestamp FROM HBMOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    // 按既有 C++ 时间语义：samplingUs * 1000 + hostNs + collectionUs * 1000 - startClockNs。
    // 精确整数为 1700000943227234062，转为 SQLite REAL 后取以下可表示值。
    EXPECT_EQ(1700000943227234048.0, std::get<0>(rows[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldConvertHbmV1TimestampWithSharedTimeUtilities)
{
    auto record = CreateHbmV1Record(484, 1524, 1, 2);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    DeviceContext context = CreateContext("1.0", "read");
    context.deviceContextInfo.hostStartLog.clockMonotonicRaw = 1005751764254705;
    context.deviceContextInfo.deviceStart.clockMonotonicRaw = 1005702776328280;
    context.deviceContextInfo.startInfo.collectionTimeBegin = 1789464015022103;
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 1005751763293860;
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<double>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT timestamp FROM HBMOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    // 同上，精确整数为 1789464015023547845，不再要求复现 Python 的浮点中间舍入。
    EXPECT_EQ(1789464015023547904.0, std::get<0>(rows[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldUseSharedHostClockCorrectionForHbm)
{
    auto record = CreateHbmV1Record(10, 88, 0, 2);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    auto context = CreateContext("1.0", "read");
    context.deviceContextInfo.cpuInfo.frequency = 2000.0;
    context.deviceContextInfo.hostStartLog.cntVctDiff = 7;
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<double>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT timestamp FROM HBMOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    // 既有 GetSyscntConversionParams 将 7 * 1000 / 2000 截断为 3 ns。
    EXPECT_EQ(260003.0, std::get<0>(rows[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldRejectHbmSamplingTimestampOverflow)
{
    CheckRejectedHbmTime(UINT64_MAX, CreateContext());
}

TEST_F(HbmNetDevParserUTest, ShouldRejectHbmCollectionStartOverflow)
{
    auto context = CreateContext();
    context.deviceContextInfo.startInfo.collectionTimeBegin = UINT64_MAX;
    CheckRejectedHbmTime(10, context);
}

TEST_F(HbmNetDevParserUTest, ShouldRejectInvalidHbmMonotonicBase)
{
    auto context = CreateContext();
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 200001;
    CheckRejectedHbmTime(10, context);
}

TEST_F(HbmNetDevParserUTest, ShouldRejectHbmAbsoluteTimestampOverflow)
{
    auto context = CreateContext();
    context.deviceContextInfo.startInfo.collectionTimeBegin = UINT64_MAX / 1000;
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 0;
    CheckRejectedHbmTime(10, context);
}

TEST_F(HbmNetDevParserUTest, ShouldFallbackToHbmV1AndSkipLeadingIncompleteBytes)
{
    std::vector<uint8_t> source{0xaa, 0xbb, 0xcc};
    std::vector<uint8_t> record = CreateHbmV1Record(9, 77, 0, 6);
    source.insert(source.end(), record.begin(), record.end());
    ASSERT_TRUE(WriteBin(source, DATA_DIR, "hbm.data.0.slice_0"));
    HbmParser parser;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("unknown")));
    const auto parsed = inventory.GetPtr<std::vector<HalHbmData>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(1UL, parsed->size());
    EXPECT_EQ(9U, parsed->at(0).timestamp);
    EXPECT_EQ(77U, parsed->at(0).count);
    EXPECT_EQ(6U, parsed->at(0).hbmId);
}

TEST_F(HbmNetDevParserUTest, ShouldCalculateHbmBandwidthWithPythonWindowSemantics)
{
    std::vector<uint8_t> records;
    for (uint64_t index = 0; index < 5; ++index)
    {
        std::vector<uint8_t> record = CreateHbmV1Record(index * 10, (index + 1) * 100, 0, 0);
        records.insert(records.end(), record.begin(), record.end());
    }
    ASSERT_TRUE(WriteBin(records, DATA_DIR, "hbm.data.0.slice_0"));
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("1.0", "read")));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext("1.0", "read")));

    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<double>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT bandwidth FROM HBMbwData ORDER BY timestamp", rows));
    ASSERT_EQ(5UL, rows.size());
    // Current Python uses one record per timestamp, not the former fixed four-record window.
    const std::vector<double> expected{0.0, 610.3515625, 915.52734375, 1220.703125, 1525.87890625};
    for (size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_EQ(expected[index], std::get<0>(rows[index]));
    }
}

TEST_F(HbmNetDevParserUTest, ShouldInferHbmWindowForNonFourChannelData)
{
    CheckHbmBandwidth({{20000, 32768, 0, 2}, {10000, 32768, 0, 1}, {20000, 32768, 0, 0},
                       {10000, 32768, 0, 2}, {20000, 32768, 0, 1}, {10000, 32768, 0, 0}},
                      {{10000.0, 0.0, 0, "read"}, {10000.0, 0.0, 1, "read"}, {10000.0, 0.0, 2, "read"},
                       {20000.0, 100000.0, 0, "read"}, {20000.0, 100000.0, 1, "read"},
                       {20000.0, 100000.0, 2, "read"}});
}

TEST_F(HbmNetDevParserUTest, ShouldFilterIncompleteHbmTimestampWithoutDeletingOriginalRows)
{
    CheckHbmBandwidth({{10000, 32768, 0, 0}, {10000, 65536, 0, 1}, {20000, 999, 0, 0},
                       {30000, 32768, 0, 0}, {30000, 65536, 0, 1}},
                      {{10000.0, 0.0, 0, "read"}, {10000.0, 0.0, 1, "read"},
                       {30000.0, 50000.0, 0, "read"}, {30000.0, 100000.0, 1, "read"}});
}

TEST_F(HbmNetDevParserUTest, ShouldPreferLargerHbmWindowOnModeTieAndKeepZeroDurationCounts)
{
    CheckHbmBandwidth({{10000, 100, 0, 0}, {20000, 100, 0, 0}, {20000, 200, 0, 1}},
                      {{20000.0, 100.0, 0, "read"}, {20000.0, 200.0, 1, "read"}});
}

TEST_F(HbmNetDevParserUTest, ShouldGroupHbmByCountsAndOnlyDeduplicateIdenticalRows)
{
    CheckHbmBandwidth({{10000, 32768, 0, 0}, {10000, 32768, 0, 0}, {10000, 65536, 0, 0},
                       {20000, 32768, 0, 0}, {20000, 65536, 0, 0}},
                      {{10000.0, 0.0, 0, "read"}, {10000.0, 0.0, 0, "read"},
                       {20000.0, 100000.0, 0, "read"}, {20000.0, 200000.0, 0, "read"}});
}

TEST_F(HbmNetDevParserUTest, ShouldInferSixteenRecordHbmWindowForDualEvents)
{
    std::vector<HbmSample> samples;
    std::vector<HbmBandwidth> expected;
    for (uint64_t timestamp : {10000U, 20000U})
    {
        for (uint32_t event = 0; event < 2; ++event)
        {
            for (uint32_t hbmId = 0; hbmId < 8; ++hbmId)
            {
                samples.emplace_back(timestamp, 32768, event, hbmId);
                expected.emplace_back(timestamp, timestamp == 10000 ? 0.0 : 100000.0, hbmId,
                                      event == 0 ? "read" : "write");
            }
        }
    }
    CheckHbmBandwidth(samples, expected, "read,write");
}

TEST_F(HbmNetDevParserUTest, ShouldKeepSingleHbmSampleWithTwoConfiguredEvents)
{
    CheckHbmBandwidth({{10000, 88, 0, 2}}, {{10000.0, 88.0, 2, "read"}}, "read,write");
}

TEST_F(HbmNetDevParserUTest, ShouldCalculateBandwidthForOneNanosecondDuration)
{
    CheckHbmBandwidth({{10000, 32768, 0, 0}, {10001, 32768, 0, 0}},
                      {{10000.0, 0.0, 0, "read"}, {10001.0, 1000000000.0, 0, "read"}});
}

TEST_F(HbmNetDevParserUTest, ShouldRejectInvalidHbmEventCountAfterPreservingOriginalRows)
{
    auto record = CreateHbmV2Record(10000, 88, 0, 2);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;
    const auto context = CreateContext("2.0", "read,write,invalid");
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, context));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"}));
    std::vector<std::tuple<uint64_t>> originalCount;
    std::vector<std::tuple<uint64_t>> bandwidthCount;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM HBMOriginalData", originalCount));
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM HBMbwData", bandwidthCount));
    ASSERT_EQ(1UL, originalCount.size());
    ASSERT_EQ(1UL, bandwidthCount.size());
    EXPECT_EQ(1U, std::get<0>(originalCount[0]));
    EXPECT_EQ(0U, std::get<0>(bandwidthCount[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldRejectHbmCountOutsideSqliteSignedIntegerRange)
{
    std::vector<uint8_t> record = CreateHbmV1Record(10, UINT64_MAX, 0, 0);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "hbm.data.0.slice_0"));
    HbmParser parser;
    HbmPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, CreateContext()));
}

TEST_F(HbmNetDevParserUTest, ShouldParseAndPersistNetDevStatsSlices)
{
    Utils::FileWriter second(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_10"}));
    second.WriteText("20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35\n");
    Utils::FileWriter first(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_2"}));
    first.WriteText("10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25\n");
    first.Close();
    second.Close();
    NetDevStatsParser parser;
    NetDevStatsPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalNetDevStatsData>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(2UL, parsed->size());
    EXPECT_EQ(10U, parsed->at(0).fields[0]);
    EXPECT_EQ(20U, parsed->at(1).fields[0]);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));

    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "netdev_stats.db"}));
    using Row = std::tuple<uint32_t, uint64_t, uint64_t>;
    std::vector<Row> rows;
    ASSERT_TRUE(runner.QueryData("SELECT device_id,timestamp,nic_rx_all_oct FROM NetDevStatsOriginalData ORDER BY "
                                 "timestamp", rows));
    ASSERT_EQ(2UL, rows.size());
    EXPECT_EQ(3U, std::get<0>(rows[0]));
    EXPECT_EQ(10U, std::get<1>(rows[0]));
    EXPECT_EQ(35U, std::get<2>(rows[1]));
}

TEST_F(HbmNetDevParserUTest, ShouldRejectNetDevStatsRecordWithTooFewFields)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14\n");
}

TEST_F(HbmNetDevParserUTest, ShouldRejectNetDevStatsRecordWithTooManyFields)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n");
}

TEST_F(HbmNetDevParserUTest, ShouldRejectBlankNetDevStatsRecord)
{
    CheckRejectedNetDevRecord(" \t\n");
}

TEST_F(HbmNetDevParserUTest, ShouldRejectNonnumericNetDevStatsField)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 invalid\n");
}

TEST_F(HbmNetDevParserUTest, ShouldRejectNegativeNetDevStatsField)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 -1\n");
}

TEST_F(HbmNetDevParserUTest, ShouldRejectNetDevStatsFieldAboveSqliteIntegerLimit)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 9223372036854775808\n");
}

TEST_F(HbmNetDevParserUTest, ShouldAcceptNetDevStatsIntegerLimitAndExtraWhitespace)
{
    Utils::FileWriter writer(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_0"}));
    writer.WriteText("  0\t1  2 3 4 5 6 7 8 9 10 11 12 13 14 9223372036854775807 \t\n");
    writer.Close();
    NetDevStatsParser parser;
    NetDevStatsPersistence persistence;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext()));
    const auto parsed = inventory.GetPtr<std::vector<HalNetDevStatsData>>();
    ASSERT_NE(nullptr, parsed);
    ASSERT_EQ(1UL, parsed->size());
    EXPECT_EQ(9223372036854775807ULL, parsed->at(0).fields[15]);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));
    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "netdev_stats.db"}));
    std::vector<std::tuple<uint64_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT nic_rx_all_oct FROM NetDevStatsOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(9223372036854775807ULL, std::get<0>(rows[0]));
}

TEST_F(HbmNetDevParserUTest, ShouldRejectEntireNetDevStatsBatchWhenMiddleRecordIsInvalid)
{
    CheckRejectedNetDevRecord("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\n"
                             "invalid\n"
                             "1 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\n");
}

TEST_F(HbmNetDevParserUTest, ShouldNotInjectEarlierNetDevStatsSlicesWhenLaterSliceIsInvalid)
{
    Utils::FileWriter first(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_2"}));
    first.WriteText("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\n");
    first.Close();
    Utils::FileWriter second(Utils::File::PathJoin({DATA_DIR, "netdev_stats.data.0.slice_10"}));
    second.WriteText("invalid\n");
    second.Close();
    NetDevStatsParser parser;
    Infra::DataInventory inventory;
    EXPECT_EQ(ANALYSIS_ERROR, parser.Run(inventory, CreateContext()));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<HalNetDevStatsData>>());
}

TEST_F(HbmNetDevParserUTest, ShouldNotCreateDatabaseWhenSourceIsMissing)
{
    Infra::DataInventory hbmInventory;
    HbmParser hbmParser;
    HbmPersistence hbmPersistence;
    ASSERT_EQ(ANALYSIS_OK, hbmParser.Run(hbmInventory, CreateContext()));
    ASSERT_EQ(ANALYSIS_OK, hbmPersistence.Run(hbmInventory, CreateContext()));

    Infra::DataInventory netdevInventory;
    NetDevStatsParser netdevParser;
    NetDevStatsPersistence netdevPersistence;
    ASSERT_EQ(ANALYSIS_OK, netdevParser.Run(netdevInventory, CreateContext()));
    ASSERT_EQ(ANALYSIS_OK, netdevPersistence.Run(netdevInventory, CreateContext()));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "hbm.db"})));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "netdev_stats.db"})));
}
}  // namespace
}  // namespace Domain
}  // namespace Analysis
