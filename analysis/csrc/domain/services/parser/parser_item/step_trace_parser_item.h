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

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{

#define PARSER_ITEM_STEP_TRACE 0x0A

#pragma pack(1)
struct StepTrace
{
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t tagId;
    uint16_t resv4;
};

struct StepTraceV6
{
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t tagId;
    uint16_t streamId;
    uint32_t taskId;
};
#pragma pack()

static_assert(sizeof(StepTrace) == 40, "Legacy StepTrace record size must be 40 bytes");
static_assert(sizeof(StepTraceV6) == 40, "V6 StepTrace record size must be 40 bytes");

int StepTraceParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
int StepTraceParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H
