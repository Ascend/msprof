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
#include "analysis/csrc/domain/services/parser/host_pmu/include/host_pmu_counter_parser.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
using Utils::Trim;

// File layout: <prof>/host/data/pmu/host_pmu_<pid|all>.data, plus a .timing sidecar holding one line
// per valid reading: "<timestamp>,<eventIndex>,<rawValue>,<timeEnabled>,<timeRunning>".
const char *const PMU_FILE_PREFIX = "host_pmu_";
const char *const PMU_FILE_SUFFIX = ".data";
const char *const PMU_TIMING_SUFFIX = ".timing";
const char *const PMU_HOST_SUB_DIR = "host/data/pmu";
const char *const PMU_FLAT_SUB_DIR = "data/pmu";

// Info line: "#msprof_pmu_v1 #pid=<pid> cpu=<cpu> intervalNs=<ns> anchorMonoNs=<ns> anchorEpochNs=<ns>".
const char *const PMU_HEADER_PREFIX = "#msprof_pmu_v1";
const char *const PMU_PID_TAG = "#pid=";
const char *const PMU_CPU_TAG = " cpu=";
const char *const PMU_INTERVAL_TAG = " intervalNs=";
const char *const PMU_ANCHOR_MONO_TAG = " anchorMonoNs=";
const char *const PMU_ANCHOR_EPOCH_TAG = " anchorEpochNs=";

// A read that found the fd gone, or a counter the kernel reset because too many events shared one
// hardware counter, is written as nan. It has to survive as "unknown", not become 0.
const char *const PMU_VALUE_MISSING = "nan";

std::string UnitOf(const std::string &name, PmuCounterSource source)
{
    if (source == PmuCounterSource::SOFTWARE)
    {
        return (name == "task-clock" || name == "cpu-clock") ? UNIT_NS : UNIT_COUNT;
    }
    if (source == PmuCounterSource::GENERIC || source == PmuCounterSource::SYSFS)
    {
        return UNIT_COUNT;
    }
    return UNIT_UNKNOWN;
}
}  // namespace

std::vector<std::string> HostPmuCounterParser::LocateFiles(const std::string &profPath)
{
    // Same layout as ftrace: host counters live under <PROF>/host/data/pmu. The flat
    // <PROF>/data/pmu form is still accepted so datasets written by an older collector keep parsing.
    std::string dirPath = Utils::File::PathJoin({profPath, PMU_HOST_SUB_DIR});
    if (!Utils::File::CheckDir(dirPath, false))
    {
        dirPath = Utils::File::PathJoin({profPath, PMU_FLAT_SUB_DIR});
        if (!Utils::File::CheckDir(dirPath, false))
        {
            // A run that did not enable PMU collection writes no such directory at all; that is normal.
            return std::vector<std::string>();
        }
    }
    std::vector<std::string> files = Utils::File::GetFilesWithPrefix(dirPath, PMU_FILE_PREFIX);
    // The prefix also matches host_pmu_status.txt and the .timing sidecars, so keep only data files.
    files.erase(std::remove_if(files.begin(), files.end(),
                               [](const std::string &file) { return !Utils::EndsWith(file, PMU_FILE_SUFFIX); }),
                files.end());
    // Stable order keeps a multi-pid run's files comparable across runs.
    std::sort(files.begin(), files.end());
    return files;
}

