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

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_NUM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_NUM_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
#define PARSER_ITEM_BLOCK_NUM 0x0f

#pragma pack(1)
struct BlockNum
{
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t taskId;
    uint32_t blockNum;
    uint8_t resv6[16];
};
#pragma pack()

int BlockNumParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
int BlockNumParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_NUM_PARSER_ITEM_H
