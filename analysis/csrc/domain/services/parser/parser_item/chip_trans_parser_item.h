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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_CHIP_TRANS_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_CHIP_TRANS_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{

constexpr uint32_t PARSER_ITEM_CHIP_TRANS = 0b011011;

int ChipTransParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);
int ChipTransParserItemV6(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_CHIP_TRANS_PARSER_ITEM_H
