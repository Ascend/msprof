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

#include "analysis/csrc/domain/services/parser/parser_item/lpm_info_parser_item.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Domain {
TEST(LpmInfoParserItemUTest, ShouldDecodeValidData)
{
    LpmInfoRawData rawData{};
    rawData.count = 1;
    rawData.type = static_cast<uint32_t>(LpmInfoType::AIC_VOLTAGE);
    rawData.lpmDataS[0].sysCnt = 100;
    rawData.lpmDataS[0].value = 725;
    HalLpmInfoRecord targetData;

    ASSERT_EQ(ANALYSIS_OK, LpmInfoParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData),
                                           reinterpret_cast<uint8_t*>(&targetData), 0));
    EXPECT_EQ(1U, targetData.count);
    EXPECT_EQ(LpmInfoType::AIC_VOLTAGE, targetData.type);
    EXPECT_EQ(100U, targetData.lpmDataS[0].sysCnt);
    EXPECT_EQ(725U, targetData.lpmDataS[0].value);
}

TEST(LpmInfoParserItemUTest, ShouldReturnErrorWhenSizeMismatch)
{
    LpmInfoRawData rawData{};
    HalLpmInfoRecord targetData;
    EXPECT_EQ(ANALYSIS_ERROR, LpmInfoParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData) - 1,
                                              reinterpret_cast<uint8_t*>(&targetData), 0));
}

TEST(LpmInfoParserItemUTest, ShouldReturnErrorWhenBinaryDataIsNull)
{
    HalLpmInfoRecord targetData;
    EXPECT_EQ(ANALYSIS_ERROR,
              LpmInfoParseItem(nullptr, sizeof(LpmInfoRawData), reinterpret_cast<uint8_t*>(&targetData), 0));
}

TEST(LpmInfoParserItemUTest, ShouldReturnErrorWhenHalUniDataIsNull)
{
    LpmInfoRawData rawData{};
    EXPECT_EQ(ANALYSIS_ERROR,
              LpmInfoParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData), nullptr, 0));
}

TEST(LpmInfoParserItemUTest, ShouldReturnErrorWhenCountExceedsCapacity)
{
    LpmInfoRawData rawData{};
    rawData.count = LPM_INFO_DATA_COUNT + 1;
    HalLpmInfoRecord targetData;
    EXPECT_EQ(ANALYSIS_ERROR, LpmInfoParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData),
                                               reinterpret_cast<uint8_t*>(&targetData), 0));
}

TEST(LpmInfoParserItemUTest, ShouldReturnErrorWhenTypeIsUnknown)
{
    LpmInfoRawData rawData{};
    rawData.type = 3;
    HalLpmInfoRecord targetData;
    EXPECT_EQ(ANALYSIS_ERROR, LpmInfoParseItem(reinterpret_cast<uint8_t*>(&rawData), sizeof(rawData),
                                               reinterpret_cast<uint8_t*>(&targetData), 0));
}
}  // namespace Domain
}  // namespace Analysis
