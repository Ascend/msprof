/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/chip_trans_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"

namespace Analysis {
namespace Domain {
namespace {
constexpr uint16_t MAGIC = 0x6bd3;
constexpr uint16_t ACC_TYPE_PA = 0b111111;
constexpr uint16_t ACC_TYPE_PCIE = 0b100010;

template <typename T>
void WriteValue(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, size_t offset, T value)
{
    for (size_t i = 0; i < sizeof(T); ++i) {
        record[offset + i] = static_cast<uint8_t>(value >> (i * 8));
    }
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateChipTransV4Record(uint16_t accType, uint16_t eventId,
                                                                            uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    WriteValue<uint16_t>(record, 0, PARSER_ITEM_CHIP_TRANS | (5U << 6) | (accType << 10));
    WriteValue<uint16_t>(record, 2, MAGIC);
    WriteValue<uint64_t>(record, 8, sysCnt);
    WriteValue<uint16_t>(record, 18, eventId);
    WriteValue<uint64_t>(record, 32, 1234);
    WriteValue<uint64_t>(record, 40, 5678);
    return record;
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateChipTransV6Record(uint16_t dieId, uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    WriteValue<uint16_t>(record, 0, PARSER_ITEM_CHIP_TRANS | (6U << 6) | (dieId << 10));
    WriteValue<uint16_t>(record, 2, MAGIC);
    WriteValue<uint64_t>(record, 8, sysCnt);
    WriteValue<uint64_t>(record, 16, 300);
    WriteValue<uint64_t>(record, 24, 400);
    return record;
}

void AssertCommonChipTransFields(const HalChipTransData& chipTrans, uint64_t sysCnt)
{
    ASSERT_EQ(sysCnt, chipTrans.sysCnt);
    ASSERT_EQ(1234ULL, chipTrans.value1);
    ASSERT_EQ(5678ULL, chipTrans.value2);
}
}  // namespace

TEST(ChipTransParserItemUTest, ShouldDecodeV4PaChipTransFields)
{
    auto record = CreateChipTransV4Record(ACC_TYPE_PA, 0x1234, 0x0102030405060708ULL);
    HalLogData result;
    const int count = ChipTransParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_CHIP_TRANS, result.type);
    ASSERT_EQ(CHIP_TRANS_PA, result.chipTrans.transType);
    ASSERT_EQ(0x34U, result.chipTrans.id);
    AssertCommonChipTransFields(result.chipTrans, 0x0102030405060708ULL);
}

TEST(ChipTransParserItemUTest, ShouldDecodeV4PcieChipTransFields)
{
    auto record = CreateChipTransV4Record(ACC_TYPE_PCIE, 0x1235, 0x1112131415161718ULL);
    HalLogData result;
    const int count = ChipTransParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_CHIP_TRANS, result.type);
    ASSERT_EQ(CHIP_TRANS_PCIE, result.chipTrans.transType);
    ASSERT_EQ(0x35U, result.chipTrans.id);
    AssertCommonChipTransFields(result.chipTrans, 0x1112131415161718ULL);
}

TEST(ChipTransParserItemUTest, ShouldDecodeInvalidV4ChipTransType)
{
    auto record = CreateChipTransV4Record(1, 0x1236, 100);
    HalLogData result;
    const int count = ChipTransParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_CHIP_TRANS, result.type);
    ASSERT_EQ(CHIP_TRANS_INVALID, result.chipTrans.transType);
    ASSERT_EQ(0x36U, result.chipTrans.id);
}

TEST(ChipTransParserItemUTest, ShouldDecodeV6ChipTransFields)
{
    auto record = CreateChipTransV6Record(17, 0x2122232425262728ULL);
    HalLogData result;
    const int count = ChipTransParserItemV6(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_CHIP_TRANS, result.type);
    ASSERT_EQ(CHIP_TRANS_V6, result.chipTrans.transType);
    ASSERT_EQ(17U, result.chipTrans.id);
    ASSERT_EQ(0x2122232425262728ULL, result.chipTrans.sysCnt);
    ASSERT_EQ(300ULL, result.chipTrans.value1);
    ASSERT_EQ(400ULL, result.chipTrans.value2);
}

TEST(ChipTransParserItemUTest, ShouldRejectInvalidRecordSize)
{
    auto record = CreateChipTransV4Record(ACC_TYPE_PA, 1, 100);
    HalLogData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              ChipTransParserItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              ChipTransParserItemV6(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(ChipTransParserItemUTest, ShouldRegisterForSocProfileParsersOnly)
{
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER, PARSER_ITEM_CHIP_TRANS));
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER_V6, PARSER_ITEM_CHIP_TRANS));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(LOG_PARSER_V6, PARSER_ITEM_CHIP_TRANS));
}

}  // namespace Domain
}  // namespace Analysis
