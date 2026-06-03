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
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"

#include <algorithm>
#include <cctype>
#include <tuple>
#include <utility>

#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
namespace
{
struct GroupData
{
    uint64_t firstTimestamp = 0;
    int64_t count = -1;
};

struct OpTypeInfo
{
    OpTypeInfo() = default;
    OpTypeInfo(double max, double min, std::string opType) : max(max), min(min), opType(std::move(opType)) {}
    double max = 0;
    double min = std::numeric_limits<double>::infinity();
    std::string opType;
};

const uint16_t RDMA_NO_BARRIER_TASK_NUM = 3;
const uint16_t RDMA_WITH_BARRIER_TASK_NUM = 5;
const uint16_t PERCENTAGE = 100;
const std::string RDMA_SEND_PAYLOAD = "RDMA_SEND_PAYLOAD";
const std::string NA = "N/A";
const int POS_COMPARE_BASE = 3;
const std::string AICPU_KERNEL = "AicpuKernel";
const std::string NORMAL = "Normal";
}  // namespace

uint32_t HcclCalculator::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    INFO("Start Hccl calculator ProcessEntry.");
    if (!GetHcclData(dataInventory))
    {
        ERROR("Failed to Get hccl task data or op data.");
        return ANALYSIS_ERROR;
    }

    // 前面多线程数据处理 此处的task可能不保序 重新排序
    std::sort(
        taskData_.begin(), taskData_.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2)
        { return std::tie(task1.hostTimestamp, task1.timestamp) < std::tie(task2.hostTimestamp, task2.timestamp); });

    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    DeviceStartInfo startInfo;
    deviceContext.Getter(startInfo);

    UpdateHcclOpNameByGroupName(startInfo.clockMonotonicRaw);
    UpdateHcclBandwidth();
    if (!GetHcclStatisticsData(startInfo.clockMonotonicRaw))
    {
        ERROR("Failed to Get hccl statistics data.");
        return ANALYSIS_ERROR;
    }
    if (!InjectData(dataInventory))
    {
        ERROR("Failed to inject hccl data.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

bool HcclCalculator::GetHcclData(DataInventory& dataInventory)
{
    INFO("Start Hccl calculator GetHcclData.");
    auto ascendTasks = dataInventory.GetPtr<std::vector<TopDownTask>>();
    auto hcclTasks = dataInventory.GetPtr<std::vector<HcclTask>>();
    if (ascendTasks == nullptr || hcclTasks == nullptr)
    {
        ERROR("Ori ascend task data pointer or ori hccl task data pointer is nullptr.");
        return false;
    }
    std::vector<DeviceHcclTask> deviceHcclTasks;
    if (!MergeHcclTaskData(ascendTasks, hcclTasks, deviceHcclTasks))
    {
        ERROR("Merge hccl task and ascend task failed.");
        return false;
    }

    auto hcclOps = dataInventory.GetPtr<std::vector<HcclOp>>();
    if (hcclOps == nullptr)
    {
        ERROR("Ori hccl op data pointer is nullptr.");
        return false;
    }
    if (!MergeHcclOpData(hcclOps, deviceHcclTasks))
    {
        ERROR("Merge hccl op failed.");
        return false;
    }
    return true;
}

bool HcclCalculator::MergeHcclTaskData(const std::shared_ptr<std::vector<TopDownTask>>& ascendTasks,
                                       const std::shared_ptr<std::vector<HcclTask>>& hcclTasks,
                                       std::vector<DeviceHcclTask>& deviceHcclTasks)
{
    INFO("Start merge hccl task and ascend task.");
    std::sort(ascendTasks->begin(), ascendTasks->end(),
              [](const TopDownTask& task1, const TopDownTask& task2) { return task1.startTime < task2.startTime; });
    std::sort(hcclTasks->begin(), hcclTasks->end(),
              [](const HcclTask& task1, const HcclTask& task2) { return task1.timestamp < task2.timestamp; });
    std::map<TaskId, std::vector<TopDownTask>> taskTable;
    for (const auto& task : *ascendTasks)
    {
        TaskId tempId(task.streamId, task.batchId, task.taskId, task.contextId);
        taskTable[tempId].emplace_back(task);
    }
    if (!Utils::Reserve(deviceHcclTasks, hcclTasks->size()))
    {
        ERROR("Reserve for hccl task failed.");
        return false;
    }
    for (const auto& task : *hcclTasks)
    {
        TaskId tempId(task.streamId, task.batchId, task.taskId, task.contextId);
        if (taskTable.find(tempId) == taskTable.end())
        {
            ERROR("Hccl task can't match ascend task, streamId is: %, taskId is: %, contextId is: %, batchId is: %",
                  task.streamId, task.taskId, task.contextId, task.batchId);
            continue;
        }
        for (const auto& ascendTask : taskTable[tempId])
        {
            if (!Utils::IsDoubleEqual(ascendTask.startTime, -1))
            {
                deviceHcclTasks.emplace_back(InitHcclTaskData(ascendTask, task));
            }
        }
    }
    return true;
}

DeviceHcclTask HcclCalculator::InitHcclTaskData(const TopDownTask& topDownTask, const HcclTask& hcclTask)
{
    DeviceHcclTask task;
    task.modelId = hcclTask.modelId;
    task.indexId = hcclTask.indexId;
    task.hcclName = hcclTask.name;
    task.planeId = hcclTask.planeId;
    task.hostTimestamp = hcclTask.timestamp;
    task.groupName = hcclTask.groupName;
    task.isMaster = hcclTask.isMaster;
    task.streamId = hcclTask.streamId;
    task.taskId = hcclTask.taskId;
    task.durationEstimated = hcclTask.duration;
    task.localRank = hcclTask.localRank;
    task.remoteRank = hcclTask.remoteRank;
    task.transportType = hcclTask.transportType;
    task.size = hcclTask.size;
    task.dataType = hcclTask.dataType;
    task.linkType = hcclTask.linkType;
    task.contextId = hcclTask.contextId;
    task.threadId = hcclTask.threadId;
    task.notifyId = hcclTask.notifyId;
    task.batchId = hcclTask.batchId;
    task.rdmaType = hcclTask.rdmaType;
    task.timestamp = topDownTask.startTime;
    task.connectionId = topDownTask.connectionId;
    task.duration = topDownTask.endTime - topDownTask.startTime;
    task.rankSize = hcclTask.rankSize;
    return task;
}

void HcclCalculator::MergeOpDataByThreadId(std::vector<HcclOp>& hcclOps, std::vector<DeviceHcclTask>& hcclTasks,
                                           std::map<TaskId, uint16_t>& opCount)
{
    std::sort(hcclOps.begin(), hcclOps.end(),
              [](const HcclOp& op1, const HcclOp& op2) { return op1.timestamp < op2.timestamp; });
    std::sort(
        hcclTasks.begin(), hcclTasks.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2)
        { return std::tie(task1.hostTimestamp, task1.timestamp) < std::tie(task2.hostTimestamp, task2.timestamp); });
    size_t taskIdx = 0;
    for (auto& op : hcclOps)
    {
        while (taskIdx < hcclTasks.size() && hcclTasks[taskIdx].hostTimestamp < op.timestamp)
        {
            ERROR("Hccl task time not in ops time range, streamId is: %, taskId is: %, contextId is: %, batchId is: %",
                  hcclTasks[taskIdx].streamId, hcclTasks[taskIdx].taskId, hcclTasks[taskIdx].contextId,
                  hcclTasks[taskIdx].batchId);
            taskIdx++;
        }

        while ((taskIdx < hcclTasks.size()) && (hcclTasks[taskIdx].hostTimestamp <= (op.timestamp + op.duration)))
        {
            TaskId tempId(hcclTasks[taskIdx].streamId, hcclTasks[taskIdx].batchId, hcclTasks[taskIdx].taskId,
                          hcclTasks[taskIdx].contextId);
            uint16_t count = (opCount.find(tempId) == opCount.end()) ? 1 : (opCount[tempId] + 1);
            opCount[tempId] = count;
            taskData_.emplace_back(GetCompleteHcclTaskData(op, hcclTasks[taskIdx], count));
            op.rankSize = hcclTasks[taskIdx].rankSize;
            taskIdx++;
        }
        opData_.emplace_back(GetCompleteHcclOpData(op));
    }
    if (taskIdx != 0 && taskIdx < hcclTasks.size() - 1)
    {
        ERROR("Task_queue is not empty, len is: %", hcclTasks.size());
    }
}

bool HcclCalculator::MergeHcclOpData(const std::shared_ptr<std::vector<HcclOp>>& hcclOps,
                                     const std::vector<DeviceHcclTask>& deviceHcclTasks)
{
    INFO("Start merge hccl op data.");
    if (!Utils::Reserve(opData_, hcclOps->size()))
    {
        ERROR("Reserve for hccl op failed.");
        return false;
    }
    if (!Utils::Reserve(taskData_, deviceHcclTasks.size()))
    {
        ERROR("Reserve for hccl task failed.");
        return false;
    }

    std::unordered_map<uint32_t, std::vector<HcclOp>> hcclOpThreadMap;
    for (const auto& op : *hcclOps)
    {
        hcclOpThreadMap[op.threadId].emplace_back(op);
    }

    std::unordered_map<uint32_t, std::vector<DeviceHcclTask>> hcclTaskThreadMap;
    for (const auto& task : deviceHcclTasks)
    {
        hcclTaskThreadMap[task.threadId].emplace_back(task);
    }

    std::map<TaskId, uint16_t> opCount;
    for (auto& pair : hcclOpThreadMap)
    {
        if (hcclTaskThreadMap.find(pair.first) == hcclTaskThreadMap.end())
        {
            ERROR("Op data can't match any task, thread id is %.", pair.first);
        }
        else
        {
            MergeOpDataByThreadId(pair.second, hcclTaskThreadMap[pair.first], opCount);
        }
    }
    return true;
}

DeviceHcclTask HcclCalculator::GetCompleteHcclTaskData(const HcclOp& op, const DeviceHcclTask& hcclTask, uint16_t count)
{
    DeviceHcclTask task = hcclTask;
    task.opName = op.opName;
    task.groupName = (hcclTask.groupName == NA) ? op.groupName : hcclTask.groupName;
    task.taskType = op.taskType;
    task.opType = op.opType;
    task.firstTimestamp = op.timestamp;
    task.iterationId = count;
    task.isDynamic = op.isDynamic;
    task.modelId = op.modelId;
    task.connectionId = op.connectionId;
    if (!isValidData_ && task.opType != NA)
    {
        isValidData_ = true;
    }
    return task;
}

HcclOp HcclCalculator::GetCompleteHcclOpData(const HcclOp& op)
{
    HcclOp hcclOp;
    hcclOp.modelId = op.modelId;
    hcclOp.opName = op.opName;
    hcclOp.taskType = op.taskType;
    hcclOp.opType = op.opType;
    hcclOp.timestamp = op.timestamp;
    hcclOp.relay = op.relay;
    hcclOp.retry = op.retry;
    hcclOp.dataType = op.dataType;
    hcclOp.algType = op.algType;
    hcclOp.count = op.count;
    hcclOp.groupName = op.groupName;
    hcclOp.connectionId = op.connectionId;
    hcclOp.rankSize = op.rankSize;
    return hcclOp;
}

void HcclCalculator::UpdateHcclOpNameByGroupName(uint64_t clockMonotonicRaw)
{
    INFO("Start UpdateHcclOpNameByGroupName.");
    std::unordered_map<std::string, GroupData> hcclGroup;
    //  if data start in warmup, index will be set -1
    //  else index++ when groupName and taskType in group_dict or group name set first
    for (auto& data : taskData_)
    {
        auto taskType = (data.opName.find(AICPU_KERNEL) != std::string::npos) ? AICPU_KERNEL : NORMAL;
        auto key = Utils::Join("_", taskType, data.groupName);
        auto& groupEntry = hcclGroup[key];
        if (data.timestamp > clockMonotonicRaw && data.firstTimestamp > groupEntry.firstTimestamp)
        {
            groupEntry.firstTimestamp = data.firstTimestamp;
            groupEntry.count++;
        }
        int subPoint = 0;
        if (static_cast<int>(data.groupName.size()) > POS_COMPARE_BASE)
        {
            subPoint = static_cast<int>(data.groupName.size()) - POS_COMPARE_BASE;
        }
        auto subGroupName = data.groupName.substr(subPoint);
        data.opName = Utils::Join("_", data.opName, subGroupName, std::to_string(groupEntry.count),
                                  std::to_string(data.iterationId));
    }
}

void HcclCalculator::UpdateHcclBandwidth()
{
    INFO("Start UpdateHcclBandwidth.");
    // 按时间升序排序，确保后续payload遍历时数据顺序正确
    std::sort(taskData_.begin(), taskData_.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2)
              { return task1.timestamp < task2.timestamp; });
    std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<DeviceHcclTask*>>> taskTable;
    for (auto& data : taskData_)
    {
        // 没有提前reserve，这里可能很耗时
        taskTable[data.opName][data.planeId].push_back(&data);
    }
    for (auto& planeTable : taskTable)
    {
        for (auto& taskData : planeTable.second)
        {
            CalculateTaskBandwidth(taskData.second);
        }
    }
}

