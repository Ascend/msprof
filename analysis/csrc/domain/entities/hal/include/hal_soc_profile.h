/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_HAL_SOC_PROFILE_H
#define MSPROF_ANALYSIS_HAL_SOC_PROFILE_H

#include <array>
#include <cstdint>

namespace Analysis
{
namespace Domain
{

constexpr uint32_t LOW_POWER_SAMPLE_COUNT = 20;

struct HalLowPowerData
{
    uint64_t sysCnt = 0;
    uint16_t dieId = 0;
    std::array<uint16_t, LOW_POWER_SAMPLE_COUNT> sampleData{{0}};
};

enum HalSocProfileType
{
    SOC_PROFILE_INVALID = 0,
    SOC_PROFILE_LOW_POWER
};

struct HalSocProfileData
{
    HalSocProfileType type = SOC_PROFILE_INVALID;
    HalLowPowerData lowPower;
};

}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_SOC_PROFILE_H
