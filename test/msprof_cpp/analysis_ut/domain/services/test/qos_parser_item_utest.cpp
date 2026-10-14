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

#include <array>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/qos_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis {
namespace Domain {
namespace {
constexpr uint16_t MAGIC = 0x6bd3;
constexpr uint64_t SYS_CNT = 0x0102030405ULL;
constexpr uint32_t METRIC_BASE = 100;

void WriteU16(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, uint32_t offset, uint16_t value)
{
    record[offset] = static_cast<uint8_t>(value & 0xFF);
    record[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void WriteU32(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, uint32_t offset, uint32_t value)
{
    for (uint32_t i = 0; i < sizeof(uint32_t); ++i) {
        record[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
}

void WriteU64(std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE>& record, uint32_t offset, uint64_t value)
{
    for (uint32_t i = 0; i < sizeof(uint64_t); ++i) {
        record[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateProfileQosRecord(uint16_t dieId, uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    const uint16_t header = static_cast<uint16_t>(QOS_V6_ITEM_TYPE | (dieId << 10U));
    WriteU16(record, 0, header);
    WriteU16(record, 2, MAGIC);
    WriteU64(record, 8, sysCnt);
    for (uint32_t i = 0; i < 10; ++i) {
        WriteU32(record, 24 + i * sizeof(uint32_t), METRIC_BASE + i);
    }
    return record;
}

std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> CreateV4QosRecord(uint64_t sysCnt)
{
    std::array<uint8_t, STARS_SOC_PROFILE_STRUCT_SIZE> record{{0}};
    WriteU64(record, 16, sysCnt);
    for (uint32_t i = 0; i < 10; ++i) {
        WriteU32(record, 24 + i * sizeof(uint32_t), METRIC_BASE + i);
    }
    return record;
}
}  // namespace

TEST(QosParserItemUTest, ShouldDecodeProfileQosFieldsAndSetProfileType)
{
    auto record = CreateProfileQosRecord(17, SYS_CNT);
    HalLogData result;
    const int count = QosProfileParseItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(DEFAULT_CNT, count);
    ASSERT_EQ(SOC_PROFILE_QOS, result.type);
    ASSERT_EQ(17, result.qos.dieId);
    ASSERT_EQ(static_cast<double>(SYS_CNT), result.qos.timestamp);
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_EQ(METRIC_BASE + i, result.qos.metrics[i]);
    }
}

TEST(QosParserItemUTest, ShouldRejectInvalidProfileRecordSize)
{
    auto record = CreateProfileQosRecord(1, 100);
    HalLogData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              QosProfileParseItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(QosParserItemUTest, ShouldRegisterProfileQosOnlyForSocProfileParserV6)
{
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(QOS_PARSER, QOS_V6_ITEM_TYPE));
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER_V6, QOS_V6_ITEM_TYPE));
}

TEST(QosParserItemUTest, ShouldDecodeV4QosFields)
{
    auto record = CreateV4QosRecord(SYS_CNT);
    HalQosBwData result;
    const int code = QosV4ParseItem(record.data(), record.size(), reinterpret_cast<uint8_t*>(&result), 0);

    ASSERT_EQ(ANALYSIS_OK, code);
    ASSERT_EQ(INVALID_VALUE, result.dieId);
    ASSERT_EQ(static_cast<double>(SYS_CNT), result.timestamp);
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_EQ(METRIC_BASE + i, result.metrics[i]);
    }
}

TEST(QosParserItemUTest, ShouldRejectInvalidV4RecordSize)
{
    auto record = CreateV4QosRecord(100);
    HalQosBwData result;
    ASSERT_EQ(PARSER_ERROR_SIZE_MISMATCH,
              QosV4ParseItem(record.data(), record.size() - 1, reinterpret_cast<uint8_t*>(&result), 0));
}

TEST(QosParserItemUTest, ShouldRegisterV4QosForQosParser)
{
    ASSERT_NE(nullptr, ParserItemFactory::GetParseItem(QOS_PARSER, QOS_V4_ITEM_TYPE));
    ASSERT_EQ(nullptr, ParserItemFactory::GetParseItem(SOC_PROFILE_PARSER_V6, QOS_V4_ITEM_TYPE));
}

}  // namespace Domain
}  // namespace Analysis
