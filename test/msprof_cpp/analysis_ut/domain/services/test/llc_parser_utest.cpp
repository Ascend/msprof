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
#include <limits>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
namespace {
const std::string LLC_ROOT = "./llc_parser";
const std::string LLC_DATA_DIR = "./llc_parser/data";

struct LlcV1Wire {
    uint64_t timestamp;
    uint64_t count;
    uint32_t event;
    uint32_t l3tid;
};

struct LlcV2Wire {
    uint32_t reserved;
    uint32_t count;
    uint32_t event;
    uint32_t l3tid;
    uint64_t timestamp;
};

static_assert(sizeof(LlcV1Wire) == 24, "LLC V1 test fixture must match QQII");
static_assert(sizeof(LlcV2Wire) == 24, "LLC V2 test fixture must match IIIIQ");

template<typename T>
std::vector<uint8_t> ToBytes(const std::vector<T> &records)
{
    const auto *begin = reinterpret_cast<const uint8_t *>(records.data());
    return std::vector<uint8_t>(begin, begin + records.size() * sizeof(T));
}

void WriteBytes(const std::string &name, const std::vector<uint8_t> &bytes)
{
    std::ofstream output(Utils::File::PathJoin({LLC_DATA_DIR, name}), std::ios::out | std::ios::binary);
    ASSERT_TRUE(output.good());
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(output.good());
}
}  // namespace

