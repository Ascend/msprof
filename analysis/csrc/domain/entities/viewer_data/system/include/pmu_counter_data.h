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
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DOMAIN_PMU_COUNTER_DATA_H
#define ANALYSIS_DOMAIN_PMU_COUNTER_DATA_H

#include <cstdint>
#include <string>
#include <vector>

namespace Analysis
{
namespace Domain
{
enum class PmuCounterSource : uint8_t
{
    UNKNOWN = 0,
    SYSFS = 1,     // resolved through /sys/bus/event_source/devices/<pmu>/events/<name>
    GENERIC = 2,   // PERF_TYPE_HARDWARE alias
    RAW = 3,       // raw:<config> supplied by the user
    SOFTWARE = 4,  // PERF_TYPE_SOFTWARE kernel counter (page-faults, context-switches, ...)
};

// Values written into HOST_PMU_EVENTS.unit. They are part of the database contract, so a query matches on
// these exact strings.
constexpr char UNIT_NS[] = "ns";
constexpr char UNIT_COUNT[] = "count";
constexpr char UNIT_UNKNOWN[] = "unknown";

struct PmuCounterDef
{
    std::string name;  // as written by the user, also the data file column header
    PmuCounterSource source = PmuCounterSource::UNKNOWN;
    std::string unit = UNIT_UNKNOWN;
};

struct PmuCounterSample
{
    uint64_t timestampNs = 0;
    std::vector<double> values;  // parallel to the bundle's counters, nan = unavailable
    std::vector<uint64_t> rawValues;
    std::vector<uint64_t> timeEnabled;
    std::vector<uint64_t> timeRunning;
};

struct PmuCounterBundle
{
    int32_t pid = -1;  // -1 = all processes
    int32_t cpu = -1;  // -1 = any cpu
    uint64_t intervalNs = 0;
    uint64_t anchorMonoNs = 0;
    uint64_t anchorEpochNs = 0;
    std::vector<PmuCounterDef> counters;
    std::vector<PmuCounterSample> samples;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_PMU_COUNTER_DATA_H
