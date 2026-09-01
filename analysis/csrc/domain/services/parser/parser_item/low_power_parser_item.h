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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_LOW_POWER_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_LOW_POWER_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"

namespace Analysis
{
namespace Domain
{

constexpr uint32_t PARSER_ITEM_LOW_POWER = 0b011101;

#pragma pack(1)
struct LowPowerData
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t dieId : 6;
    uint16_t magicNum;
    uint32_t resv1;
    uint64_t sysCnt;
    uint64_t resv2;
    uint16_t sampleData[LOW_POWER_SAMPLE_COUNT];
};
#pragma pack()

int LowPowerParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_LOW_POWER_PARSER_ITEM_H
