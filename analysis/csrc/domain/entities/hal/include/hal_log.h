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
    SOC_PROFILE_SIO,
    SOC_PROFILE_CHIP_TRANS,
    SOC_PROFILE_INTER_SOC,
    INVALID_LOG = 8,
};

constexpr uint32_t LOW_POWER_SAMPLE_COUNT = 20;
constexpr uint32_t SIO_BANDWIDTH_COUNT = 8;

struct HalSioData
{
    uint64_t sysCnt = 0;
    uint32_t accId = 0;
    std::array<uint32_t, SIO_BANDWIDTH_COUNT> bandwidth{{0}};
};

enum HalChipTransType
{
    CHIP_TRANS_INVALID = 0,
    CHIP_TRANS_V6,   // V6 芯片，落表 PcieInfoV6
    CHIP_TRANS_PA,   // 非 V6 芯片，acc_type == 0b111111，落表 PaLinkInfo
    CHIP_TRANS_PCIE  // 非 V6 芯片，acc_type == 0b100010，落表 PcieInfo
};

struct HalChipTransData
{
    uint64_t sysCnt = 0;
    HalChipTransType transType = CHIP_TRANS_INVALID;
    int32_t id = -1;      // V6: die_id; 非 V6: pa_link_id / pcie_id（取自 event_id）
    uint64_t value1 = 0;  // V6: write_bandwidth; 非 V6: pa rx / pcie write
    uint64_t value2 = 0;  // V6: read_bandwidth; 非 V6: pa tx / pcie read
};

struct HalInterSocData
{
    uint64_t sysCnt = 0;
    uint32_t l2BufferBwLevel = 0;
    uint32_t mataBwLevel = 0;
};

struct HalLowPowerData
{
    uint64_t sysCnt = 0;
    int16_t dieId = -1;
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

// type 标识匿名 union 中的有效成员；默认构造只初始化 hd 和 type，不初始化 union。
// 写入方必须先初始化对应成员再交给下游；读取方必须按 type 分发，只读取对应成员。
// 需要成员默认值时，应显式赋值（如 lowPower = HalLowPowerData{}），仅设置 type 不会初始化成员。
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
        HalSioData sio;
        HalChipTransData chipTrans;
        HalInterSocData interSoc;
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
