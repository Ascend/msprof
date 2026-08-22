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
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
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
const std::string RDMA_SEND_PAYLOAD = "RDMA_SEND_PAYLOAD";
const int POS_COMPARE_BASE = 3;
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
    std::sort(taskData_.begin(), taskData_.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2)
              { return task1.timestamp < task2.timestamp; });
    std::sort(opData_.begin(), opData_.end(), [](const DeviceHcclOp& op1, const DeviceHcclOp& op2)
              { return std::tie(op1.start, op1.end) < std::tie(op2.start, op2.end); });

    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    DeviceStartInfo startInfo;
    SampleInfo sampleInfo;
    deviceContext.Getter(startInfo);
    deviceContext.Getter(sampleInfo);

    UpdateHcclOpNameByGroupName(startInfo.clockMonotonicRaw);
    UpdateHcclBandwidth();
    if (!GetHcclStatisticsData(startInfo.clockMonotonicRaw, sampleInfo))
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
    task.groupName = hcclTask.groupName;
    task.isMaster = hcclTask.isMaster;
    task.streamId = hcclTask.streamId;
    task.taskId = hcclTask.taskId;
    task.contextId = hcclTask.contextId;
    task.batchId = hcclTask.batchId;
    task.localRank = hcclTask.localRank;
    task.remoteRank = hcclTask.remoteRank;
    task.transportType = hcclTask.transportType;
    task.size = hcclTask.size;
    task.dataType = hcclTask.dataType;
    task.linkType = hcclTask.linkType;
    task.threadId = hcclTask.threadId;
    task.notifyId = hcclTask.notifyId;
    task.rdmaType = hcclTask.rdmaType;
    task.timestamp = topDownTask.startTime;
    task.duration = topDownTask.endTime - topDownTask.startTime;
    task.rankSize = hcclTask.rankSize;
    task.opId = hcclTask.opId;
    return task;
}

