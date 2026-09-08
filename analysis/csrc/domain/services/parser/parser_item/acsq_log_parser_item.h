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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"

namespace Analysis
{
namespace Domain
{

const int PARSER_ITEM_ACSQ_LOG_START = 0b000000;
const int PARSER_ITEM_ACSQ_LOG_END = 0b000001;
// V6 Topic blk记录, 仅解析不落盘
const int PARSER_ITEM_TOPIC_BLK_START_V6 = 0b000010;
const int PARSER_ITEM_TOPIC_BLK_END_V6 = 0b000011;
// V6 Block log记录, flush已禁用不落盘
const int PARSER_ITEM_BLOCK_LOG_START_V6 = 0b100100;
const int PARSER_ITEM_BLOCK_LOG_END_V6 = 0b100101;

#pragma pack(1)
struct AcsqLog
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t resv4;
    uint16_t resv5;
    uint32_t resv6[11];
};
struct AcsqLogV6
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint32_t taskId;
    uint64_t timestamp;
    uint16_t resv4;
    uint16_t resv5;
    uint32_t resv6[3];
};
struct BlockLogV6
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv[15];
};
#pragma pack()

int AcsqLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
int AcsqLogParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
int BlockLogParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H
