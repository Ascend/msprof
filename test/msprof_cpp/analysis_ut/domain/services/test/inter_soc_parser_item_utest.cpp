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
#include "analysis/csrc/domain/services/parser/parser_item/inter_soc_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"

namespace Analysis {
namespace Domain {
namespace {
constexpr uint16_t MAGIC = 0x6bd3;
constexpr uint16_t MATA_BW_LEVEL = 5;
constexpr uint16_t L2_BUFFER_BW_LEVEL = 0x66;

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateInterSocRecord(uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    auto* raw = reinterpret_cast<InterSocRawData*>(record.data());
    raw->funcType = PARSER_ITEM_INTER_SOC;
    raw->cnt = 4;
    raw->magicNum = MAGIC;
    raw->sysCnt = sysCnt;
    raw->bwLevel = static_cast<uint16_t>((L2_BUFFER_BW_LEVEL << 3U) | MATA_BW_LEVEL);
    return record;
}
}  // namespace

TEST(InterSocParserItemUTest, ShouldDecodeInterSocFields)
{
    auto record = CreateInterSocRecord(0x0102030405060708ULL);
    HalLogData result;
    const int count = InterSocParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_INTER_SOC, result.type);
    ASSERT_EQ(0x0102030405060708ULL, result.interSoc.sysCnt);
    ASSERT_EQ(MATA_BW_LEVEL, result.interSoc.mataBwLevel);
    ASSERT_EQ(L2_BUFFER_BW_LEVEL, result.interSoc.l2BufferBwLevel);
}

TEST(InterSocParserItemUTest, ShouldRejectInvalidRecordSize)
{
    auto record = CreateInterSocRecord(100);
    HalLogData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              InterSocParserItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(InterSocParserItemUTest, ShouldRegisterOnlyForSocProfileParser)
{
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER, PARSER_ITEM_INTER_SOC));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER_V6, PARSER_ITEM_INTER_SOC));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(LOG_PARSER_V6, PARSER_ITEM_INTER_SOC));
}

}  // namespace Domain
}  // namespace Analysis