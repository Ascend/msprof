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

#include <cstdint>
#include <fstream>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/pcie/include/pcie_parser.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
namespace {
const std::string PCIE_ROOT = "./pcie_parser";
const std::string PCIE_DATA_DIR = "./pcie_parser/data";

struct PcieWire {
    uint64_t timestamp;
    uint32_t deviceId;
    uint32_t txPBandwidthMin;
    uint32_t txPBandwidthMax;
    uint32_t txPBandwidthAvg;
    uint32_t txNpBandwidthMin;
    uint32_t txNpBandwidthMax;
    uint32_t txNpBandwidthAvg;
    uint32_t txCplBandwidthMin;
    uint32_t txCplBandwidthMax;
    uint32_t txCplBandwidthAvg;
    uint32_t txNpLatencyMin;
    uint32_t txNpLatencyMax;
    uint32_t txNpLatencyAvg;
    uint32_t rxPBandwidthMin;
    uint32_t rxPBandwidthMax;
    uint32_t rxPBandwidthAvg;
    uint32_t rxNpBandwidthMin;
    uint32_t rxNpBandwidthMax;
    uint32_t rxNpBandwidthAvg;
    uint32_t rxCplBandwidthMin;
    uint32_t rxCplBandwidthMax;
    uint32_t rxCplBandwidthAvg;
};

static_assert(sizeof(PcieWire) == 96, "PCIe test fixture must match Q22I");

PcieWire MakeWire(uint64_t timestamp, uint32_t rawDeviceId, uint32_t firstMetric)
{
    PcieWire wire{};
    wire.timestamp = timestamp;
    wire.deviceId = rawDeviceId;
    wire.txPBandwidthMin = firstMetric;
    wire.txPBandwidthMax = firstMetric + 1;
    wire.txPBandwidthAvg = firstMetric + 2;
    wire.txNpBandwidthMin = firstMetric + 3;
    wire.txNpBandwidthMax = firstMetric + 4;
    wire.txNpBandwidthAvg = firstMetric + 5;
    wire.txCplBandwidthMin = firstMetric + 6;
    wire.txCplBandwidthMax = firstMetric + 7;
    wire.txCplBandwidthAvg = firstMetric + 8;
    wire.txNpLatencyMin = firstMetric + 9;
    wire.txNpLatencyMax = firstMetric + 10;
    wire.txNpLatencyAvg = firstMetric + 11;
    wire.rxPBandwidthMin = firstMetric + 12;
    wire.rxPBandwidthMax = firstMetric + 13;
    wire.rxPBandwidthAvg = firstMetric + 14;
    wire.rxNpBandwidthMin = firstMetric + 15;
    wire.rxNpBandwidthMax = firstMetric + 16;
    wire.rxNpBandwidthAvg = firstMetric + 17;
    wire.rxCplBandwidthMin = firstMetric + 18;
    wire.rxCplBandwidthMax = firstMetric + 19;
    wire.rxCplBandwidthAvg = firstMetric + 20;
    return wire;
}

template<typename T>
std::vector<uint8_t> ToBytes(const std::vector<T> &records)
{
    const auto *begin = reinterpret_cast<const uint8_t *>(records.data());
    return std::vector<uint8_t>(begin, begin + records.size() * sizeof(T));
}

void WriteBytes(const std::string &name, const std::vector<uint8_t> &bytes)
{
    std::ofstream output(Utils::File::PathJoin({PCIE_DATA_DIR, name}), std::ios::out | std::ios::binary);
    ASSERT_TRUE(output.good());
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}

void ExpectMetrics(const HalPcieData &actual, uint32_t first)
{
    EXPECT_EQ(first + 0, actual.txPBandwidthMin);
    EXPECT_EQ(first + 1, actual.txPBandwidthMax);
    EXPECT_EQ(first + 2, actual.txPBandwidthAvg);
    EXPECT_EQ(first + 3, actual.txNpBandwidthMin);
    EXPECT_EQ(first + 4, actual.txNpBandwidthMax);
    EXPECT_EQ(first + 5, actual.txNpBandwidthAvg);
    EXPECT_EQ(first + 6, actual.txCplBandwidthMin);
    EXPECT_EQ(first + 7, actual.txCplBandwidthMax);
    EXPECT_EQ(first + 8, actual.txCplBandwidthAvg);
    EXPECT_EQ(first + 9, actual.txNpLatencyMin);
    EXPECT_EQ(first + 10, actual.txNpLatencyMax);
    EXPECT_EQ(first + 11, actual.txNpLatencyAvg);
    EXPECT_EQ(first + 12, actual.rxPBandwidthMin);
    EXPECT_EQ(first + 13, actual.rxPBandwidthMax);
    EXPECT_EQ(first + 14, actual.rxPBandwidthAvg);
    EXPECT_EQ(first + 15, actual.rxNpBandwidthMin);
    EXPECT_EQ(first + 16, actual.rxNpBandwidthMax);
    EXPECT_EQ(first + 17, actual.rxNpBandwidthAvg);
    EXPECT_EQ(first + 18, actual.rxCplBandwidthMin);
    EXPECT_EQ(first + 19, actual.rxCplBandwidthMax);
    EXPECT_EQ(first + 20, actual.rxCplBandwidthAvg);
}
}  // namespace

