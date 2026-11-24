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
#include "analysis/csrc/domain/data_process/system/host_pmu_counter_processor.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/services/parser/host_pmu/include/host_pmu_counter_parser.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
// Converts a CLOCK_MONOTONIC row timestamp to epoch ns. The ftrace anchor cannot be reused: it is
// CLOCK_MONOTONIC_RAW, and the offset between the two clocks exceeds one sampling interval.
bool ToEpochNs(const PmuCounterBundle &bundle, uint64_t rawNs, uint64_t &epochNs)
{
    if (bundle.anchorMonoNs == 0 || bundle.anchorEpochNs == 0)
    {
        return false;
    }
    if (rawNs >= bundle.anchorMonoNs)
    {
        return Utils::AddWithoutOverflow(bundle.anchorEpochNs, rawNs - bundle.anchorMonoNs, epochNs);
    }
    // A row older than the anchor would need subtraction; guard against underflow and reject it.
    const uint64_t delta = bundle.anchorMonoNs - rawNs;
    if (delta > bundle.anchorEpochNs)
    {
        return false;
    }
    epochNs = bundle.anchorEpochNs - delta;
    return true;
}
}  // namespace

HostPmuCounterProcessor::HostPmuCounterProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool HostPmuCounterProcessor::Process(DataInventory &dataInventory)
{
    const std::vector<std::string> files = HostPmuCounterParser::LocateFiles(profPath_);
    if (files.empty())
    {
        // Normal for a run that did not enable PMU collection; not an error.
        INFO("No host PMU counter file under %, skip.", profPath_);
        return true;
    }

    std::vector<PmuCounterBundle> bundles;
    bundles.reserve(files.size());
    for (const auto &path : files)
    {
        PmuCounterBundle bundle;
        std::string errorMsg;
        if (!HostPmuCounterParser::ParseFile(path, bundle, errorMsg))
        {
            ERROR("Parse host PMU file failed: %.", errorMsg);
            continue;
        }
        if (bundle.anchorMonoNs == 0 || bundle.anchorEpochNs == 0)
        {
            WARN("Host PMU file % has no clock anchor, skipping its samples.", path);
            continue;
        }
        std::vector<PmuCounterSample> converted;
        converted.reserve(bundle.samples.size());
        size_t dropped = 0;
        for (auto &sample : bundle.samples)
        {
            const uint64_t rawNs = sample.timestampNs;
            uint64_t epochNs = 0;
            if (!ToEpochNs(bundle, rawNs, epochNs))
            {
                // Older than the anchor: cannot be placed on the timeline, drop just this row.
                ++dropped;
                continue;
            }
            sample.timestampNs = epochNs;
            converted.push_back(std::move(sample));
        }
        bundle.samples = std::move(converted);
        if (dropped > 0)
        {
            WARN("Host PMU file % dropped % row(s) older than the clock anchor.", path, dropped);
        }
        if (bundle.samples.empty())
        {
            // Keep the bundle: a counter with no samples still records that the CPU was collected,
            // which is different from "never requested".
            WARN("Host PMU file % has no usable sample; its counters are kept without samples.", path);
        }
        bundles.push_back(std::move(bundle));
    }

    if (bundles.empty())
    {
        WARN("Host PMU files found under % but none parsed, skip.", profPath_);
        return true;
    }
    if (!SaveToDataInventory(std::move(bundles), dataInventory, PROCESSOR_NAME_HOST_PMU_COUNTER))
    {
        ERROR("Save host PMU counter data to inventory failed.");
        return false;
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
