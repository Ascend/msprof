/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_NPU_MODULE_MEM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_NPU_MODULE_MEM_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
constexpr uint32_t NPU_MODULE_MEM_ITEM_TYPE = 0;
int NpuModuleMemParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_NPU_MODULE_MEM_PARSER_ITEM_H
