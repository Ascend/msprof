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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * -------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/low_power_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const std::string PROFILE_PATH = "./stars_soc_profile";
constexpr uint8_t TEST_PROFILE_FUNC_TYPE = 0b111111;
constexpr uint8_t TEST_FAILED_PROFILE_FUNC_TYPE = 0b111110;

int TestSocProfileParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)binaryData;
    (void)expandStatus;
    if (binaryDataSize != STARS_SOC_PROFILE_STRUCT_SIZE)
    {
        return -1;
    }
    auto* target = reinterpret_cast<HalSocProfileData*>(halUniData);
    target->type = SOC_PROFILE_LOW_POWER;
    target->lowPower.sysCnt = 9876;
    return DEFAULT_CNT;
}

int FailedSocProfileParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData,
                               uint16_t expandStatus)
{
    (void)binaryData;
    (void)binaryDataSize;
    (void)halUniData;
    (void)expandStatus;
    return ANALYSIS_ERROR;
}

REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, TEST_PROFILE_FUNC_TYPE, TestSocProfileParserItem);
REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, TEST_FAILED_PROFILE_FUNC_TYPE, FailedSocProfileParserItem);

void WriteU16(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, uint32_t offset, uint16_t value)
{
    record[offset] = static_cast<uint8_t>(value & 0xFF);
    record[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void WriteU64(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, uint32_t offset, uint64_t value)
{
    for (uint32_t i = 0; i < sizeof(uint64_t); ++i) {
        record[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateRecord(uint8_t funcType, uint64_t sysCnt,
                                                                 uint16_t magic = 0x6bd3)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    WriteU16(record, 0, static_cast<uint16_t>(funcType | (3U << 6) | (2U << 10)));
    WriteU16(record, 2, magic);
    WriteU64(record, 8, sysCnt);
    for (uint32_t i = 0; i < LOW_POWER_SAMPLE_COUNT; ++i) {
        WriteU16(record, 24 + i * sizeof(uint16_t), static_cast<uint16_t>(i));
    }
    return record;
}

bool WriteRecords(const std::string& fileName,
                  const std::vector<std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>>& records)
{
    std::ofstream output(File::PathJoin({PROFILE_PATH, "data", fileName}), std::ios::binary | std::ios::app);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(records.data()), records.size() * STARS_SOC_PROFILE_STRUCT_SIZE);
    return output.good();
}
}  // namespace

class StarsSocProfileParserUTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(File::CreateDir(PROFILE_PATH));
        ASSERT_TRUE(File::CreateDir(File::PathJoin({PROFILE_PATH, "data"})));
        context_.deviceContextInfo.deviceFilePath = PROFILE_PATH;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(File::RemoveDir(PROFILE_PATH, 0));
    }

    DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(StarsSocProfileParserUTest, ShouldReadOnlyProfileFileAndKeepOnlyLowPowerRecords)
{
    ASSERT_TRUE(WriteRecords("stars_soc.data.0.slice_0", {CreateRecord(PARSER_ITEM_LOW_POWER, 50)}));
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_0",
                             {CreateRecord(0b011010, 80), CreateRecord(PARSER_ITEM_LOW_POWER, 100),
                              CreateRecord(PARSER_ITEM_LOW_POWER, 120, 0)}));

    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1UL, result->size());
    ASSERT_EQ(SOC_PROFILE_LOW_POWER, result->at(0).type);
    ASSERT_EQ(100UL, result->at(0).lowPower.sysCnt);
    ASSERT_EQ(2, result->at(0).lowPower.dieId);
    ASSERT_EQ(1, result->at(0).lowPower.sampleData[0]);
    ASSERT_EQ(0, result->at(0).lowPower.sampleData[1]);
}

TEST_F(StarsSocProfileParserUTest, ShouldAcceptSingleRecordAndPreserveSliceOrder)
{
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_1", {CreateRecord(PARSER_ITEM_LOW_POWER, 200)}));
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_0", {CreateRecord(PARSER_ITEM_LOW_POWER, 100)}));

    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2UL, result->size());
    ASSERT_EQ(100UL, result->at(0).lowPower.sysCnt);
    ASSERT_EQ(200UL, result->at(1).lowPower.sysCnt);
}

TEST_F(StarsSocProfileParserUTest, ShouldDispatchRegisteredParserItemByFuncType)
{
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_0", {CreateRecord(TEST_PROFILE_FUNC_TYPE, 100)}));

    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1UL, result->size());
    ASSERT_EQ(9876UL, result->at(0).lowPower.sysCnt);
}

TEST_F(StarsSocProfileParserUTest, ShouldReturnEmptyDataWhenProfileFileDoesNotExist)
{
    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_TRUE(result->empty());
}

TEST_F(StarsSocProfileParserUTest, ShouldReturnEmptyDataWhenAllRecordsHaveInvalidMagic)
{
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_0",
                             {CreateRecord(PARSER_ITEM_LOW_POWER, 100, 0),
                              CreateRecord(PARSER_ITEM_LOW_POWER, 200, 0)}));

    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_TRUE(result->empty());
}

TEST_F(StarsSocProfileParserUTest, ShouldKeepValidRecordsWhenOneParserItemFails)
{
    ASSERT_TRUE(WriteRecords("stars_soc_profile.data.0.slice_0",
                             {CreateRecord(PARSER_ITEM_LOW_POWER, 100),
                              CreateRecord(TEST_FAILED_PROFILE_FUNC_TYPE, 200),
                              CreateRecord(PARSER_ITEM_LOW_POWER, 300)}));

    StarsSocProfileParser parser;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context_));
    auto result = dataInventory_.GetPtr<std::vector<HalSocProfileData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2UL, result->size());
    ASSERT_EQ(100UL, result->at(0).lowPower.sysCnt);
    ASSERT_EQ(300UL, result->at(1).lowPower.sysCnt);
}

}  // namespace Domain
}  // namespace Analysis
