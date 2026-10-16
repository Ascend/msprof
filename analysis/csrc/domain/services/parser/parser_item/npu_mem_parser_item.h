/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_NPU_MEM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_NPU_MEM_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
enum NpuMemParserItemType : uint32_t
{
    NPU_MEM_V1_ITEM_TYPE = 0,
    NPU_MEM_V2_ITEM_TYPE = 1
};

int NpuMemV1ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t expandStatus);
int NpuMemV2ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_NPU_MEM_PARSER_ITEM_H
