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

#include "analysis/csrc/domain/services/association/calculator/hccl/include/kfc_calculator.h"

#include <algorithm>
#include <utility>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/aicpu_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/ascend_task_persistence.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis
{
namespace Domain
{
namespace
{
// plane_id 基数：HCCL 从 1 开始，MC2 从 0 开始（对齐 Python _assign_op_and_plane 的 plane_id_base）
const int32_t HCCL_PLANE_ID_BASE = 1;
const int32_t MC2_PLANE_ID_BASE = 0;

std::vector<int64_t> SplitCommaToInt64(const std::string& str)
{
    // 复用 Utils::Split 按逗号拆分，仅保留非空且能转成 int64 的项（行为与原 getline 拆分一致）
    std::vector<int64_t> ids;
    for (const auto& item : Utils::Split(str, ","))
    {
        if (item.empty()) continue;
        int64_t id = 0;
        if (Utils::StrToInt64(id, item) != ANALYSIS_OK)
        {
            WARN("kfc_connection_ids contains invalid number: %", item);
            continue;
        }
        ids.emplace_back(id);
    }
    return ids;
}
}  // namespace

uint32_t KfcCalculator::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    INFO("Start Kfc calculator ProcessEntry.");
    auto ascendTasks = dataInventory.GetPtr<std::vector<TopDownTask>>();
    auto hcclOps = dataInventory.GetPtr<std::vector<HcclOp>>();
    if (ascendTasks == nullptr || hcclOps == nullptr)
    {
        ERROR("Kfc calculator get ascend task or hccl op data pointer is nullptr.");
        return ANALYSIS_ERROR;
    }

    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    KfcUpstreamData upstream;
    if (!BuildUpstreamData(dataInventory, deviceContext, *ascendTasks, *hcclOps, upstream))
    {
        // 对齐 Python ms_run：mc2_comm_info.db 缺失时整体跳过；其余上游缺失时结果为空，同样优雅跳过
        WARN("Failed to build kfc upstream data, skip kfc calculation.");
        return ANALYSIS_OK;
    }
    if (upstream.aicpuKernels.empty())
    {
        WARN("Kfc calculator has no aicpu kernel data.");
        return ANALYSIS_OK;
    }

    if (!Calculate(upstream))
    {
        ERROR("Failed to calculate kfc data.");
        return ANALYSIS_ERROR;
    }
    if (!InjectData(dataInventory))
    {
        ERROR("Failed to inject kfc data.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

bool KfcCalculator::BuildUpstreamData(DataInventory& dataInventory, const DeviceContext& deviceContext,
                                      const std::vector<TopDownTask>& ascendTasks, const std::vector<HcclOp>& hcclOps,
                                      KfcUpstreamData& upstream)
{
    DeviceStartInfo startInfo;
    SampleInfo sampleInfo;
    deviceContext.Getter(startInfo);
    deviceContext.Getter(sampleInfo);
    upstream.startTimeRawTimestamp = startInfo.clockMonotonicRaw;
    upstream.isLevel0 = sampleInfo.isLevel0;

    // master stream：对齐 get_aicpu_master_stream_hccl_task。
    // aicpu_batch_id 由 AicpuPersistence 用 device flip 计算后注入，直接透传。
    auto masterStreamTasks = dataInventory.GetPtr<std::vector<MasterStreamTaskData>>();
    if (masterStreamTasks != nullptr)
    {
        for (const auto& item : *masterStreamTasks)
        {
            DeviceMasterStreamHcclTask masterTask;
            masterTask.taskType = item.taskType;
            masterTask.streamId = item.streamId;
            masterTask.taskId = item.taskId;
            masterTask.batchId = item.batchId;
            masterTask.aicpuStreamId = item.aicpuStreamId;
            masterTask.aicpuTaskId = item.aicpuTaskId;
            masterTask.aicpuBatchId = item.aicpuBatchId;
            masterTask.timestamp = item.timeStamp;
            upstream.masterStreamHcclTask.emplace_back(std::move(masterTask));
        }
    }

    // aicpu kernel：对齐 get_kfc_op_data（GE_TASK + ASCEND_TASK 内存 join）。
    // op_name 由 LoadHostData 读取 ge_info.db 注入（缺失/无 AicpuKernel 时 map 为空，
    // aicpuKernels 为空，ProcessEntry 据此优雅跳过）。
    auto opNameByTask = dataInventory.GetPtr<AicpuOpNameMap>();
    if (opNameByTask != nullptr)
    {
        BuildAicpuKernels(ascendTasks, *opNameByTask, upstream.aicpuKernels);
    }

    // hccl host op：对齐 get_hccl_ops（HCCLOP，LoadHostData 注入）
    for (const auto& op : hcclOps)
    {
        DeviceHcclHostOp hostOp;
        hostOp.connectionId = op.connectionId;
        hostOp.kfcConnectionIds = op.kfcConnectionIds;
        hostOp.opName = op.opName;
        hostOp.groupName = op.groupName;
        upstream.hcclHostOps.emplace_back(std::move(hostOp));
    }

    // mc2 通信域：由 LoadHostData 读取 host mc2_comm_info.db 注入。
    // 对齐 Python ms_run：mc2_comm_info.db 缺失/为空时 kfc 计算整体跳过。
    auto mc2CommInfo = dataInventory.GetPtr<std::vector<Mc2CommInfo>>();
    if (mc2CommInfo == nullptr || mc2CommInfo->empty()) return false;
    upstream.mc2CommInfo = *mc2CommInfo;

    // kfc task：对齐 get_kfc_info_with_task_by_stream_ids / get_ascend_task_with_kfc_defaults。
    // KfcInfoData/MasterStreamTaskData 由 AicpuPersistence 注入；level0 无 aicpu 数据时为空向量。
    std::vector<KfcInfoData> kfcInfos;
    auto kfcInfoData = dataInventory.GetPtr<std::vector<KfcInfoData>>();
    if (kfcInfoData != nullptr)
    {
        kfcInfos = *kfcInfoData;
    }
    BuildKfcTasks(upstream.isLevel0, kfcInfos, ascendTasks, upstream.mc2CommInfo, upstream.kfcTasks);
    return true;
}

void KfcCalculator::BuildAicpuKernels(const std::vector<TopDownTask>& ascendTasks, const AicpuOpNameMap& opNameByTask,
                                      std::vector<DeviceKfcOp>& aicpuKernels)
{
    // 对齐 Python get_kfc_op_data：GE_TASK(TaskInfo, op_name) INNER JOIN ASCEND_TASK(TopDownTask)。
    // op_name 已由 LoadHostData 读取 ge_info.db 注入（SQL 过滤 op_name LIKE '%AicpuKernel%'）
    for (const auto& task : ascendTasks)
    {
        if (Utils::IsDoubleEqual(task.startTime, INVALID_TIME)) continue;
        auto opIt = opNameByTask.find(TaskId(task.streamId, task.batchId, task.taskId, task.contextId));
        if (opIt == opNameByTask.end()) continue;

        DeviceKfcOp kernel;
        kernel.modelId = task.modelId;
        kernel.indexId = task.indexId;
        kernel.streamId = task.streamId;
        kernel.taskId = task.taskId;
        kernel.contextId = task.contextId;
        kernel.batchId = task.batchId;
        kernel.start = task.startTime;
        kernel.end = task.endTime;
        kernel.kfcConnectionId = task.connectionId;
        kernel.opName = opIt->second;
        aicpuKernels.emplace_back(std::move(kernel));
    }
    // 对齐 SQL order by b.start_time
    std::sort(aicpuKernels.begin(), aicpuKernels.end(),
              [](const DeviceKfcOp& k1, const DeviceKfcOp& k2) { return k1.start < k2.start; });
}

void KfcCalculator::BuildKfcTasks(bool isLevel0, const std::vector<KfcInfoData>& kfcInfos,
                                  const std::vector<TopDownTask>& ascendTasks, const std::vector<Mc2CommInfo>& commInfo,
                                  std::vector<DeviceHcclTask>& kfcTasks)
{
    // 展开 comm_stream_ids，用于过滤 kfc 任务流（对齐 SQL stream_id in (...)/b.stream_id in (...)
    std::set<uint32_t> commStreamIds;
    for (const auto& info : commInfo)
    {
        commStreamIds.insert(info.commStreamIds.begin(), info.commStreamIds.end());
    }
    if (commStreamIds.empty()) return;

    // 两分支公共部分：从 task 取任务身份/时间字段，过滤非法 startTime 后追加；
    // 非 level0 的 JOIN 键保证 task 与 kfc 四元组一致，统一从 task 取即可
    auto AppendAscendTask = [&kfcTasks](const TopDownTask& task, DeviceHcclTask& item)
    {
        if (Utils::IsDoubleEqual(task.startTime, INVALID_TIME)) return;
        item.timestamp = task.startTime;
        item.duration = task.endTime - task.startTime;
        item.streamId = task.streamId;
        item.taskId = task.taskId;
        item.contextId = task.contextId;
        item.batchId = task.batchId;
        item.opId = INVALID_VALUE;  // FilterMatchedTasks 靠 -1 过滤，DeviceHcclTask 默认 0 不适用
        kfcTasks.emplace_back(std::move(item));
    };

    if (isLevel0)
    {
        // 对齐 get_ascend_task_with_kfc_defaults：仅 ASCEND_TASK + kfc 默认值
        // localRank/remoteRank/rankSize/planeId/bandwidth 等与 DeviceHcclTask 默认值一致，无需重复赋值
        for (const auto& task : ascendTasks)
        {
            if (commStreamIds.find(task.streamId) == commStreamIds.end()) continue;
            DeviceHcclTask item;
            item.hcclName = NA;
            item.notifyId = NA;
            item.size = -1;
            item.dataType = NA;
            item.linkType = NA;
            item.transportType = NA;
            item.rdmaType = NA;
            AppendAscendTask(task, item);
        }
    }
    else
    {
        // 对齐 get_kfc_info_with_task_by_stream_ids：KFC_INFO(KfcInfoData) INNER JOIN ASCEND_TASK(TopDownTask)。
        // 同一四元组在 ASCEND_TASK 可能多次出现（动态 profiling 的重复执行），JOIN 会展开为多行，需全部保留
        // TaskId 构造函数参数序为 (streamId, batchId, taskId, contextId)，deviceId 默认 0
        // （per-device 处理，等价于原四元组 (streamId, taskId, contextId, batchId)）
        std::map<TaskId, std::vector<const TopDownTask*>> ascendIndex;
        for (const auto& task : ascendTasks)
        {
            ascendIndex[TaskId(task.streamId, task.batchId, task.taskId, task.contextId)].emplace_back(&task);
        }
        for (const auto& kfc : kfcInfos)
        {
            if (commStreamIds.find(kfc.streamId) == commStreamIds.end()) continue;
            auto ascendIt = ascendIndex.find(TaskId(kfc.streamId, kfc.batchId, kfc.taskId, kfc.contextId));
            if (ascendIt == ascendIndex.end()) continue;
            for (const TopDownTask* taskPtr : ascendIt->second)
            {
                DeviceHcclTask item;
                item.hcclName = kfc.hcclName;
                item.localRank = kfc.localRank;
                item.remoteRank = kfc.remoteRank;
                item.rankSize = kfc.rankSize;
                item.planeId = kfc.planeId;
                item.notifyId = kfc.notifyId;
                item.size = kfc.size;
                item.dataType = kfc.dataType;
                item.linkType = kfc.linkType;
                item.transportType = kfc.transportType;
                item.rdmaType = kfc.rdmaType;
                AppendAscendTask(*taskPtr, item);
            }
        }
    }
    // 对齐 SQL order by b.start_time / ASCEND_TASK start_time：AssignOpAndPlane 依赖按 timestamp 升序
    std::sort(kfcTasks.begin(), kfcTasks.end(),
              [](const DeviceHcclTask& t1, const DeviceHcclTask& t2) { return t1.timestamp < t2.timestamp; });
}

bool KfcCalculator::InjectData(DataInventory& inventory)
{
    INFO("Start inject kfc data.");
    bool flag = true;
    std::shared_ptr<std::vector<KfcTaskRecord>> kfcTaskData;
    MAKE_SHARED0_NO_OPERATION(kfcTaskData, std::vector<KfcTaskRecord>, std::move(kfcTaskData_));
    if (!inventory.Inject(kfcTaskData))
    {
        ERROR("Inject kfc task data failed.");
        flag = false;
    }
    std::shared_ptr<std::vector<KfcOpRecord>> kfcOpData;
    MAKE_SHARED0_NO_OPERATION(kfcOpData, std::vector<KfcOpRecord>, std::move(kfcOpData_));
    if (!inventory.Inject(kfcOpData))
    {
        ERROR("Inject kfc op data failed.");
        flag = false;
    }
    std::shared_ptr<std::vector<KfcOpStatistics>> kfcOpReportData;
    MAKE_SHARED0_NO_OPERATION(kfcOpReportData, std::vector<KfcOpStatistics>, std::move(mc2OpReportData_));
    if (!inventory.Inject(kfcOpReportData))
    {
        ERROR("Inject kfc op report data failed.");
        flag = false;
    }
    return flag;
}

bool KfcCalculator::Calculate(const KfcUpstreamData& upstream)
{
    std::vector<DeviceKfcOp> hcclKernels;
    std::vector<DeviceKfcOp> mc2Kernels;
    GetHcclAndMc2Op(upstream.aicpuKernels, upstream.hcclHostOps, hcclKernels, mc2Kernels);
    GenerateHcclKernels(hcclKernels, upstream.isLevel0, upstream);
    GenerateMc2Kernels(mc2Kernels, upstream.isLevel0, upstream);
    if (upstream.isLevel0)
    {
        WARN("Profiling level is level0, no need to export statistics data.");
        return true;
    }
    // 对齐 HcclCalculator::GetHcclStatisticsData：报告聚合失败（Reserve 分配失败）时透传失败，
    // ProcessEntry 据此返回 ANALYSIS_ERROR，避免注入残缺的 mc2 op 报告
    if (!GenerateOpReportData(mc2Kernels, mc2OpReportData_, static_cast<double>(upstream.startTimeRawTimestamp)))
    {
        ERROR("Failed to generate mc2 op report data.");
        return false;
    }
    return true;
}

void KfcCalculator::GetHcclAndMc2Op(const std::vector<DeviceKfcOp>& aicpuKernels,
                                    const std::vector<DeviceHcclHostOp>& hcclHostOps,
                                    std::vector<DeviceKfcOp>& hcclKernels, std::vector<DeviceKfcOp>& mc2Kernels) const
{
    // 获取符合条件的 aicpuKernel（SQL 已按 start_time 排序，此处假定上游数据已排好序）
    std::vector<DeviceKfcOp> aicpuKernelList = aicpuKernels;
    // 按四元组出现次数标记 iter_id（从 1 开始），区分同一 kernel 的多次执行（如 task flip/重执行）
    std::map<TaskId, uint32_t> counter;
    for (auto& kfcOp : aicpuKernelList)
    {
        auto key = TaskId(kfcOp.streamId, kfcOp.batchId, kfcOp.taskId, kfcOp.contextId);
        ++counter[key];
        kfcOp.iterId = counter[key];
    }

    // 从 HCCL_OP 里取数据，以 kfc_connection_id 进行分组
    auto kfcHcclOpMap = GetKfcHostHcclOp(hcclHostOps);
    for (const auto& kfcOp : aicpuKernelList)
    {
        auto hcclOpIt = kfcHcclOpMap.find(kfcOp.kfcConnectionId);
        if (hcclOpIt != kfcHcclOpMap.end())
        {
            // 匹配到 HCCL_OP，说明是 HCCL
            DeviceKfcOp hcclKernel = kfcOp;
            hcclKernel.connectionId = hcclOpIt->second->connectionId;
            hcclKernel.opName = hcclOpIt->second->opName;
            hcclKernel.groupName = hcclOpIt->second->groupName;
            hcclKernels.emplace_back(std::move(hcclKernel));
        }
        else
        {
            // 未匹配到，说明是 MC2
            DeviceKfcOp mc2Kernel = kfcOp;
            mc2Kernel.connectionId = mc2Kernel.kfcConnectionId;
            mc2Kernels.emplace_back(std::move(mc2Kernel));
        }
    }
}

std::unordered_map<int64_t, const DeviceHcclHostOp*> KfcCalculator::GetKfcHostHcclOp(
    const std::vector<DeviceHcclHostOp>& hcclHostOps) const
{
    std::unordered_map<int64_t, const DeviceHcclHostOp*> kfcHcclOpMap;
    for (const auto& data : hcclHostOps)
    {
        for (auto kfcConnectionId : SplitCommaToInt64(data.kfcConnectionIds))
        {
            kfcHcclOpMap[kfcConnectionId] = &data;
        }
    }
    return kfcHcclOpMap;
}

void KfcCalculator::RefineKernelTimesWithMasterStream(const std::vector<DeviceMasterStreamHcclTask>& masterStreamTasks,
                                                      const std::vector<DeviceHcclTask>& kfcTaskData,
                                                      std::vector<DeviceKfcOp>& hcclKernels) const
{
    if (masterStreamTasks.empty()) return;

    // 用 kfcTask 数据构建小 task 索引 (unique_id → 多次执行按 timestamp 排序)。
    // 同一四元组多次执行（动态 profiling 重复执行）时，若只保留最后一条，各轮 kernel 都会被
    // 修正为最后一轮的时间，故按出现顺序保留全部，处理时按轮次取对应小 task
    std::map<TaskId, std::vector<const DeviceHcclTask*>> hcclSmallTask;
    for (const auto& data : kfcTaskData)
    {
        hcclSmallTask[TaskId(data.streamId, data.batchId, data.taskId, data.contextId)].emplace_back(&data);
    }
    for (auto& kv : hcclSmallTask)
    {
        std::sort(kv.second.begin(), kv.second.end(),
                  [](const DeviceHcclTask* t1, const DeviceHcclTask* t2) { return t1->timestamp < t2->timestamp; });
    }

    // 构建 kernel_key → [first_start, last_end]，key 中加入 iter_id 区分多次执行
    using KernelKey = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
    std::map<KernelKey, std::pair<double, double>> kernelTimes;
    // aicpu 侧 key 保留 4 元组：后续经 std::tuple_cat 与 iter_id 拼接成 KernelKey，无法用 TaskId 平替
    using AicpuTaskId = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>;
    std::map<AicpuTaskId, uint32_t> aicpuIter;  // (aicpu_stream_id, aicpu_task_id, ctx, batch) → 当前执行次数
    std::map<TaskId, std::pair<uint32_t, uint32_t>> uidOccurrence;  // uid → (FIRST 出现次数, LAST 出现次数)
    std::set<TaskId> mismatch;                                      // 与 hcclSmallTask 匹配失败的 uid
    std::set<KernelKey> missingFirst;                               // 有 LAST 但无对应 FIRST 的异常 key
    for (const auto& data : masterStreamTasks)
    {
        if (data.taskType != FIRST_TASK_TYPE && data.taskType != LAST_TASK_TYPE) continue;
        // masterStreamTask 无 context_id，按 GE 默认 context（UINT32_MAX）与 hcclSmallTask 的 TaskId 匹配
        TaskId uid(data.streamId, data.batchId, data.taskId, INVALID_CONTEXT_ID);
        auto smallTaskIt = hcclSmallTask.find(uid);
        if (smallTaskIt == hcclSmallTask.end())
        {
            mismatch.insert(uid);
            continue;
        }
        AicpuTaskId aicpuKey =
            std::make_tuple(data.aicpuStreamId, data.aicpuTaskId, INVALID_CONTEXT_ID, data.aicpuBatchId);
        // 按出现顺序取本轮对应的小 task：FIRST(起始 task) 与 LAST(结束 task) 通常是不同 uid，
        // 各自独立计数推进；同一 uid 多次执行（重复执行）时后一次不再覆盖前一次
        uint32_t occ;
        if (data.taskType == FIRST_TASK_TYPE)
        {
            occ = uidOccurrence[uid].first++;
        }
        else
        {
            occ = uidOccurrence[uid].second++;
        }
        if (occ >= smallTaskIt->second.size())
        {
            // master 中出现次数多于 kfc 小 task 条数时，取最后一条兜底，避免越界
            occ = smallTaskIt->second.size() - 1;
        }
        const DeviceHcclTask* smallTask = smallTaskIt->second[occ];
        // FIRST 表示新一轮执行开始，递增 iter_id（从 1 开始）
        if (data.taskType == FIRST_TASK_TYPE)
        {
            ++aicpuIter[aicpuKey];
        }
        uint32_t iterId = aicpuIter[aicpuKey];
        KernelKey key = std::tuple_cat(aicpuKey, std::make_tuple(iterId));
        double taskEnd = smallTask->timestamp + smallTask->duration;
        if (data.taskType == FIRST_TASK_TYPE)
        {
            kernelTimes[key] = std::make_pair(smallTask->timestamp, taskEnd);
        }
        else if (kernelTimes.find(key) == kernelTimes.end())
        {
            // LAST 无对应 FIRST（如 FIRST 因 uid mismatch 被跳过），记录异常 key，不做时间修正
            missingFirst.insert(key);
        }
        else
        {
            kernelTimes[key].second = taskEnd;
        }
    }
    if (!mismatch.empty())
    {
        ERROR("Can not match any master task for these unique id, size is: %", mismatch.size());
    }
    if (!missingFirst.empty())
    {
        ERROR("LAST task has no matching FIRST task, abnormal key size is: %", missingFirst.size());
    }

    // 修正 hccl_kernels 的 start/end，key 中加入 iter_id 匹配对应次数的执行
    for (auto& kernel : hcclKernels)
    {
        auto timesIt = kernelTimes.find(MakeKernelKey(kernel));
        if (timesIt == kernelTimes.end()) continue;
        kernel.start = timesIt->second.first;
        kernel.end = timesIt->second.second;
    }
}

std::map<std::string, std::vector<DeviceKfcOp>> KfcCalculator::GroupKernelsByGroupName(
    const std::vector<DeviceKfcOp>& kernels)
{
    // 按 group_name 分组并排序 kernels（组内按 start 升序）
    std::map<std::string, std::vector<DeviceKfcOp>> kb;
    for (const auto& k : kernels)
    {
        kb[k.groupName].emplace_back(k);
    }
    for (auto& v : kb)
    {
        std::sort(v.second.begin(), v.second.end(),
                  [](const DeviceKfcOp& k1, const DeviceKfcOp& k2) { return k1.start < k2.start; });
    }
    return kb;
}

std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t> KfcCalculator::MakeKernelKey(const DeviceKfcOp& kernel)
{
    // (stream_id, task_id, context_id, batch_id, iter_id)
    return std::make_tuple(kernel.streamId, kernel.taskId, kernel.contextId, kernel.batchId, kernel.iterId);
}

std::map<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>, std::pair<double, double>>
KfcCalculator::AssignOpAndPlane(std::map<std::string, std::vector<DeviceHcclTask>>& groupedTasks,
                                const std::map<std::string, std::vector<DeviceKfcOp>>& kernelByGroup,
                                int planeIdBase) const
{
    // 双指针匹配 op_id 并分配 plane_id，标记 is_master（op 时间范围内=1，外=0），
    // 原地修改 groupedTasks，返回 kernel_times：{kernel_key → (min_start, max_end)}
    using KernelKey = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
    std::map<KernelKey, std::pair<double, double>> kernelTimes;
    for (auto& gnamePair : groupedTasks)
    {
        auto kernelsIt = kernelByGroup.find(gnamePair.first);
        if (kernelsIt == kernelByGroup.end() || kernelsIt->second.empty()) continue;
        const std::vector<DeviceKfcOp>& kernels = kernelsIt->second;
        std::map<uint32_t, int32_t> streamPlane;
        size_t opIndex = 0;
        size_t opLen = kernels.size();
        std::vector<DeviceHcclTask>& tasks = gnamePair.second;
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            // 引用原地修改，避免 DeviceHcclTask（11 个 string 成员）逐元素拷贝进/写回两次深拷贝
            DeviceHcclTask& data = tasks[i];
            while (opIndex < opLen && kernels[opIndex].end < data.timestamp)
            {
                ++opIndex;
            }
            if (opIndex < opLen && kernels[opIndex].start <= data.timestamp)
            {
                // 用 op 的 opId 和 groupName，并同步 kernel 的 iter_id 给 task；
                // 落在 op 时间范围内的 task 标记为主流
                const DeviceKfcOp& kernel = kernels[opIndex];
                KernelKey key = MakeKernelKey(kernel);
                data.opId = kernel.connectionId;
                data.groupName = kernel.groupName;
                data.modelId = kernel.modelId;
                data.indexId = kernel.indexId;
                data.iterId = kernel.iterId;
                data.opName = kernel.opName;
                data.isMaster = 1;

                double start = data.timestamp;
                double end = data.timestamp + data.duration;
                auto prev = kernelTimes.find(key);
                if (prev != kernelTimes.end())
                {
                    prev->second.first = std::min(start, prev->second.first);
                    prev->second.second = std::max(end, prev->second.second);
                }
                else
                {
                    kernelTimes[key] = std::make_pair(start, end);
                }
            }
            else
            {
                // 时间范围外的 task 不算主流
                data.isMaster = 0;
            }
            // 同一 stream 分配连续 plane_id（从 planeIdBase 开始）
            auto sidIt = streamPlane.find(data.streamId);
            if (sidIt == streamPlane.end())
            {
                streamPlane[data.streamId] = static_cast<int32_t>(streamPlane.size()) + planeIdBase;
            }
            data.planeId = streamPlane[data.streamId];
        }
    }
    // 仅用于 mc2 算子 op 算子时间刷新（mc2 无 mainStreamTask）
    return kernelTimes;
}

std::map<std::string, std::vector<DeviceHcclTask>> KfcCalculator::GroupTasksByCommStream(
    const std::vector<DeviceHcclTask>& taskData,
    const std::unordered_map<uint32_t, std::set<std::string>>& commStreamIdGroupTable)
{
    // 按 mc2Info 展开流信息(stream_id → {group_name}) 分组 task，非 comm 流的 task 自然被排除
    std::map<std::string, std::vector<DeviceHcclTask>> gt;
    for (const auto& data : taskData)
    {
        auto it = commStreamIdGroupTable.find(data.streamId);
        if (it == commStreamIdGroupTable.end()) continue;
        for (const auto& gname : it->second)
        {
            gt[gname].emplace_back(data);
        }
    }
    return gt;
}

std::vector<DeviceHcclTask> KfcCalculator::FilterMatchedTasks(const std::vector<DeviceHcclTask>& tasks)
{
    // 按 op 时间筛选：只保留匹配到某个 op 时间范围内的 task（op_id 已赋值）；
    // op_id 仍为默认 -1 的 task 落在所有 op 时间窗之外（归属其他 op），不落库
    std::vector<DeviceHcclTask> matchedTasks;
    for (const auto& t : tasks)
    {
        if (t.opId != INVALID_VALUE)
        {
            matchedTasks.emplace_back(t);
        }
    }
    return matchedTasks;
}

void KfcCalculator::GenerateHcclKernels(std::vector<DeviceKfcOp>& hcclKernels, bool isLevel0,
                                        const KfcUpstreamData& upstream)
{
    if (isLevel0 || hcclKernels.empty()) return;

    // task 数据与 mc2 流程一致：按 mc2Info 展开流(comm_stream_ids)过滤（上游已过滤，此处仅用分组表）
    auto commInfo = GetMc2CommInfoData(upstream.mc2CommInfo);
    const auto& commStreamIdGroupTable = commInfo.second;
    const auto& kfcTaskData = upstream.kfcTasks;
    if (commStreamIdGroupTable.empty() || kfcTaskData.empty()) return;

    RefineKernelTimesWithMasterStream(upstream.masterStreamHcclTask, kfcTaskData, hcclKernels);

    auto kernelByGroup = GroupKernelsByGroupName(hcclKernels);
    auto groupedTasks = GroupTasksByCommStream(kfcTaskData, commStreamIdGroupTable);
    AssignOpAndPlane(groupedTasks, kernelByGroup, HCCL_PLANE_ID_BASE);

    // op_id/iter_id 已在 AssignOpAndPlane 中赋值，逐组按 (op_id, iter_id) 计算带宽；
    // 主从流 task 都落库，仅按 op 时间剔除归属其他 op 的 task
    for (auto& gnamePair : groupedTasks)
    {
        if (kernelByGroup.find(gnamePair.first) == kernelByGroup.end()) continue;
        auto matchedTasks = FilterMatchedTasks(gnamePair.second);
        UpdateBandwidth(matchedTasks);
        for (const auto& t : matchedTasks)
        {
            kfcTaskData_.emplace_back(SerializeKfcTask(t, HcclType::HCCL));
        }
    }
}

void KfcCalculator::GenerateMc2Kernels(std::vector<DeviceKfcOp>& mc2Kernels, bool isLevel0,
                                       const KfcUpstreamData& upstream)
{
    (void)isLevel0;  // 上游数据形状差异（level0 用 ascend task + kfc 默认值）已在上游侧处理
    if (mc2Kernels.empty()) return;

    auto commInfo = GetMc2CommInfoData(upstream.mc2CommInfo);
    const auto& aicpuInfo = commInfo.first;
    const auto& commStreamIdGroupTable = commInfo.second;

    for (auto& kernel : mc2Kernels)
    {
        auto infoIt = aicpuInfo.find(kernel.streamId);
        if (infoIt != aicpuInfo.end())
        {
            kernel.groupName = infoIt->second.first;
            kernel.rankSize = infoIt->second.second;
        }
        kernel.opType = kernel.opName;
    }
    const auto& commData = upstream.kfcTasks;
    if (commStreamIdGroupTable.empty() || commData.empty()) return;

    auto groupedTasks = GroupTasksByCommStream(commData, commStreamIdGroupTable);
    auto kernelByGroup = GroupKernelsByGroupName(mc2Kernels);
    auto kernelTimes = AssignOpAndPlane(groupedTasks, kernelByGroup, MC2_PLANE_ID_BASE);

    for (auto& gnamePair : groupedTasks)
    {
        if (kernelByGroup.find(gnamePair.first) == kernelByGroup.end()) continue;
        auto matchedTasks = FilterMatchedTasks(gnamePair.second);
        UpdateBandwidth(matchedTasks);
        for (const auto& t : matchedTasks)
        {
            kfcTaskData_.emplace_back(SerializeKfcTask(t, HcclType::MC2));
        }
    }

    // 用 task 首尾时间刷新 kernel 的 start/end
    for (auto& kernel : mc2Kernels)
    {
        auto timesIt = kernelTimes.find(MakeKernelKey(kernel));
        if (timesIt != kernelTimes.end())
        {
            kernel.start = timesIt->second.first;
            kernel.end = timesIt->second.second;
        }
    }
    // 更新 op_name（内部按 (start, end) 排序），序列化写入 KfcOPMap
    UpdateOpNameByGroupName(mc2Kernels, static_cast<double>(upstream.startTimeRawTimestamp));
    kfcOpData_.clear();
    for (const auto& k : mc2Kernels)
    {
        kfcOpData_.emplace_back(SerializeKfcOp(k, HcclType::MC2));
    }
}

std::pair<std::unordered_map<uint32_t, std::pair<std::string, int64_t>>,
          std::unordered_map<uint32_t, std::set<std::string>>>
KfcCalculator::GetMc2CommInfoData(const std::vector<Mc2CommInfo>& commInfo) const
{
    // 返回:
    //   aicpuInfo: {aicpu_stream_id: (group_name, rank_size)}
    //   commStreamIdGroupTable: {comm_stream_id: {group_names}}
    std::unordered_map<uint32_t, std::pair<std::string, int64_t>> aicpuInfo;
    std::unordered_map<uint32_t, std::set<std::string>> commStreamIdGroupTable;
    for (const auto& info : commInfo)
    {
        aicpuInfo[info.aicpuKfcStreamId] = std::make_pair(info.groupName, info.rankSize);
        for (auto streamId : info.commStreamIds)
        {
            commStreamIdGroupTable[streamId].insert(info.groupName);
        }
    }
    return std::make_pair(std::move(aicpuInfo), std::move(commStreamIdGroupTable));
}

KfcTaskRecord KfcCalculator::SerializeKfcTask(const DeviceHcclTask& data, HcclType source)
{
    // 对齐 Python _serialize_kfc_task：KfcTaskMap 25 列（HCCLTaskSingleDeviceMap + source）
    KfcTaskRecord record;
    record.modelId = data.modelId;
    record.indexId = data.indexId;
    record.hcclName = data.hcclName;
    record.groupName = data.groupName;
    record.planeId = data.planeId;
    record.timestamp = data.timestamp;
    record.duration = data.duration;
    record.opId = data.opId;
    record.isMaster = data.isMaster;
    record.streamId = data.streamId;
    record.taskId = data.taskId;
    record.contextId = data.contextId;
    record.batchId = data.batchId;
    record.size = data.size;
    record.bandwidth = data.bandwidth;
    record.localRank = data.localRank;
    record.remoteRank = data.remoteRank;
    record.rankSize = data.rankSize;
    record.transportType = data.transportType;
    record.dataType = data.dataType;
    record.linkType = data.linkType;
    record.rdmaType = data.rdmaType;
    record.notifyId = data.notifyId;
    record.iterId = data.iterId;
    record.source = source;
    return record;
}

KfcOpRecord KfcCalculator::SerializeKfcOp(const DeviceKfcOp& kernel, HcclType source)
{
    // 对齐 Python _serialize_kfc_op：KfcOPMap 16 列
    KfcOpRecord record;
    record.modelId = kernel.modelId;
    record.indexId = kernel.indexId;
    record.opName = kernel.opName;
    record.start = kernel.start;
    record.end = kernel.end;
    record.groupName = kernel.groupName;
    record.connectionId = kernel.kfcConnectionId;
    record.opType = kernel.opType;
    record.relay = kernel.relay;
    record.retry = kernel.retry;
    record.dataType = kernel.dataType;
    record.algType = kernel.algType;
    record.count = kernel.count;
    record.rankSize = kernel.rankSize;
    record.iterId = kernel.iterId;
    record.source = source;
    return record;
}

// 拓扑：AicpuPersistence（kfc 上游注入）/ AscendTaskPersistence（TopDownTask 同源）→ KfcCalculator。
// TopDownTask 由 AscendTaskAssociation 生产、HcclOp 由 LoadHostData 生产，经依赖链传递保证先于本流程执行。
REGISTER_PROCESS_SEQUENCE(KfcCalculator, true, AicpuPersistence, AscendTaskPersistence);
REGISTER_PROCESS_DEPENDENT_DATA(KfcCalculator, std::vector<KfcInfoData>, std::vector<MasterStreamTaskData>,
                                std::vector<HcclOp>, std::vector<TopDownTask>, AicpuOpNameMap,
                                std::vector<Mc2CommInfo>);
REGISTER_PROCESS_SUPPORT_CHIP(KfcCalculator, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
