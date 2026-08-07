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
#ifndef ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H
#define ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/overlap_analysis_data.h"

namespace Analysis
{
namespace Application
{
class OverlapAnalysisAssembler : public JsonAssembler
{
   public:
    OverlapAnalysisAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    std::vector<std::shared_ptr<TraceEvent>> GenerateOverlapEvents(const std::vector<OverlapAnalysisData> &events);
    std::vector<std::shared_ptr<TraceEvent>> GenerateMetaData(uint16_t deviceId);

   private:
    std::unordered_map<uint16_t, uint32_t> pidMap_;
};
class OverlapEvent : public DurationEvent
{
   public:
    OverlapEvent(int pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                 OverlapAnalysisType type)
        : DurationEvent(pid, tid, dur, ts, name), type_(type)
    {
    }

   private:
    OverlapAnalysisType type_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H
