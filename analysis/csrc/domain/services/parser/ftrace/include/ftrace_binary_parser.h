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
#ifndef ANALYSIS_DOMAIN_FTRACE_BINARY_PARSER_H
#define ANALYSIS_DOMAIN_FTRACE_BINARY_PARSER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/system/include/ftrace_generic_event_data.h"

namespace Analysis
{
namespace Domain
{
// GENERIC_EVENTS.flags bit1: a field failed the layout/bounds check, so its value columns are
// placeholders. Lets a consumer tell a decoded 0 (for example the idle tid) from a
// zero-filled decode failure.
constexpr uint32_t FTRACE_EVENT_FLAG_FIELD_FAILED = 1U << 1;
constexpr uint32_t FTRACE_EVENT_FLAG_INCOMPLETE = 1U;

struct FtraceCpuFile
{
    std::string path;
    uint32_t cpuId;
};
// Decodes the artifacts produced by the ftrace collector bin:
//   trace_event_meta.json  - event types + field layout (tracefs format offsets)
//   thread_map.json        - tid -> tgid snapshot
//   trace.cpuN.bin         - per-CPU raw tracefs pages + 64B footer
// The CPU of an event comes from the file it was read from: struct trace_entry
// has no cpu field.
class FtraceBinaryParser
{
   public:
    // struct buffer_data_page is { u64 time_stamp; local_t commit; data[] }. Measured on
    // aarch64 5.15: commit == 4096 - 16, so data[] starts at 16; the cacheline attribute pads the
    // page, it does not move the data offset. trace_event_meta.json carries the value actually
    // used by the collector, so MIN/MAX only bound what a meta file may claim.
    static constexpr size_t PAGE_DATA_OFFSET_MIN = 16;
    static constexpr size_t PAGE_DATA_OFFSET_MAX = 256;
    static constexpr size_t FOOTER_SIZE = 64;

    // Resolves the ftrace artifacts and the matching host start_info in the same layout.
    // The input may be a profile root, a flat host layout, or a parent containing one matching dataset.
    static bool LocateFiles(const std::string &profPath, std::vector<FtraceCpuFile> &cpuFiles, std::string &metaPath,
                            std::string &threadMapPath, std::string &ftraceDir, std::string &anchorPath);

    static bool ParseMeta(const std::string &metaPath, GenericEventMeta &meta, size_t &pageSize);
    static bool ParseThreadMap(const std::string &threadMapPath, std::unordered_map<uint32_t, uint32_t> &tgidOf);

    // Reads the epoch anchor (clockMonotonicRaw + collectionTimeBegin) from the
    // profiling start_info, so that trace timestamps can be converted to epoch ns.
    static bool ParseEpochAnchor(const std::string &anchorPath, uint64_t &anchorEpochNs, uint64_t &anchorMonoRawNs);

    // Returns false for an incomplete file or an invalid footer. Successfully decoded events are retained.
    static bool ParseCpuFile(const std::string &cpuFilePath, uint32_t cpuId, const GenericEventMeta &meta,
                             size_t pageSize, std::vector<GenericEventData> &events, uint64_t &footerEvents);
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_FTRACE_BINARY_PARSER_H