class LlcParserUtest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(LLC_ROOT));
        ASSERT_TRUE(Utils::File::CreateDir(LLC_DATA_DIR));
        context_.deviceContextInfo.deviceFilePath = LLC_ROOT;
        context_.deviceContextInfo.deviceInfo.deviceId = 7;
        context_.deviceContextInfo.deviceInfo.collectionVersion = "1.0";
        context_.deviceContextInfo.hostStartLog.clockMonotonicRaw = 1000000;
        context_.deviceContextInfo.deviceStart.clockMonotonicRaw = 500000;
        context_.deviceContextInfo.startInfo.collectionTimeBegin = 10000;
        context_.deviceContextInfo.startInfo.clockMonotonicRaw = 9000000;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(Utils::File::RemoveDir(LLC_ROOT, 0));
    }

    std::shared_ptr<std::vector<HalLlcData>> RunParser()
    {
        LlcParser parser;
        EXPECT_EQ(static_cast<uint32_t>(ANALYSIS_OK), parser.Run(dataInventory_, context_));
        return dataInventory_.GetPtr<std::vector<HalLlcData>>();
    }

    Infra::DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(LlcParserUtest, ShouldParseV1RecordsAndConvertAbsoluteTime)
{
    const std::vector<LlcV1Wire> records{{4, UINT64_C(0x100000002), 0x34, 1},
                                         {9, UINT64_C(0x200000003), 0x36, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(7U, actual->at(0).deviceId);
    EXPECT_EQ(UINT64_C(2004000), actual->at(0).timestamp);
    EXPECT_EQ(UINT64_C(0x100000002), actual->at(0).count);
    EXPECT_EQ(0x34U, actual->at(0).event);
    EXPECT_EQ(1U, actual->at(0).l3tid);
    EXPECT_EQ(UINT64_C(2009000), actual->at(1).timestamp);
}

TEST_F(LlcParserUtest, ShouldMatchPythonFloatRoundingForLargeV1AbsoluteTime)
{
    context_.deviceContextInfo.hostStartLog.clockMonotonicRaw = UINT64_C(8121584747441305);
    context_.deviceContextInfo.deviceStart.clockMonotonicRaw = UINT64_C(8121583217919180);
    context_.deviceContextInfo.startInfo.collectionTimeBegin = UINT64_C(1784971328877269);
    context_.deviceContextInfo.startInfo.clockMonotonicRaw = UINT64_C(8121586262461640);
    const std::vector<LlcV1Wire> records{{164044, 73989, 0x00, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(1UL, actual->size());
    EXPECT_EQ(UINT64_C(1784971327526292480), actual->at(0).timestamp);
}

TEST_F(LlcParserUtest, ShouldParseV2RecordWithoutChangingTimestamp)
{
    context_.deviceContextInfo.deviceInfo.collectionVersion = "2.0";
    const std::vector<LlcV2Wire> records{{0xFFFFFFFFU, 23, 0x14, 1, UINT64_C(0x100000001)}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(1UL, actual->size());
    EXPECT_EQ(UINT64_C(0x100000001), actual->at(0).timestamp);
    EXPECT_EQ(23U, actual->at(0).count);
    EXPECT_EQ(0x14U, actual->at(0).event);
    EXPECT_EQ(1U, actual->at(0).l3tid);
}

TEST_F(LlcParserUtest, ShouldUseV1ForUnknownCollectionVersion)
{
    context_.deviceContextInfo.deviceInfo.collectionVersion = "unexpected";
    const std::vector<LlcV1Wire> records{{6, 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(1UL, actual->size());
    EXPECT_EQ(UINT64_C(2006000), actual->at(0).timestamp);
    EXPECT_EQ(17U, actual->at(0).count);
}

TEST_F(LlcParserUtest, ShouldUseV1WhenCollectionVersionIsUnavailable)
{
    context_.deviceContextInfo.deviceInfo.collectionVersion = NA;
    const std::vector<LlcV1Wire> records{{6, 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(1UL, actual->size());
    EXPECT_EQ(UINT64_C(2006000), actual->at(0).timestamp);
    EXPECT_EQ(17U, actual->at(0).count);
}

TEST_F(LlcParserUtest, ShouldReturnErrorForUnrepresentableV1Timestamp)
{
    const std::vector<LlcV1Wire> records{{std::numeric_limits<uint64_t>::max(), 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    LlcParser parser;
    EXPECT_EQ(static_cast<uint32_t>(PARSER_PARSE_DATA_ERROR), parser.Run(dataInventory_, context_));
    EXPECT_EQ(nullptr, dataInventory_.GetPtr<std::vector<HalLlcData>>());
}

TEST_F(LlcParserUtest, ShouldReturnErrorWhenV1TimeContextIsMissing)
{
    context_.deviceContextInfo.deviceStart = {};
    const std::vector<LlcV1Wire> records{{4, 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    LlcParser parser;
    EXPECT_EQ(static_cast<uint32_t>(PARSER_PARSE_DATA_ERROR), parser.Run(dataInventory_, context_));
    EXPECT_EQ(nullptr, dataInventory_.GetPtr<std::vector<HalLlcData>>());
}

TEST_F(LlcParserUtest, ShouldReturnErrorWhenHostMonotonicPrecedesDeviceStart)
{
    context_.deviceContextInfo.hostStartLog.clockMonotonicRaw = 1000;
    context_.deviceContextInfo.deviceStart.clockMonotonicRaw = 2000;
    const std::vector<LlcV1Wire> records{{4, 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    LlcParser parser;
    EXPECT_EQ(static_cast<uint32_t>(PARSER_PARSE_DATA_ERROR), parser.Run(dataInventory_, context_));
    EXPECT_EQ(nullptr, dataInventory_.GetPtr<std::vector<HalLlcData>>());
}

TEST_F(LlcParserUtest, ShouldReturnErrorWhenCollectionEpochPrecedesMonotonicClock)
{
    context_.deviceContextInfo.startInfo.collectionTimeBegin = 8999;
    const std::vector<LlcV1Wire> records{{4, 17, 0x20, 0}};
    WriteBytes("llc.data.0.slice_0", ToBytes(records));

    LlcParser parser;
    EXPECT_EQ(static_cast<uint32_t>(PARSER_PARSE_DATA_ERROR), parser.Run(dataInventory_, context_));
    EXPECT_EQ(nullptr, dataInventory_.GetPtr<std::vector<HalLlcData>>());
}

TEST_F(LlcParserUtest, ShouldSortFilesByNumericSliceSuffix)
{
    context_.deviceContextInfo.deviceInfo.collectionVersion = "2.0";
    std::vector<LlcV2Wire> slice10{{0, 10, 0x22, 0, 10}};
    std::vector<LlcV2Wire> slice2{{0, 2, 0x20, 0, 2}};
    WriteBytes("llc.data.0.slice_10", ToBytes(slice10));
    WriteBytes("llc.data.0.slice_2", ToBytes(slice2));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(2U, actual->at(0).timestamp);
    EXPECT_EQ(10U, actual->at(1).timestamp);
}

TEST_F(LlcParserUtest, ShouldSkipLeadingRemainderAndJoinRecordAcrossFiles)
{
    context_.deviceContextInfo.deviceInfo.collectionVersion = "2.0";
    const std::vector<LlcV2Wire> records{{0, 3, 0x20, 0, 3}, {0, 4, 0x22, 1, 4}};
    auto bytes = ToBytes(records);
    bytes.insert(bytes.begin(), {0xAA, 0xBB, 0xCC});
    WriteBytes("llc.data.0.slice_0", std::vector<uint8_t>(bytes.begin(), bytes.begin() + 15));
    WriteBytes("llc.data.0.slice_1", std::vector<uint8_t>(bytes.begin() + 15, bytes.end()));

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    ASSERT_EQ(2UL, actual->size());
    EXPECT_EQ(3U, actual->at(0).timestamp);
    EXPECT_EQ(4U, actual->at(1).timestamp);
}

TEST_F(LlcParserUtest, ShouldReturnEmptyVectorForTruncatedOnlyData)
{
    WriteBytes("llc.data.0.slice_0", {1, 2, 3, 4, 5});

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST_F(LlcParserUtest, ShouldReturnEmptyVectorForEmptyFile)
{
    WriteBytes("llc.data.0.slice_0", {});

    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST_F(LlcParserUtest, ShouldReturnEmptyVectorWhenNoLlcFileExists)
{
    auto actual = RunParser();

    ASSERT_NE(nullptr, actual);
    EXPECT_TRUE(actual->empty());
}

TEST(LlcParserTypeUtest, ShouldPreserveRequiredFieldWidths)
{
    EXPECT_TRUE((std::is_same<decltype(HalLlcData::timestamp), uint64_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalLlcData::count), uint64_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalLlcData::event), uint32_t>::value));
    EXPECT_TRUE((std::is_same<decltype(HalLlcData::l3tid), uint32_t>::value));
}

}  // namespace Domain
}  // namespace Analysis