class PcieParserUtest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(PCIE_ROOT));
        ASSERT_TRUE(Utils::File::CreateDir(PCIE_DATA_DIR));
        context_.deviceContextInfo.deviceFilePath = PCIE_ROOT;
        context_.deviceContextInfo.deviceInfo.deviceId = 7;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(Utils::File::RemoveDir(PCIE_ROOT, 0));
    }

    std::shared_ptr<std::vector<HalPcieData>> RunParser()
    {
        PcieParser parser;
        EXPECT_EQ(static_cast<uint32_t>(ANALYSIS_OK), parser.Run(dataInventory_, context_));
        return dataInventory_.GetPtr<std::vector<HalPcieData>>();
    }

    Infra::DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(PcieParserUtest, ShouldParseAllTwentyThreeColumnsAndOverrideDeviceId)
{
    const std::vector<PcieWire> records{MakeWire(UINT64_C(0x100000002), 99, 100)};
    WriteBytes("pcie.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(1UL, actual->size());
    EXPECT_EQ(UINT64_C(0x100000002), actual->at(0).timestamp);
    EXPECT_EQ(7U, actual->at(0).deviceId);
    ExpectMetrics(actual->at(0), 100);
}

TEST_F(PcieParserUtest, ShouldParseMultipleRecords)
{
    const std::vector<PcieWire> records{MakeWire(11, 1, 10), MakeWire(22, 2, 40)};
    WriteBytes("pcie.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(11U, actual->at(0).timestamp);
    EXPECT_EQ(22U, actual->at(1).timestamp);
    ExpectMetrics(actual->at(1), 40);
}

TEST_F(PcieParserUtest, ShouldSortFilesByNumericSliceSuffix)
{
    WriteBytes("pcie.data.0.slice_10", ToBytes(std::vector<PcieWire>{MakeWire(10, 0, 10)}));
    WriteBytes("pcie.data.0.slice_2", ToBytes(std::vector<PcieWire>{MakeWire(2, 0, 2)}));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(2U, actual->at(0).timestamp);
    EXPECT_EQ(10U, actual->at(1).timestamp);
}

TEST_F(PcieParserUtest, ShouldSkipLeadingRemainderAndJoinRecordAcrossFiles)
{
    auto bytes = ToBytes(std::vector<PcieWire>{MakeWire(3, 0, 3), MakeWire(4, 0, 4)});
    bytes.insert(bytes.begin(), {1, 2, 3, 4, 5});
    WriteBytes("pcie.data.0.slice_0", std::vector<uint8_t>(bytes.begin(), bytes.begin() + 50));
    WriteBytes("pcie.data.0.slice_1", std::vector<uint8_t>(bytes.begin() + 50, bytes.end()));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(3U, actual->at(0).timestamp);
    EXPECT_EQ(4U, actual->at(1).timestamp);
}

TEST_F(PcieParserUtest, ShouldReturnEmptyVectorForTruncatedOnlyData)
{
    WriteBytes("pcie.data.0.slice_0", {1, 2, 3});

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST_F(PcieParserUtest, ShouldReturnEmptyVectorForEmptyFile)
{
    WriteBytes("pcie.data.0.slice_0", {});

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST_F(PcieParserUtest, ShouldReturnEmptyVectorWhenNoPcieFileExists)
{
    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST(PcieParserTypeUtest, ShouldPreserveRequiredFieldWidths)
{
    EXPECT_TRUE((std::is_same<decltype(HalPcieData::timestamp), uint64_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalPcieData::deviceId), uint32_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalPcieData::txPBandwidthMin), uint32_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalPcieData::rxCplBandwidthAvg), uint32_t>::value));
}

}  // namespace Domain
}  // namespace Analysis
