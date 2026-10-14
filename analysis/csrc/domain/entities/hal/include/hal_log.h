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

#ifndef MSPROF_ANALYSIS_HAL_LOG_H
#define MSPROF_ANALYSIS_HAL_LOG_H

#include <array>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"

namespace Analysis
{
namespace Domain
{

enum HalLogType
{
    ACSQ_LOG = 0,
    FFTS_LOG,
    ACC_PMU,
    SOC_PROFILE_LOW_POWER,
    SOC_PROFILE_QOS,
    INVALID_LOG = 5,
};

constexpr uint32_t LOW_POWER_SAMPLE_COUNT = 20;

struct HalLowPowerData
{
    uint64_t sysCnt = 0;
    uint16_t dieId = 0;
    std::array<uint16_t, LOW_POWER_SAMPLE_COUNT> sampleData{{0}};
};

struct HalAcsqLog
{
    bool isEndTimestamp = true;
    uint64_t timestamp = 0;
    uint16_t taskType = 0;
};

struct HalFftsPlusLog
{
    bool isEndTimestamp = true;
    uint64_t timestamp = 0;
    uint16_t subTaskType = 0;
    uint16_t fftsType = 0;
    uint16_t threadId = 0;
};

struct HalAccPmu
{
    uint64_t timestamp = 0;
    uint16_t accId = 0;
    uint64_t bandwidth[2] = {0};
    uint64_t ost[2] = {0};
};

struct HalLogData
{
    HalUniData hd;

    HalLogType type = INVALID_LOG;

    union
    {
        HalAcsqLog acsq;
        HalFftsPlusLog ffts;
        HalAccPmu accPmu;
        HalLowPowerData lowPower;
        HalQosBwData qos;
    };
    HalLogData() {}
};

// StarsSocProfileParser 与 StarsSocParser 均产出 HalLogData，但语义不同（soc profile vs task log）。
// DataInventory 以 typeid 作为 key，同类型仅能注入一份，故用独立容器类型区分两路数据，避免注入冲突。
struct HalSocProfileData
{
    std::vector<HalLogData> items;
    HalSocProfileData() = default;
    explicit HalSocProfileData(std::vector<HalLogData> data) : items(std::move(data)) {}
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_LOG_H
