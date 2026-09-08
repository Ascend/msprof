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

#ifndef MSPROF_ANALYSIS_HAL_PMU_H
#define MSPROF_ANALYSIS_HAL_PMU_H

#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis
{
namespace Domain
{
constexpr int DEFAULT_LENGTH = 8;
enum HalPmuType
{
    PMU = 0,
    BLOCK_PMU,
    INVALID_PMU = 2
};

struct HalPmu
{                                              // 主核context, 从核block
    AcceleratorType acceleratorType{INVALID};  // 加速器类型，即AIC、AIV、MIX_AIC、MIX_AIV
    uint64_t totalCycle = 0;
    std::vector<uint64_t> pmuList;
    uint64_t timeList[2] = {0};  // startTimestamp 和 endTimestamp

    uint8_t ovFlag = 0;
    uint16_t subBlockId = 0;
    uint16_t blockId = 0;
    uint8_t coreType = 0;  // mix时判断是aic还是aiv上报
    uint8_t coreId = 0;
    uint8_t mst = 0;  // mix时判断context pmu是否为主核

    explicit HalPmu() : pmuList(DEFAULT_LENGTH) {}
};

struct HalPmuData
{
    HalUniData hd;
    HalPmuType type{INVALID_PMU};
    HalPmu pmu;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_PMU_H
