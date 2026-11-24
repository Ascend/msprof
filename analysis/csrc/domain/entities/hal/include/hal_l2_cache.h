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

#ifndef MSPROF_ANALYSIS_HAL_L2_CACHE_H
#define MSPROF_ANALYSIS_HAL_L2_CACHE_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
constexpr uint32_t L2_CACHE_RECORD_SIZE = 72;
constexpr uint32_t L2_CACHE_EVENT_SLOT_COUNT = 8;

#pragma pack(push, 1)
struct HalL2CacheRaw
{
    uint16_t taskType;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t reserve;
    uint64_t events[L2_CACHE_EVENT_SLOT_COUNT];
};
#pragma pack(pop)

static_assert(sizeof(HalL2CacheRaw) == L2_CACHE_RECORD_SIZE,
              "HalL2CacheRaw must match the fixed 72-byte L2 cache record size");

}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_L2_CACHE_H
