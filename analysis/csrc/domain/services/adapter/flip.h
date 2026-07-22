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

#ifndef ANALYSIS_PARSER_ADAPTER_FLIP_H
#define ANALYSIS_PARSER_ADAPTER_FLIP_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/infrastructure/utils/prof_common.h"

namespace Analysis
{
namespace Domain
{
namespace Adapter
{
struct FlipTask
{
    uint16_t deviceId;
    uint32_t streamId;
    uint16_t taskId;
    uint32_t flipNum;
    uint64_t timeStamp;
};

// 该类作用是根据FlipTask计算每个task的batch id
class Flip
{
   public:
    static void ComputeBatchId(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                               std::vector<std::shared_ptr<FlipTask>> &flipTask, bool isV2 = false);
    static uint16_t GetTaskId(const MsprofCompactInfo &task);
    static uint16_t GetBatchId(const MsprofCompactInfo &task);
    static std::shared_ptr<FlipTask> CreateFlipTask(MsprofCompactInfo *taskTrack);

   private:
    using CompactInfoMap = std::unordered_map<uint64_t, std::vector<std::shared_ptr<MsprofCompactInfo>>>;
    using FlipTaskMap = std::unordered_map<uint64_t, std::vector<std::shared_ptr<FlipTask>>>;
    static void CalibrateFlipTaskIdNotZero(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                                           const std::shared_ptr<FlipTask> &flip, uint32_t taskIdx, uint32_t batchId,
                                           bool isV2 = false);
    static CompactInfoMap SepTaskTrack(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack);
    static FlipTaskMap SepFlipTask(const std::vector<std::shared_ptr<FlipTask>> &flipTask);
    static void SetBatchId(MsprofCompactInfo &task, uint32_t batchId);

};  // class Flip
}  // namespace Adapter
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_PARSER_ADAPTER_FLIP_H
