/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_APPLICATION_CANN_ASSEMBLER_H
#define ANALYSIS_APPLICATION_CANN_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis
{
namespace Application
{
class CannAssembler : public JsonAssembler
{
   public:
    CannAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

class ApiTraceEvent : public DurationEvent
{
   public:
    ApiTraceEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                  uint32_t threadId, uint64_t connectionId, const std::string &mode, const std::string level,
                  const std::string id, const std::string itemId)
        : DurationEvent(pid, tid, dur, ts, name),
          threadId_(threadId),
          connectionId_(connectionId),
          mode_(mode),
          level_(level),
          id_(id),
          itemId_(itemId)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override
    {
        ostream["Thread Id"] << threadId_;
        ostream["Mode"] << mode_;
        ostream["level"] << level_;
        ostream["id"] << id_;
        ostream["item_id"] << itemId_;
        ostream["connection_id"] << connectionId_;
    }

   private:
    uint32_t threadId_;
    uint64_t connectionId_;
    std::string mode_;
    std::string level_;
    std::string id_;
    std::string itemId_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_CANN_ASSEMBLER_H
