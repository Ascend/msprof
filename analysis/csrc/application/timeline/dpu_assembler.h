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

#ifndef ANALYSIS_APPLICATION_DPU_ASSEMBLER_H
#define ANALYSIS_APPLICATION_DPU_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"

namespace Analysis
{
namespace Application
{
class DPUAssembler : public JsonAssembler
{
   public:
    DPUAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateMetaData(const std::vector<DPUData> &dpuData, uint32_t pid);
    void GenerateDPUTrace(const std::vector<DPUData> &dpuData, uint32_t pid);

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

class DPUTrackTraceEvent : public DurationEvent
{
   public:
    DPUTrackTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                       uint32_t threadId, uint32_t streamId, uint32_t taskId, const std::string &taskType)
        : DurationEvent(pid, tid, dur, ts, name),
          threadId_(threadId),
          streamId_(streamId),
          taskId_(taskId),
          taskType_(taskType)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t threadId_;
    uint32_t streamId_;
    uint32_t taskId_;
    std::string taskType_;
};

class DPUHcclTraceEvent : public DurationEvent
{
   public:
    DPUHcclTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                      uint32_t threadId, uint32_t streamId, uint32_t taskId, const std::string &opType,
                      uint16_t npuDeviceId, uint32_t aicpuTaskId, const std::string &groupName,
                      const std::string &groupNameId, uint16_t planeId, const std::string &notifyId, uint32_t rankSize,
                      double durationEstimated, uint16_t localRank, uint16_t remoteRank,
                      const std::string &transportType, uint64_t dataSize, double bandwidth,
                      const std::string &dataType, const std::string &linkType, const std::string &rdmaType)
        : DurationEvent(pid, tid, dur, ts, name),
          threadId_(threadId),
          streamId_(streamId),
          taskId_(taskId),
          opType_(opType),
          npuDeviceId_(npuDeviceId),
          aicpuTaskId_(aicpuTaskId),
          groupName_(groupName),
          groupNameId_(groupNameId),
          planeId_(planeId),
          notifyId_(notifyId),
          durationEstimated_(durationEstimated),
          rankSize_(rankSize),
          localRank_(localRank),
          remoteRank_(remoteRank),
          transportType_(transportType),
          dataSize_(dataSize),
          bandwidth_(bandwidth),
          dataType_(dataType),
          linkType_(linkType),
          rdmaType_(rdmaType)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint16_t localRank_;
    uint16_t remoteRank_;
    uint16_t planeId_;
    uint16_t streamId_;
    uint16_t npuDeviceId_;
    uint32_t threadId_;
    uint32_t taskId_;
    uint32_t rankSize_;
    uint32_t aicpuTaskId_;
    uint64_t dataSize_;
    double durationEstimated_;
    double bandwidth_;
    std::string transportType_;
    std::string notifyId_;
    std::string opType_;
    std::string groupName_;
    std::string groupNameId_;
    std::string dataType_;
    std::string linkType_;
    std::string rdmaType_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_DPU_ASSEMBLER_H
