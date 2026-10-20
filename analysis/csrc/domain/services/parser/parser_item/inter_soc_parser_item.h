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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_INTER_SOC_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_INTER_SOC_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"

namespace Analysis
{
namespace Domain
{

constexpr uint32_t PARSER_ITEM_INTER_SOC = 0b011100;

#pragma pack(1)
// 非 V6 结构：SOC_FMT = "HHLQ4LHH7L"
struct InterSocRawData
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t resv1 : 6;
    uint16_t magicNum;
    uint32_t resv2;
    uint64_t sysCnt;
    uint32_t resv3[4];
    uint16_t bwLevel;  // 低 3 位为 mata_bw_level，第 3~10 位为 buffer_bw_level
    uint16_t resv4;
    uint32_t resv5[7];
};
#pragma pack()

int InterSocParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_INTER_SOC_PARSER_ITEM_H
