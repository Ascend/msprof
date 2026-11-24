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

#ifndef ANALYSIS_DOMAIN_SERVICES_L2_CACHE_L2_CACHE_SAMPLE_EVENTS_H
#define ANALYSIS_DOMAIN_SERVICES_L2_CACHE_L2_CACHE_SAMPLE_EVENTS_H

#include <string>
#include <unordered_set>
#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"

namespace Analysis
{
namespace Domain
{
constexpr uint32_t L2_CACHE_MAX_EVENTS = 8;

inline const std::unordered_set<std::string> &L2CacheEventSet()
{
    static const std::unordered_set<std::string> kEvents = {
        "0x59", "0x5b", "0x5c", "0x62", "0x6a", "0x6c", "0x71", "0x74", "0x77", "0x78", "0x79",
        "0x7c", "0x7d", "0x7e", "0xf6", "0xfb", "0xfc", "0x90", "0x91", "0x9c", "0x9d", "0xbf",
        "0x00", "0x88", "0x89", "0x8a", "0x75", "0x97", "0x81", "0x82", "0x83"};
    return kEvents;
}

std::vector<std::string> ParseL2CacheEventsFromSample(const SampleInfo &sampleInfo);
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_L2_CACHE_L2_CACHE_SAMPLE_EVENTS_H