void HcclCalculator::CalculateTaskBandwidth(std::vector<DeviceHcclTask*> hcclTasks)
{
    uint16_t idx_jump = GetJumpNum(*hcclTasks.front());
    for (size_t idx = 0; idx < hcclTasks.size(); ++idx)
    {
        // 非RDMA_SEND_PAYLOAD类型直接计算；RDMA_SEND_PAYLOAD类型走其他计算逻辑
        if (hcclTasks[idx]->rdmaType != RDMA_SEND_PAYLOAD)
        {
            hcclTasks[idx]->bandwidth = CalculateBandwidth(hcclTasks[idx]->size, hcclTasks[idx]->duration);
            continue;
        }
        uint16_t payloadCnt = FindConsecutivePayloadTask(hcclTasks, idx);
        auto closeIdx = idx + payloadCnt + idx_jump - 2;
        if ((closeIdx) >= hcclTasks.size())
        {
            WARN(
                "Bandwidth calculation abnormal. Missing closure tasks. op_name: %, index is: %, paypladCnt is: %, "
                "idx_jump is: %,",
                hcclTasks[idx]->opName, idx, payloadCnt, idx_jump);
            hcclTasks[idx]->bandwidth = CalculateBandwidth(hcclTasks[idx]->size, hcclTasks[idx]->duration);
            continue;
        }
        auto payLoadAllSize = hcclTasks[idx]->size;
        for (size_t sizeI = idx + 1; sizeI < idx + payloadCnt; ++sizeI)
        {
            payLoadAllSize += hcclTasks[sizeI]->size;
        }
        auto transitTime = hcclTasks[closeIdx]->timestamp + hcclTasks[closeIdx]->duration - hcclTasks[idx]->timestamp;
        double payloadBandwidth = Utils::IsDoubleEqual(transitTime, 0.0) ? 0 : (payLoadAllSize / transitTime);
        for (size_t sizeI = idx; sizeI < idx + payloadCnt; ++sizeI)
        {
            hcclTasks[sizeI]->bandwidth = payloadBandwidth;
        }
        // 修改原有逻辑，下一个idx从连续的payload后开始。确保每个算子的bandwidth都被计算。
        idx += payloadCnt - 1;
    }
}

