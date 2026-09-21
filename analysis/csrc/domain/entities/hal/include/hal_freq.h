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

#ifndef MSPROF_ANALYSIS_HAL_FREQ_H
#define MSPROF_ANALYSIS_HAL_FREQ_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis
{
namespace Domain
{
constexpr uint32_t FREQ_LPM_DATA_COUNT = 55;

struct HalFreqLpmData
{
    uint64_t sysCnt = 0;
    uint32_t freq = 0;
};

// 用于接收二进制转换后数据, 需要在parser中过滤有效数据
struct HalFreqData
{
    uint64_t count;
    HalFreqLpmData freqLpmDataS[FREQ_LPM_DATA_COUNT];
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_FREQ_H
