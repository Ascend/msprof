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

#ifndef ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H
#define ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H

#include <map>
#include <unordered_set>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Application
{

class TaskTraceEvent : public DurationEvent
{
   public:
    TaskTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t modelId,
                   uint32_t streamId, uint32_t taskId, uint32_t batchId, uint32_t contextId, uint64_t connectionId,
                   const std::string taskType)
        : DurationEvent(pid, tid, dur, ts, name),
          modelId_(modelId),
          streamId_(streamId),
          taskId_(taskId),
          batchId_(batchId),
          contextId_(contextId),
          connectionId_(connectionId),
          taskType_(taskType)
    {
    }

   protected:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t modelId_;
    uint32_t streamId_;
    uint32_t taskId_;
    uint32_t batchId_;
    uint32_t contextId_;
    uint64_t connectionId_;
    std::string taskType_;
};

class MemcpyAsyncEvent : public TaskTraceEvent
{
   public:
    MemcpyAsyncEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                     uint32_t modelId, uint32_t streamId, uint32_t taskId, uint32_t batchId, uint32_t contextId,
                     uint64_t connectionId, const std::string taskType, uint64_t dataSize, double bandwidth,
                     std::string memcpyDirection, bool showFlag)
        : TaskTraceEvent(pid, tid, dur, ts, name, modelId, streamId, taskId, batchId, contextId, connectionId,
                         taskType),
          dataSize_(dataSize),
          bandwidth_(bandwidth),
          memcpyDirection_(memcpyDirection),
          showFlag_(showFlag)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint64_t dataSize_;
    double bandwidth_;
    std::string memcpyDirection_;
    bool showFlag_;
};

class SimtTaskEvent : public TaskTraceEvent
{
   public:
    SimtTaskEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t modelId,
                  uint32_t streamId, uint32_t taskId, uint32_t batchId, uint32_t contextId, uint64_t connectionId,
                  const std::string taskType, std::string gridDim, std::string blockDim)
        : TaskTraceEvent(pid, tid, dur, ts, name, modelId, streamId, taskId, batchId, contextId, connectionId,
                         taskType),
          gridDim_(std::move(gridDim)),
          blockDim_(std::move(blockDim))
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    std::string gridDim_;
    std::string blockDim_;
};

class KfcTurnTraceEvent : public DurationEvent
{
   public:
    KfcTurnTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t streamId,
                      uint32_t taskId)
        : DurationEvent(pid, tid, dur, ts, name), streamId_(streamId), taskId_(taskId)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t streamId_;
    uint32_t taskId_;
};

class AscendHardwareAssembler : public JsonAssembler
{
   public:
    AscendHardwareAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateTaskTrace(const std::vector<AscendTaskData> &taskData, const std::string &profPath,
                           const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap);
    void InitData(DataInventory &dataInventory, std::vector<AscendTaskData> &taskData);
    std::string GetOpName(const AscendTaskData &data);
    std::string GetTaskType(const AscendTaskData &data);
    uint32_t GetPhysicStreamId(const uint32_t streamId);
    void GenerateTaskConnectionTrace(const AscendTaskData &data, uint32_t formatPid, TaskId &id);
    void GenerateKfcTrace(const std::vector<KfcTurnData> &kfcData, const std::string &profPath, const LayerInfo &layer,
                          std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateMemcpyAsyncTrace(DataInventory &dataInventory, const std::string &profPath, const LayerInfo &layer,
                                  std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateMemcpyAsyncConnectionTrace(const AscendTaskData &data, uint32_t formatPid);

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::shared_ptr<std::unordered_map<uint32_t, uint32_t>> logicStream_;
    std::map<TaskId, std::string> opName_;
    std::map<TaskId, std::string> taskType_;
    std::map<TaskId, std::pair<std::string, std::string>> simtInfoMap_;
    std::set<std::pair<uint32_t, int>> pidTidSet_;
    std::set<TaskId> ffts_;
    std::set<uint64_t> aclEvent_;
    std::vector<AscendTaskData> memcpyAsyncDeviceTasks_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H
