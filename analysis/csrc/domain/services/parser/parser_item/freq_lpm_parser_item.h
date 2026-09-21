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

#ifndef MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"

namespace Analysis
{
namespace Domain
{
#define DEFAULT_FREQ_LPM 0x00
#pragma pack(1)

struct FreqLpmData
{
    uint64_t sysCnt;
    uint32_t freq;
    uint32_t resv1;
};
#pragma pack()

#pragma pack(1)
struct FreqData
{
    uint32_t count;
    uint32_t resv1;
    FreqLpmData lpmDataS[FREQ_LPM_DATA_COUNT];
};
#pragma pack()

int FreqLpmParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H
