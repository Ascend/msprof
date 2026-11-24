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

#include "analysis/csrc/domain/services/l2_cache/l2_cache_sample_events.h"

#include <algorithm>
#include <cctype>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
std::vector<std::string> ParseL2CacheEventsFromSample(const SampleInfo &sampleInfo)
{
    const auto &raw = sampleInfo.l2CacheTaskProfilingEventsStr;
    if (raw.empty())
    {
        return {};
    }
    std::vector<std::string> events;
    for (auto event : Utils::Split(raw, ","))
    {
        const auto first = event.find_first_not_of(" \t\r\n");
        const auto last = event.find_last_not_of(" \t\r\n");
        event = first == std::string::npos ? "" : event.substr(first, last - first + 1);
        std::transform(event.begin(), event.end(), event.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
        events.emplace_back(std::move(event));
    }
    if (events.empty())
    {
        return {};
    }
    if (events.size() > L2_CACHE_MAX_EVENTS)
    {
        WARN("Option --L2_cache_events number should less than %.", L2_CACHE_MAX_EVENTS);
        return {};
    }
    for (const auto &event : events)
    {
        if (L2CacheEventSet().count(event) == 0)
        {
            WARN("Option --L2_cache_events value should be in the L2 cache event list");
            return {};
        }
    }
    return events;
}
}  // namespace Domain
}  // namespace Analysis
