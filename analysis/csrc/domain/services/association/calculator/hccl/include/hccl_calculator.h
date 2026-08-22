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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H

#include <limits>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;

struct DeviceHcclOp
{
    uint32_t iterId;
    int32_t relay;
    int32_t retry;
    int32_t indexId;
    int64_t rankSize;
    uint32_t threadId;
    int64_t connectionId;
    uint64_t modelId;
    uint64_t count;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string dataType;
    std::string algType;
    std::string groupName;
    double start;
    double end;
};

struct DeviceHcclTask
{
    uint16_t isMaster = 0;
    int32_t indexId = 0;
    int32_t planeId = 0;
    uint32_t batchId = 0;
    uint32_t taskId = 0;
    uint32_t iterId = 0;
    uint32_t streamId = 0;
    int64_t localRank = -1;
    int64_t remoteRank = -1;
    uint32_t contextId = 0;
    uint32_t threadId = 0;
    int64_t rankSize = -1;
    int64_t opId = 0;
    uint64_t modelId = 0;
    double size = 0.0;
    double bandwidth = 0.0;
    double timestamp = 0;
    double duration = 0;
    std::string dataType;
    std::string linkType;
    std::string transportType;
    std::string taskType;
    std::string hcclName;
    std::string opName;
    std::string groupName;
    std::string notifyId;
    std::string rdmaType;
};

struct HcclStatistics
{
    std::string opType;
    uint32_t count = 0;
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
};

class HcclCalculator : public Process
{
   private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
    bool GetHcclData(DataInventory& dataInventory);
    bool MergeHcclTaskData(const std::shared_ptr<std::vector<TopDownTask>>& ascendTasks,
                           const std::shared_ptr<std::vector<HcclTask>>& hcclTasks,
                           std::vector<DeviceHcclTask>& deviceHcclTasks);
    DeviceHcclTask InitHcclTaskData(const TopDownTask& topDownTask, const HcclTask& hcclTask);
    void MergeOpDataByThreadId(std::vector<HcclOp>& hcclOps, std::vector<DeviceHcclTask>& hcclTasks);
    bool MergeHcclOpData(const std::shared_ptr<std::vector<HcclOp>>& hcclOps,
                         const std::vector<DeviceHcclTask>& deviceHcclTasks);
    DeviceHcclTask GetCompleteHcclTaskData(const HcclOp& op, const DeviceHcclTask& hcclTask, uint32_t count);
    DeviceHcclOp GetCompleteHcclOpData(const HcclOp& op, double groupStart, double groupEnd, int64_t rankSize,
                                       uint32_t iterId);
    void UpdateHcclOpNameByGroupName(uint64_t clockMonotonicRaw);
    void UpdateHcclBandwidth();
    void CalculateTaskBandwidth(std::vector<DeviceHcclTask*> hcclTasks);
    uint16_t GetJumpNum(const DeviceHcclTask& task);
    double CalculateBandwidth(double size, double duration);
    uint16_t FindConsecutivePayloadTask(std::vector<DeviceHcclTask*> tasks, size_t idx);
    bool GetHcclStatisticsData(uint64_t clockMonotonicRaw, SampleInfo sampleInfo);
    bool InjectData(DataInventory& inventory);

   private:
    std::vector<DeviceHcclOp> opData_;
    std::vector<DeviceHcclTask> taskData_;
    std::vector<HcclStatistics> statisticsData_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H
