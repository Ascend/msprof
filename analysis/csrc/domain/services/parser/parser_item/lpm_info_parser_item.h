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

#ifndef MSPROF_ANALYSIS_LPM_INFO_PARSER_ITEM_H
#define MSPROF_ANALYSIS_LPM_INFO_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_lpm_info.h"

namespace Analysis
{
namespace Domain
{
constexpr uint32_t DEFAULT_LPM_INFO = 0;

#pragma pack(push, 1)
struct LpmInfoValue
{
    uint64_t sysCnt;
    uint32_t value;
    uint32_t reserved;
};

struct LpmInfoRawData
{
    uint32_t count;
    uint32_t type;
    LpmInfoValue lpmDataS[LPM_INFO_DATA_COUNT];
};
#pragma pack(pop)

static_assert(sizeof(LpmInfoRawData) == 888, "The size of LpmInfoRawData must be 888 bytes");

int LpmInfoParseItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_LPM_INFO_PARSER_ITEM_H