bool HostPmuCounterParser::ParseFile(const std::string &path, PmuCounterBundle &bundle, std::string &errorMsg)
{
    std::vector<std::string> lines;
    Utils::FileReader reader(path);
    if (!reader.IsOpen() || reader.ReadText(lines) != ANALYSIS_OK)
    {
        errorMsg = "cannot read " + path;
        return false;
    }
    if (lines.empty())
    {
        errorMsg = path + ": empty file";
        return false;
    }
    // Exact match, not a prefix match: "#msprof_pmu_v10" would satisfy a prefix test against
    // "#msprof_pmu_v1" and then be parsed with the wrong column meaning.
    if (Trim(lines[0]) != PMU_HEADER_PREFIX)
    {
        errorMsg = path + ": unsupported header '" + Trim(lines[0]) + "'";
        return false;
    }
    if (lines.size() < 2)
    {
        errorMsg = path + ": missing info line";
        return false;
    }
    const std::string info = Trim(lines[1]);
    const size_t pidPos = info.find(PMU_PID_TAG);
    const size_t cpuPos = info.find(PMU_CPU_TAG);
    const size_t intervalPos = info.find(PMU_INTERVAL_TAG);
    if (pidPos == std::string::npos || cpuPos == std::string::npos || intervalPos == std::string::npos)
    {
        errorMsg = path + ": malformed info line '" + info + "'";
        return false;
    }
    // -1 is a legal value ("all processes" / "any cpu"), so a failed parse is reported instead of being
    // returned as the sentinel: a malformed info line must be rejected, not read as "collect everything".
    if (Utils::StrToInt32(bundle.pid, info.substr(pidPos + std::strlen(PMU_PID_TAG),
                                                  cpuPos - pidPos - std::strlen(PMU_PID_TAG))) != ANALYSIS_OK ||
        Utils::StrToInt32(bundle.cpu, info.substr(cpuPos + std::strlen(PMU_CPU_TAG),
                                                  intervalPos - cpuPos - std::strlen(PMU_CPU_TAG))) != ANALYSIS_OK)
    {
        errorMsg = path + ": malformed pid/cpu in info line '" + info + "'";
        return false;
    }
    // The interval field ends either at the anchor pair or at end of line for older collectors.
    const size_t anchorMonoPos = info.find(PMU_ANCHOR_MONO_TAG);
    const size_t intervalEnd = (anchorMonoPos == std::string::npos) ? info.size() : anchorMonoPos;
    if (Utils::StrToU64(bundle.intervalNs, info.substr(intervalPos + std::strlen(PMU_INTERVAL_TAG),
                                                       intervalEnd - intervalPos - std::strlen(PMU_INTERVAL_TAG))) !=
        ANALYSIS_OK)
    {
        errorMsg = path + ": malformed interval in info line '" + info + "'";
        return false;
    }
    if (anchorMonoPos != std::string::npos)
    {
        const size_t anchorEpochPos = info.find(PMU_ANCHOR_EPOCH_TAG);
        if (anchorEpochPos == std::string::npos)
        {
            errorMsg = path + ": missing epoch anchor in info line '" + info + "'";
            return false;
        }
        if (Utils::StrToU64(bundle.anchorMonoNs, info.substr(anchorMonoPos + std::strlen(PMU_ANCHOR_MONO_TAG),
                                                             anchorEpochPos - anchorMonoPos -
                                                                 std::strlen(PMU_ANCHOR_MONO_TAG))) != ANALYSIS_OK ||
            Utils::StrToU64(bundle.anchorEpochNs, info.substr(anchorEpochPos + std::strlen(PMU_ANCHOR_EPOCH_TAG))) !=
                ANALYSIS_OK)
        {
            errorMsg = path + ": malformed clock anchor in info line '" + info + "'";
            return false;
        }
    }

    if (lines.size() < 3)
    {
        errorMsg = path + ": missing column header";
        return false;
    }
    const std::vector<std::string> columns = Utils::Split(Trim(lines[2]), ",");
    // Column 0 is the timestamp, the rest are counters.
    if (columns.size() < 2)
    {
        errorMsg = path + ": column header has no counters";
        return false;
    }
    bundle.counters.clear();
    for (size_t i = 1; i < columns.size(); ++i)
    {
        const std::string column = Trim(columns[i]);
        PmuCounterDef def;
        // The collector writes "<name>/<source>". Older collectors wrote a bare name, so a missing
        // slash yields source=unknown rather than being treated as an error.
        const size_t slash = column.rfind('/');
        if (slash == std::string::npos)
        {
            def.name = column;
            def.source = PmuCounterSource::UNKNOWN;
        }
        else
        {
            def.name = column.substr(0, slash);
            const std::string source = column.substr(slash + 1);
            if (source == "sysfs")
            {
                def.source = PmuCounterSource::SYSFS;
            }
            else if (source == "generic")
            {
                def.source = PmuCounterSource::GENERIC;
            }
            else if (source == "raw")
            {
                def.source = PmuCounterSource::RAW;
            }
            else if (source == "software")
            {
                // A kernel counter (page-faults, ...), never interchangeable with a PMU event: mixing
                // the two in one ratio produces a meaningless number.
                def.source = PmuCounterSource::SOFTWARE;
            }
            else
            {
                def.source = PmuCounterSource::UNKNOWN;
            }
        }
        def.unit = UnitOf(def.name, def.source);
        bundle.counters.push_back(def);
    }

    // Rows start at the fourth line; lineNo stays 1-based for the error messages below.
    for (size_t index = 3; index < lines.size(); ++index)
    {
        const size_t lineNo = index + 1;
        const std::string row = Trim(lines[index]);
        if (row.empty())
        {
            continue;
        }
        const std::vector<std::string> fields = Utils::Split(row, ",");
        if (fields.size() != columns.size())
        {
            // Reject the whole file, not the row: a short row means this reader and the writer
            // disagree about the format.
            std::ostringstream oss;
            oss << path << ":" << lineNo << ": expected " << columns.size() << " columns, got " << fields.size();
            errorMsg = oss.str();
            return false;
        }
        PmuCounterSample sample;
        if (Utils::StrToU64(sample.timestampNs, Trim(fields[0])) != ANALYSIS_OK)
        {
            std::ostringstream oss;
            oss << path << ":" << lineNo << ": bad timestamp '" << fields[0] << "'";
            errorMsg = oss.str();
            return false;
        }
        sample.values.reserve(fields.size() - 1);
        for (size_t i = 1; i < fields.size(); ++i)
        {
            const std::string value = Trim(fields[i]);
            if (value == PMU_VALUE_MISSING)
            {
                sample.values.push_back(std::numeric_limits<double>::quiet_NaN());
                continue;
            }
            double parsed = 0.0;
            if (Utils::StrToDouble(parsed, value) != ANALYSIS_OK)
            {
                std::ostringstream oss;
                oss << path << ":" << lineNo << ": bad value '" << value << "'";
                errorMsg = oss.str();
                return false;
            }
            sample.values.push_back(parsed);
        }
        bundle.samples.push_back(std::move(sample));
    }

    // Timing sidecar; absent in datasets from older collectors, which then keep only the values.
    const std::string timingPath = path + PMU_TIMING_SUFFIX;
    std::vector<std::string> timingLines;
    bool hasTiming = false;
    if (Utils::File::Exist(timingPath))
    {
        Utils::FileReader timingReader(timingPath);
        hasTiming = timingReader.IsOpen() && timingReader.ReadText(timingLines) == ANALYSIS_OK;
    }
    if (hasTiming)
    {
        // The sidecar and the data rows are both written in timestamp order, but keying by timestamp
        // keeps this correct for any order and turns the merge from O(timing lines x samples) into
        // O(timing lines): with 4 counters over 10 minutes the quadratic form runs 1.4e8 comparisons.
        std::unordered_map<uint64_t, size_t> sampleByTimestamp;
        sampleByTimestamp.reserve(bundle.samples.size());
        for (size_t i = 0; i < bundle.samples.size(); ++i)
        {
            sampleByTimestamp.emplace(bundle.samples[i].timestampNs, i);
        }
        for (const auto &timingLine : timingLines)
        {
            const std::vector<std::string> fields = Utils::Split(Trim(timingLine), ",");
            if (fields.size() != 5)
            {
                continue;
            }
            uint64_t rawTimestamp = 0;
            uint64_t index = 0;
            uint64_t rawValue = 0;
            uint64_t timeEnabled = 0;
            uint64_t timeRunning = 0;
            if (Utils::StrToU64(rawTimestamp, Trim(fields[0])) != ANALYSIS_OK ||
                Utils::StrToU64(index, Trim(fields[1])) != ANALYSIS_OK ||
                Utils::StrToU64(rawValue, Trim(fields[2])) != ANALYSIS_OK ||
                Utils::StrToU64(timeEnabled, Trim(fields[3])) != ANALYSIS_OK ||
                Utils::StrToU64(timeRunning, Trim(fields[4])) != ANALYSIS_OK)
            {
                continue;
            }
            const auto hit = sampleByTimestamp.find(rawTimestamp);
            if (hit == sampleByTimestamp.end())
            {
                continue;
            }
            auto &sample = bundle.samples[hit->second];
            sample.rawValues.resize(sample.values.size(), 0);
            sample.timeEnabled.resize(sample.values.size(), 0);
            sample.timeRunning.resize(sample.values.size(), 0);
            if (index < sample.values.size())
            {
                sample.rawValues[index] = rawValue;
                sample.timeEnabled[index] = timeEnabled;
                sample.timeRunning[index] = timeRunning;
            }
        }
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