void HcclCalculator::MergeOpDataByThreadId(std::vector<HcclOp>& hcclOps, std::vector<DeviceHcclTask>& hcclTasks)
{
    // 对齐 Python _merge_hccl_ops_and_tasks: 按 op_id 精确匹配，不再使用时间窗口
    // 1. tasks 先按 op_id 分组、组内按 timestamp (ASCEND_TASK start_time) 排序。
    //    同一线程可能下发到多个 op，不同 op 的 task 时间戳会交错；
    //    若只按 timestamp 排序，同一 op 的 task 会被其他 op 的 task 打断，导致按 op_id 切分窗口时
    //    把单个 op 错误拆成多次迭代。先按 op_id 聚拢，才能保证同一 op 的 task 连续。
    std::sort(hcclTasks.begin(), hcclTasks.end(),
              [](const DeviceHcclTask& a, const DeviceHcclTask& b)
              {
                  if (a.opId != b.opId)
                  {
                      return a.opId < b.opId;
                  }
                  return a.timestamp < b.timestamp;
              });

    // 2. 构建 connection_id → HcclOp* 索引 (Python: op_thread_map[thread_id][connection_id] = op)
    std::unordered_map<int64_t, HcclOp*> opMap;
    for (auto& op : hcclOps)
    {
        opMap[op.connectionId] = &op;
    }

    // 3. 以四元组 (streamId, taskId, contextId, batchId) 作为 task 的唯一标识，
    //    记录当前 op 已出现的 task 四元组：同一静态 op 反复执行时四元组会重复，据此递增 iter_id；
    //    不同 op 的 task 四元组不同，不会误判，通信域交错的 task 仍能正确合并到各自 op。
    std::set<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> seenTaskIds;
    // 记录无法匹配的 op_id 用于日志
    std::set<int64_t> mismatchOpIds;

    int64_t currentOpId = -1;
    uint32_t currentIterId = 1;
    double groupStart = 0;
    double groupEnd = 0;
    int64_t lastRankSize = 0;
    bool hasGroup = false;  // 是否已有 master 聚合出窗口起点

    for (auto& task : hcclTasks)
    {
        // Python: if task.op_id not in op_id_map: mismatch, continue
        auto opIt = opMap.find(task.opId);
        if (opIt == opMap.end())
        {
            mismatchOpIds.insert(task.opId);
            continue;
        }

        // task 侧 groupName 来自 hcclTrace，与 op 侧（hcclopInfo）不同源，level0 下 task 侧残缺/不一致；
        // 统一用所属 op 的 groupName 刷新 task，保证后续按 groupName 分组/渲染以 op 侧为准。
        task.groupName = opIt->second->groupName;

        // 是否开始新一轮：op_id 变动，或同一 op 内 task 四元组重复（静态算子反复执行）
        auto taskKey = std::make_tuple(task.streamId, task.taskId, task.contextId, task.batchId);
        bool newRound = (task.opId != currentOpId) || (seenTaskIds.find(taskKey) != seenTaskIds.end());

        if (newRound)
        {
            // 上一组 op 结束，emit 上一组的 enriched op
            if (currentOpId != -1 && opMap.find(currentOpId) != opMap.end())
            {
                // 无 master task 的 op 无法聚合出有效 [start, end] 窗口，报错并跳过，避免落全 0 数据
                if (!hasGroup)
                {
                    ERROR("Hccl op has no master task, op_id(connection_id) is: %.", currentOpId);
                }
                else
                {
                    HcclOp* prevOp = opMap[currentOpId];
                    opData_.emplace_back(
                        GetCompleteHcclOpData(*prevOp, groupStart, groupEnd, lastRankSize, currentIterId));
                }
            }

            if (task.opId != currentOpId)
            {
                // 新的 op_id 分组，iter_id 从 1 开始
                currentOpId = task.opId;
                currentIterId = 1;
            }
            else
            {
                // 同一 op 反复执行，iter_id 递增
                currentIterId++;
            }
            seenTaskIds.clear();
            groupStart = 0;
            groupEnd = 0;
            lastRankSize = 0;
            hasGroup = false;
        }

        seenTaskIds.insert(taskKey);

        // 所有 task（含 non-master）统一携带 iter_id 落盘
        taskData_.emplace_back(GetCompleteHcclTaskData(*opMap[currentOpId], task, currentIterId));

        // 再取主流：只 master 聚合 [start, end] 窗口
        if (!task.isMaster) continue;
        lastRankSize = task.rankSize;
        if (!hasGroup)
        {
            groupStart = task.timestamp;
            groupEnd = task.timestamp + task.duration;
            hasGroup = true;
        }
        else
        {
            // Python: group_end = max(group_end, task.timestamp + task.duration)
            double newEnd = task.timestamp + task.duration;
            groupEnd = (newEnd > groupEnd) ? newEnd : groupEnd;
        }
    }

    if (currentOpId != -1 && opMap.find(currentOpId) != opMap.end())
    {
        // 无 master task 的 op 无法聚合出有效 [start, end] 窗口，报错并跳过，避免落全 0 数据
        if (!hasGroup)
        {
            ERROR("Hccl op has no master task, op_id(connection_id) is: %.", currentOpId);
        }
        else
        {
            HcclOp* lastOp = opMap[currentOpId];
            // TODO: 同上，timing 暂存于 DeviceHcclTask
            opData_.emplace_back(GetCompleteHcclOpData(*lastOp, groupStart, groupEnd, lastRankSize, currentIterId));
        }
    }

    if (!mismatchOpIds.empty())
    {
        std::string ids;
        for (auto id : mismatchOpIds)
        {
            ids += std::to_string(id) + ",";
        }
        ERROR("Some op_id can't match any task, size: %, op_ids: %", mismatchOpIds.size(), ids);
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

    for (auto& pair : hcclOpThreadMap)
    {
        if (hcclTaskThreadMap.find(pair.first) == hcclTaskThreadMap.end())
        {
            ERROR("Op data can't match any task, thread id is %.", pair.first);
        }
        else
        {
            MergeOpDataByThreadId(pair.second, hcclTaskThreadMap[pair.first]);
        }
    }
    return true;
}

DeviceHcclTask HcclCalculator::GetCompleteHcclTaskData(const HcclOp& op, const DeviceHcclTask& hcclTask, uint32_t count)
{
    DeviceHcclTask task = hcclTask;
    task.iterId = count;
    // 回填 op 的原始 opName，供带宽计算判定 send/receive（op 名 ≠ task 的 hcclName）
    task.opName = op.opName;
    return task;
}

DeviceHcclOp HcclCalculator::GetCompleteHcclOpData(const HcclOp& op, double groupStart, double groupEnd,
                                                   int64_t rankSize, uint32_t iterId)
{
    DeviceHcclOp hcclOp;
    hcclOp.modelId = op.modelId;
    hcclOp.indexId = op.indexId;
    hcclOp.threadId = op.threadId;
    hcclOp.opName = op.opName;
    hcclOp.taskType = op.taskType;
    hcclOp.opType = op.opType;
    hcclOp.connectionId = op.connectionId;
    hcclOp.relay = op.relay;
    hcclOp.retry = op.retry;
    hcclOp.dataType = op.dataType;
    hcclOp.algType = op.algType;
    hcclOp.count = op.count;
    hcclOp.groupName = op.groupName;
    hcclOp.rankSize = rankSize;
    hcclOp.iterId = iterId;
    hcclOp.start = groupStart;
    hcclOp.end = groupEnd;
    return hcclOp;
}

void HcclCalculator::UpdateHcclOpNameByGroupName(uint64_t clockMonotonicRaw)
{
    INFO("Start UpdateHcclOpNameByGroupName.");
    std::unordered_map<std::string, GroupData> hcclGroup;
    //  if data start in warmup, index will be set -1
    //  else index++ when groupName in group_dict or group name set first
    for (auto& data : opData_)
    {
        auto& groupEntry = hcclGroup[data.groupName];
        if (data.end > clockMonotonicRaw) groupEntry.count++;
        int subPoint = 0;
        if (static_cast<int>(data.groupName.size()) > POS_COMPARE_BASE)
        {
            subPoint = static_cast<int>(data.groupName.size()) - POS_COMPARE_BASE;
        }
        auto subGroupName = data.groupName.substr(subPoint);
        data.opName =
            Utils::Join("_", data.opName, subGroupName, std::to_string(groupEntry.count), std::to_string(data.iterId));
    }
}

void HcclCalculator::UpdateHcclBandwidth()
{
    INFO("Start UpdateHcclBandwidth.");
    // 按时间升序排序，确保后续payload遍历时数据顺序正确
    std::sort(taskData_.begin(), taskData_.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2)
              { return task1.timestamp < task2.timestamp; });
    // 按 (opId, iterId) 精确区分每个算子实例，替代原先的 hcclName 分组
    std::map<std::pair<int64_t, uint32_t>, std::map<int32_t, std::vector<DeviceHcclTask*>>> taskTable;
    for (auto& data : taskData_)
    {
        // 没有提前reserve，这里可能很耗时
        taskTable[std::make_pair(data.opId, data.iterId)][data.planeId].push_back(&data);
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

bool HcclCalculator::GetHcclStatisticsData(uint64_t clockMonotonicRaw, SampleInfo sampleInfo)
{
    INFO("Start GetHcclStatisticsData.");
    if (sampleInfo.isLevel0)
    {
        WARN("No op type in hccl data.");
        return true;
    }
    std::unordered_map<std::string, HcclStatistics> statisticsTable;
    for (const auto& op : opData_)
    {
        // 对齐 Python generate_op_report_data: 过滤 warmup 算子（end 早于采集起始时间）
        if (op.end < clockMonotonicRaw) continue;
        auto& record = statisticsTable[op.opType];
        double duration = op.end - op.start;
        record.opType = op.opType;
        record.count++;
        record.totalTime += duration;
        record.max = std::max(duration, record.max);
        record.min = std::min(duration, record.min);
    }
    if (!Utils::Reserve(statisticsData_, statisticsTable.size()))
    {
        ERROR("Reserve for hccl statistics data failed.");
        return false;
    }
    for (auto& data : statisticsTable)
    {
        // 业务保证count非零
        data.second.avg = static_cast<double>(data.second.totalTime) / data.second.count;
        statisticsData_.emplace_back(data.second);
    }
    return true;
}

bool HcclCalculator::InjectData(DataInventory& inventory)
{
    INFO("Start inject hccl data.");
    bool flag = true;
    std::shared_ptr<std::vector<DeviceHcclOp>> hcclOpData;
    MAKE_SHARED0_NO_OPERATION(hcclOpData, std::vector<DeviceHcclOp>, std::move(opData_));
    if (!inventory.Inject(hcclOpData))
    {
        ERROR("Inject hccl op data failed.");
        flag = false;
    }

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
