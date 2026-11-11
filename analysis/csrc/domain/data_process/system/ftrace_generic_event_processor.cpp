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
#include "analysis/csrc/domain/data_process/system/ftrace_generic_event_processor.h"

#include <algorithm>
#include <limits>
#include <tuple>
#include <unordered_map>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/services/parser/ftrace/include/ftrace_binary_parser.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{

constexpr uint64_t GLOBAL_TID_SHIFT = 32;

uint64_t MakeGlobalTid(uint32_t tid, uint32_t tgid)
{
    const uint32_t parent = (tgid == 0) ? tid : tgid;
    return (static_cast<uint64_t>(parent) << GLOBAL_TID_SHIFT) | static_cast<uint64_t>(tid);
}
}  // namespace

FtraceGenericEventProcessor::FtraceGenericEventProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool FtraceGenericEventProcessor::Process(DataInventory &dataInventory)
{
    std::vector<FtraceCpuFile> cpuFiles;
    std::string metaPath;
    std::string threadMapPath;
    std::string ftraceDir;
    std::string anchorPath;
    const bool hasCpuFiles =
        FtraceBinaryParser::LocateFiles(profPath_, cpuFiles, metaPath, threadMapPath, ftraceDir, anchorPath);
    if (ftraceDir.empty())
    {
        INFO("No usable ftrace directory under %, skip.", profPath_);
        return true;
    }

    std::vector<GenericEventBundle> bundles(1);
    const auto saveBundle = [this, &bundles, &dataInventory]()
    {
        return SaveToDataInventory<GenericEventBundle>(std::move(bundles), dataInventory,
                                                       PROCESSOR_NAME_FTRACE_GENERIC_EVENT);
    };
    if (!hasCpuFiles)
    {
        WARN("No usable CPU trace files in %.", ftraceDir);
        return saveBundle();
    }
    auto &bundle = bundles.front();
    auto &meta = bundle.meta;
    size_t pageSize = 0;
    if (!FtraceBinaryParser::ParseMeta(metaPath, meta, pageSize))
    {
        WARN("Failed to parse ftrace event meta %, skip ftrace events.", metaPath);
        return saveBundle();
    }

    uint64_t anchorEpochNs = 0;
    uint64_t anchorMonoRawNs = 0;
    if (!FtraceBinaryParser::ParseEpochAnchor(anchorPath, anchorEpochNs, anchorMonoRawNs))
    {
        WARN("No valid ftrace host time anchor %, skip ftrace events.", anchorPath);
        return saveBundle();
    }
    std::unordered_map<uint32_t, uint32_t> tgidOf;
    if (!FtraceBinaryParser::ParseThreadMap(threadMapPath, tgidOf))
    {
        WARN("Failed to parse ftrace thread map %, globalTid falls back to tid.", threadMapPath);
    }

    // field position of common_pid is declared by each type, do not hard code it
    std::unordered_map<uint32_t, uint32_t> pidFieldOf;
    for (const auto &def : meta.types)
    {
        uint32_t fieldIdx = 0;
        for (const auto &field : def.fields)
        {
            if (field.name == "common_pid")
            {
                pidFieldOf[def.typeId] = fieldIdx;
                break;
            }
            fieldIdx++;
        }
    }
    auto &allEvents = bundle.events;
    uint64_t rejectedTimestamps = 0;
    uint64_t firstRejectedTimestamp = 0;
    uint32_t firstRejectedCpu = 0;
    const auto maxTimestamp = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    for (const auto &cpuFile : cpuFiles)
    {
        std::vector<GenericEventData> cpuEvents;
        uint64_t footerEvents = 0;
        const bool incomplete =
            !FtraceBinaryParser::ParseCpuFile(cpuFile.path, cpuFile.cpuId, meta, pageSize, cpuEvents, footerEvents);
        if (incomplete)
        {
            WARN("ftrace cpu % file is incomplete: %.", cpuFile.cpuId, cpuFile.path);
        }

        for (auto &event : cpuEvents)
        {
            if (event.rawTimestamp < anchorMonoRawNs ||
                event.rawTimestamp - anchorMonoRawNs > maxTimestamp - anchorEpochNs)
            {
                if (rejectedTimestamps++ == 0)
                {
                    firstRejectedTimestamp = event.rawTimestamp;
                    firstRejectedCpu = event.cpuId;
                }
                continue;
            }
            event.timestamp = anchorEpochNs + (event.rawTimestamp - anchorMonoRawNs);
            if (incomplete)
            {
                event.flags |= FTRACE_EVENT_FLAG_INCOMPLETE;
            }
            // The position of common_pid comes from the type definition. A type that declares no
            // common_pid must not fall back to a positional guess: fieldIdx 3 is only common_pid
            // because of the standard tracefs prefix, and using it would bake an unrelated value
            // (for example a preempt count) into globalTid.
            const auto pidIt = pidFieldOf.find(event.typeId);
            uint32_t tid = 0;
            if (pidIt != pidFieldOf.end())
            {
                const uint32_t pidFieldIdx = pidIt->second;
                for (const auto &value : event.values)
                {
                    if (value.fieldIdx != pidFieldIdx)
                    {
                        continue;
                    }
                    if (value.kind == GenericFieldKind::UINT_VALUE &&
                        value.uintVal <= std::numeric_limits<uint32_t>::max())
                    {
                        tid = static_cast<uint32_t>(value.uintVal);
                    }
                    else if (value.kind == GenericFieldKind::INT_VALUE && value.intVal > 0 &&
                             value.intVal <= std::numeric_limits<uint32_t>::max())
                    {
                        tid = static_cast<uint32_t>(value.intVal);
                    }
                    break;
                }
            }
            const auto tgid = tgidOf.find(tid);
            event.globalTid = (tid == 0) ? 0 : MakeGlobalTid(tid, tgid != tgidOf.end() ? tgid->second : 0);
            allEvents.emplace_back(std::move(event));
        }
    }
    if (rejectedTimestamps > 0)
    {
        WARN("ftrace skipped % events outside the epoch range (anchor %, first raw timestamp %, cpu %).",
             rejectedTimestamps, anchorMonoRawNs, firstRejectedTimestamp, firstRejectedCpu);
    }
    if (allEvents.empty())
    {
        WARN("No ftrace events parsed from %.", ftraceDir);
    }
    std::stable_sort(allEvents.begin(), allEvents.end(), [](const GenericEventData &left, const GenericEventData &right)
                     { return std::tie(left.timestamp, left.cpuId) < std::tie(right.timestamp, right.cpuId); });
    return saveBundle();
}

}  // namespace Domain
}  // namespace Analysis
