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

#ifndef ANALYSIS_BLOCK_DETAIL_ASSEMBLER_H
#define ANALYSIS_BLOCK_DETAIL_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/block_detail_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Application
{
class BlockDetailEvent : public DurationEvent
{
   public:
    BlockDetailEvent(uint32_t pid, uint32_t tid, double dur, const std::string &ts, const std::string &name,
                     uint32_t streamId, uint32_t taskId, uint32_t batchId, uint32_t subTaskId, uint32_t coreId)
        : DurationEvent(pid, tid, dur, ts, name),
          streamId_(streamId),
          taskId_(taskId),
          batchId_(batchId),
          subTaskId_(subTaskId),
          coreId_(coreId)
    {
    }

   protected:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t streamId_;
    uint32_t taskId_;
    uint32_t batchId_;
    uint32_t subTaskId_;
    uint64_t coreId_;
};

struct PmuTimelineRow
{
    uint32_t pid;
    uint32_t tid;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t batchId;
    uint32_t subtaskId;
    uint32_t coreId;
    uint64_t timestamp;
    uint64_t duration;
    std::string name;
};

class BlockDetailAssembler : public JsonAssembler
{
   public:
    BlockDetailAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, const std::string &profPath) override;
    void FormatTaskInfoData(const std::vector<TaskInfoData> &taskInfoData);
    std::unordered_map<uint16_t, uint32_t> GenerateBlockDetailTrace(
        std::vector<AscendTaskData> &ascendTaskData,
        std::unordered_map<uint32_t, std::vector<BlockDetailData>> &blockDetailDataMap, uint32_t sortIndex,
        const std::string &profPath, JsonEventSink &res);
    std::vector<const BlockDetailData *> GetBlockPmuGroup(const std::vector<BlockDetailData> &pmuDataList,
                                                          const AscendTaskData &taskData);
    void SetPmuResultList(const std::vector<const BlockDetailData *> &datas, uint32_t pid,
                          std::vector<PmuTimelineRow> &block_pmu_timeline_data, const std::string &op_name,
                          JsonEventSink &res);
    void GenerateBlockPmuTimelineData(const std::vector<PmuTimelineRow> &block_pmu_timeline_data, JsonEventSink &res);

   private:
    std::map<TaskId, std::string> formatedTaskInfo_;
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_BLOCK_DETAIL_ASSEMBLER_H
