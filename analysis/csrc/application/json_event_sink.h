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

#ifndef ANALYSIS_APPLICATION_JSON_EVENT_SINK_H
#define ANALYSIS_APPLICATION_JSON_EVENT_SINK_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

namespace Analysis
{
namespace Application
{
// Assemblers push assembled trace events here. File names, JSON brackets and
// the writer threads live in DeliverableConsumer.
class JsonEventSink
{
   public:
    void Open(const std::unordered_map<std::string, Infra::FileCategory> &files, const std::string &profPath);
    void push_back(std::shared_ptr<Domain::TraceEvent> event);
    void emplace_back(std::shared_ptr<Domain::TraceEvent> event);
    // True when this assembler has not produced any event, including events already spilled.
    bool empty() const { return accepted_ == 0; }
    void Flush();
    // Drop pending and spilled events. Nothing from this sink is published after Discard,
    // unless the spill file was unusable and the sink already degraded to direct publishing.
    void Discard();
    // Block until this sink's own JSON files have consumed their queues.
    void Sync();

   private:
    bool AppendSpill(const std::string &body);
    void PublishDirect(const std::string &body);
    void ReplaySpill();
    void DeleteSpill();

    std::vector<std::string> prefixes_;
    std::string profPath_;
    std::vector<std::shared_ptr<Domain::TraceEvent>> pending_;
    std::string spillPath_;
    uint64_t spillRecords_ = 0;
    bool spillBroken_ = false;
    // Set once the spill file cannot be used. Later batches go straight to the consumer so a
    // broken spill never turns into unbounded memory.
    bool spillDisabled_ = false;
    size_t accepted_ = 0;
    bool open_ = false;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_JSON_EVENT_SINK_H
