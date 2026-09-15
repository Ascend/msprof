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

#ifndef ANALYSIS_DOMAIN_ENTITIES_HAL_INCLUDE_HAL_LLC_PCIE_H
#define ANALYSIS_DOMAIN_ENTITIES_HAL_INCLUDE_HAL_LLC_PCIE_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
struct HalLlcData
{
    uint32_t deviceId;
    uint64_t timestamp;
    uint64_t count;
    uint32_t event;
    uint32_t l3tid;
};

struct HalPcieData
{
    uint64_t timestamp;
    uint32_t deviceId;
    uint32_t txPBandwidthMin;
    uint32_t txPBandwidthMax;
    uint32_t txPBandwidthAvg;
    uint32_t txNpBandwidthMin;
    uint32_t txNpBandwidthMax;
    uint32_t txNpBandwidthAvg;
    uint32_t txCplBandwidthMin;
    uint32_t txCplBandwidthMax;
    uint32_t txCplBandwidthAvg;
    uint32_t txNpLatencyMin;
    uint32_t txNpLatencyMax;
    uint32_t txNpLatencyAvg;
    uint32_t rxPBandwidthMin;
    uint32_t rxPBandwidthMax;
    uint32_t rxPBandwidthAvg;
    uint32_t rxNpBandwidthMin;
    uint32_t rxNpBandwidthMax;
    uint32_t rxNpBandwidthAvg;
    uint32_t rxCplBandwidthMin;
    uint32_t rxCplBandwidthMax;
    uint32_t rxCplBandwidthAvg;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_ENTITIES_HAL_INCLUDE_HAL_LLC_PCIE_H