uint16_t HcclCalculator::GetJumpNum(const DeviceHcclTask& task)
{
    std::string opName = task.opName;
    transform(opName.begin(), opName.end(), opName.begin(), tolower);
    if (opName.find("send") != std::string::npos || opName.find("receive") != std::string::npos)
    {
        return RDMA_NO_BARRIER_TASK_NUM;
    }
    return RDMA_WITH_BARRIER_TASK_NUM;
}

double HcclCalculator::CalculateBandwidth(double size, double duration)
{
    // B -> GB: 以 1 / 10^9替代； ns -> s: 以 1 / 10^9替代。两者约分，带宽单位为 GB/s
    return (Utils::IsDoubleEqual(duration, 0.0) || (duration <= 0)) ? 0 : static_cast<double>(size) / duration;
}

uint16_t HcclCalculator::FindConsecutivePayloadTask(std::vector<DeviceHcclTask*> tasks, size_t idx)
{
    uint16_t count = 0;
    while (idx < tasks.size() && tasks[idx]->rdmaType == RDMA_SEND_PAYLOAD)
    {
        idx++;
        count++;
    }
    return count;
}

bool HcclCalculator::GetHcclStatisticsData(uint64_t clockMonotonicRaw)
{
    INFO("Start GetHcclStatisticsData.");
    if (!isValidData_)
    {
        WARN("No op type in hccl data.");
        return true;
    }
    std::unordered_map<std::string, OpTypeInfo> groupedData;
    for (const auto& task : taskData_)
    {
        if (task.isMaster == 0 || task.timestamp < clockMonotonicRaw)
        {
            continue;
        }
        auto key = Utils::Join("-", task.opName, std::to_string(task.firstTimestamp), task.opType);
        if (groupedData.find(key) == groupedData.end())
        {
            OpTypeInfo info(task.timestamp + task.duration, task.timestamp, task.opType);
            groupedData[key] = info;
        }
        else
        {
            auto& temp = groupedData[key];
            temp.max = std::max(task.timestamp + task.duration, temp.max);
            temp.min = std::min(task.timestamp, temp.min);
        }
    }
    std::unordered_map<std::string, HcclStatistics> statisticsTable;
    double allTaskTime = 0;
    for (const auto& data : groupedData)
    {
        auto& record = statisticsTable[data.second.opType];
        double duration = data.second.max - data.second.min;
        record.opType = data.second.opType;
        record.count++;
        record.totalTime += duration;
        record.max = std::max(duration, record.max);
        record.min = std::min(duration, record.min);
        allTaskTime += duration;
    }
    if (!Utils::Reserve(statisticsData_, statisticsTable.size()))
    {
        ERROR("Reserve for hccl statistics data failed.");
        return false;
    }
    for (auto& data : statisticsTable)
    {
        if ((data.second.count == 0) || Utils::IsDoubleEqual(allTaskTime, 0))
        {
            ERROR("Division by zero, and data.second.count: %  or allTaskTime: %.", data.second.count, allTaskTime);
            continue;
        }
        data.second.avg = static_cast<double>(data.second.totalTime) / data.second.count;
        data.second.ratio = static_cast<double>(data.second.totalTime) / allTaskTime * PERCENTAGE;
        statisticsData_.emplace_back(data.second);
    }
    std::sort(statisticsData_.begin(), statisticsData_.end(),
              [](const HcclStatistics& task1, const HcclStatistics& task2) { return task1.ratio > task2.ratio; });
    return true;
}

