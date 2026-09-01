/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef MSPROF_ANALYSIS_HAL_LPM_INFO_H
#define MSPROF_ANALYSIS_HAL_LPM_INFO_H

#include <cstdint>
#include <vector>

namespace Analysis
{
namespace Domain
{
constexpr uint32_t LPM_INFO_DATA_COUNT = 55;

enum class LpmInfoType : uint32_t
{
    AIC_FREQ = 0,
    AIC_VOLTAGE = 1,
    BUS_VOLTAGE = 2
};

struct HalLpmValue
{
    uint64_t sysCnt;
    uint32_t value;

    HalLpmValue() : sysCnt(0), value(0) {}
    HalLpmValue(uint64_t sysCntValue, uint32_t dataValue) : sysCnt(sysCntValue), value(dataValue) {}
};

struct HalLpmInfoRecord
{
    uint32_t count;
    LpmInfoType type;
    HalLpmValue lpmDataS[LPM_INFO_DATA_COUNT];

    HalLpmInfoRecord() : count(0), type(LpmInfoType::AIC_FREQ) {}
};

struct HalLpmInfoData
{
    std::vector<HalLpmValue> freqData;
    std::vector<HalLpmValue> aicVoltageData;
    std::vector<HalLpmValue> busVoltageData;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_LPM_INFO_H
