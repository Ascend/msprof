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

#ifndef ANALYSIS_APPLICATION_CCU_ASSEMBLER_H
#define ANALYSIS_APPLICATION_CCU_ASSEMBLER_H

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
class CCUMissionTraceEvent : public DurationEvent
{
   public:
    CCUMissionTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const CCUMissionTimelineData &data)
        : DurationEvent(pid, tid, dur, ts, data.timeType), data_(data)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    CCUMissionTimelineData data_;
};

class CCUAssembler : public JsonAssembler
{
   public:
    CCUAssembler();

   private:
    void GenerateMetaData(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_CCU_ASSEMBLER_H