bool HcclCalculator::InjectData(DataInventory& inventory)
{
    INFO("Start inject hccl data.");
    auto hcclOpData = inventory.GetPtr<std::vector<HcclOp>>();
    // 直接替换原有的HcclOp数据
    hcclOpData->swap(opData_);

    bool flag = true;
    std::shared_ptr<std::vector<DeviceHcclTask>> hcclTaskData;
    MAKE_SHARED0_NO_OPERATION(hcclTaskData, std::vector<DeviceHcclTask>, std::move(taskData_));
    if (!inventory.Inject(hcclTaskData))
    {
        ERROR("Inject hccl task data failed.");
        flag = false;
    }

    std::shared_ptr<std::vector<HcclStatistics>> hcclStatisticsData;
    MAKE_SHARED0_NO_OPERATION(hcclStatisticsData, std::vector<HcclStatistics>, std::move(statisticsData_));
    if (!inventory.Inject(hcclStatisticsData))
    {
        ERROR("Inject hccl statistics data failed.");
        flag = false;
    }
    return flag;
}

REGISTER_PROCESS_SEQUENCE(HcclCalculator, true, AscendTaskAssociation, LoadHostData);
REGISTER_PROCESS_DEPENDENT_DATA(HcclCalculator, std::vector<TopDownTask>, std::vector<HcclTask>, std::vector<HcclOp>);
REGISTER_PROCESS_SUPPORT_CHIP(HcclCalculator, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
