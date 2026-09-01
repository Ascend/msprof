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

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/low_power_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"

namespace Analysis {
namespace Domain {
namespace {
constexpr uint16_t MAGIC = 0x6bd3;

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

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateLowPowerRecord(uint16_t dieId, uint16_t count,
                                                                         uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    const uint16_t header = PARSER_ITEM_LOW_POWER | (count << 6) | (dieId << 10);
    WriteU16(record, 0, header);
    WriteU16(record, 2, MAGIC);
    WriteU64(record, 8, sysCnt);
    for (uint32_t i = 0; i < LOW_POWER_SAMPLE_COUNT; ++i) {
        WriteU16(record, 24 + i * sizeof(uint16_t), static_cast<uint16_t>(100 + i));
    }
    return record;
}
}  // namespace

TEST(LowPowerParserItemUTest, ShouldDecodeLowPowerFieldsAndSwapAdjacentSamples)
{
    auto record = CreateLowPowerRecord(17, 9, 0x0102030405060708ULL);
    HalSocProfileData result;
    const int count = LowPowerParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_LOW_POWER, result.type);
    ASSERT_EQ(17, result.lowPower.dieId);
    ASSERT_EQ(0x0102030405060708ULL, result.lowPower.sysCnt);
    for (uint32_t i = 0; i < LOW_POWER_SAMPLE_COUNT; ++i) {
        const uint16_t expected = static_cast<uint16_t>(100 + (i ^ 1U));
        ASSERT_EQ(expected, result.lowPower.sampleData[i]);
    }
}

TEST(LowPowerParserItemUTest, ShouldRejectInvalidRecordSize)
{
    auto record = CreateLowPowerRecord(1, 1, 100);
    HalSocProfileData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              LowPowerParserItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(LowPowerParserItemUTest, ShouldRegisterOnlyForSocProfileParser)
{
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER, PARSER_ITEM_LOW_POWER));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(LOG_PARSER_V6, PARSER_ITEM_LOW_POWER));
}

}  // namespace Domain
}  // namespace Analysis
