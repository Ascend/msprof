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
#include "analysis/csrc/domain/services/parser/parser_item/sio_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"

namespace Analysis {
namespace Domain {
namespace {
constexpr uint16_t MAGIC = 0x6bd3;

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateSioV4Record(uint16_t accId, uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    auto* raw = reinterpret_cast<SioV4RawData*>(record.data());
    raw->funcType = PARSER_ITEM_SIO;
    raw->cnt = 3;
    raw->accType = 11;
    raw->magicNum = MAGIC;
    raw->sysCnt = sysCnt;
    raw->accId = accId;
    raw->reqRx = 100;
    raw->rspRx = 101;
    raw->snpRx = 102;
    raw->datRx = 103;
    raw->reqTx = 104;
    raw->rspTx = 105;
    raw->snpTx = 106;
    raw->datTx = 107;
    return record;
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateSioV6Record(uint8_t accId, uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    auto* raw = reinterpret_cast<SioV6RawData*>(record.data());
    raw->funcType = PARSER_ITEM_SIO;
    raw->accIdByte = static_cast<uint8_t>(accId << 2U);
    raw->magicNum = MAGIC;
    raw->sysCnt = sysCnt;
    raw->reqRx = 200;
    raw->rspRx = 201;
    raw->snpRx = 202;
    raw->datRx = 203;
    raw->reqTx = 204;
    raw->rspTx = 205;
    raw->snpTx = 206;
    raw->datTx = 207;
    return record;
}

void AssertBandwidth(const HalSioData& sio, uint32_t base)
{
    for (uint32_t i = 0; i < SIO_BANDWIDTH_COUNT; ++i) {
        ASSERT_EQ(base + i, sio.bandwidth[i]);
    }
}
}  // namespace

TEST(SioParserItemUTest, ShouldDecodeV4SioFields)
{
    auto record = CreateSioV4Record(13, 0x0102030405060708ULL);
    HalLogData result;
    const int count = SioParserItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_SIO, result.type);
    ASSERT_EQ(0x0102030405060708ULL, result.sio.sysCnt);
    ASSERT_EQ(13U, result.sio.accId);
    AssertBandwidth(result.sio, 100);
}

TEST(SioParserItemUTest, ShouldDecodeV6SioFields)
{
    auto record = CreateSioV6Record(9, 0x1112131415161718ULL);
    HalLogData result;
    const int count = SioParserItemV6(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_SIO, result.type);
    ASSERT_EQ(0x1112131415161718ULL, result.sio.sysCnt);
    ASSERT_EQ(9U, result.sio.accId);
    AssertBandwidth(result.sio, 200);
}

TEST(SioParserItemUTest, ShouldDecodeV6AccIdIndependentlyOfCntHighBits)
{
    for (const uint8_t accId : {0, 9, 63}) {
        for (uint8_t cntHighBits = 0; cntHighBits < 4; ++cntHighBits) {
            auto record = CreateSioV6Record(accId, 100);
            record[1] |= cntHighBits;
            HalLogData result;

            ASSERT_EQ(DEFAULT_CNT,
                      SioParserItemV6(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0));
            EXPECT_EQ(static_cast<uint32_t>(accId), result.sio.accId);
            EXPECT_EQ(100ULL, result.sio.sysCnt);
            AssertBandwidth(result.sio, 200);
        }
    }
}

TEST(SioParserItemUTest, ShouldRejectInvalidRecordSize)
{
    auto record = CreateSioV4Record(1, 100);
    HalLogData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              SioParserItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              SioParserItemV6(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(SioParserItemUTest, ShouldRegisterForSocProfileParsersOnly)
{
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER, PARSER_ITEM_SIO));
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER_V6, PARSER_ITEM_SIO));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(LOG_PARSER_V6, PARSER_ITEM_SIO));
}

}  // namespace Domain
}  // namespace Analysis
