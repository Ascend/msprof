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

#include <algorithm>
#include <limits>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;

// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct DeviceHcclOp
{
    int64_t rankSize;
    int64_t connectionId;
    uint64_t modelId;
    uint64_t count;
    double start;
    double end;
    uint32_t iterId;
    int32_t relay;
    int32_t retry;
    int32_t indexId;
    uint32_t threadId;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string dataType;
    std::string algType;
    std::string groupName;
};

// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct DeviceHcclTask
{
    int64_t localRank = -1;
    int64_t remoteRank = -1;
    int64_t rankSize = -1;
    int64_t opId = 0;
    uint64_t modelId = 0;
    double size = 0.0;
    double bandwidth = 0.0;
    double timestamp = 0;
    double duration = 0;
    int32_t indexId = 0;
    int32_t planeId = 0;
    uint32_t batchId = 0;
    uint32_t taskId = 0;
    uint32_t iterId = 0;
    uint32_t streamId = 0;
    uint32_t contextId = 0;
    uint32_t threadId = 0;
    uint16_t isMaster = 0;
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

// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct HcclStatistics
{
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
    uint32_t count = 0;
    std::string opType;
};

class HcclCalculator : public Process
{
   protected:
    // 对齐 Python HcclCalculator 的静态方法（update_bandwidth / update_op_name_by_group_name /
    // generate_op_report_data），供 KfcCalculator 等通过继承复用。op_name 与报告聚合模板化，
    // 以兼容 DeviceHcclOp / DeviceKfcOp 两类 op 结构（Python 侧为鸭子类型）。
    static void UpdateBandwidth(std::vector<DeviceHcclTask>& tasks);
    // startTimeRawTimestamp 在调用处一次性转成 double，循环内直接比较，避免逐迭代 cast
    template <typename Op>
    static void UpdateOpNameByGroupName(std::vector<Op>& ops, double startTimeRawTimestamp);
    // Stat 模板化：默认 HcclStatistics，KfcCalculator 用 KfcOpStatistics（与 HcclStatistics 同形，
    // DataInventory 同一 C++ 类型仅可注入一次，mc2 报告需独立类型）
    template <typename Op, typename Stat = HcclStatistics>
    static bool GenerateOpReportData(const std::vector<Op>& ops, std::vector<Stat>& reportList,
                                     double startTimeRawTimestamp);

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
    static void CalculateTaskBandwidth(std::vector<DeviceHcclTask*> hcclTasks);
    static uint16_t GetJumpNum(const DeviceHcclTask& task);
    static double CalculateBandwidth(double size, double duration);
    static uint16_t FindConsecutivePayloadTask(std::vector<DeviceHcclTask*> tasks, size_t idx);
    bool GetHcclStatisticsData(uint64_t clockMonotonicRaw, SampleInfo sampleInfo);
    bool InjectData(DataInventory& inventory);

   private:
    std::vector<DeviceHcclOp> opData_;
    std::vector<DeviceHcclTask> taskData_;
    std::vector<HcclStatistics> statisticsData_;
};

template <typename Op>
void HcclCalculator::UpdateOpNameByGroupName(std::vector<Op>& ops, double startTimeRawTimestamp)
{
    // 对齐 Python HcclCalculator.update_op_name_by_group_name：前面多线程数据处理此处的 op 可能不保序，
    // 先按 (start, end) 重新排序
    std::sort(ops.begin(), ops.end(),
              [](const Op& op1, const Op& op2) { return std::tie(op1.start, op1.end) < std::tie(op2.start, op2.end); });
    //  if data start in warmup, index will be set -1
    //  else index++ when groupName in group_dict or group name set first
    std::unordered_map<std::string, int64_t> hcclGroup;
    for (auto& data : ops)
    {
        auto groupIt = hcclGroup.find(data.groupName);
        if (groupIt == hcclGroup.end())
        {
            // Python defaultdict(lambda: -1)
            groupIt = hcclGroup.emplace(data.groupName, -1).first;
        }
        if (data.end > startTimeRawTimestamp)
        {
            ++groupIt->second;
        }
        const size_t kSubGroupNamePos = 3;
        size_t subPoint = 0;
        if (data.groupName.size() > kSubGroupNamePos)
        {
            subPoint = data.groupName.size() - kSubGroupNamePos;
        }
        data.opName = Utils::Join("_", data.opName, data.groupName.substr(subPoint), std::to_string(groupIt->second),
                                  std::to_string(data.iterId));
    }
}

template <typename Op, typename Stat>
bool HcclCalculator::GenerateOpReportData(const std::vector<Op>& ops, std::vector<Stat>& reportList,
                                          double startTimeRawTimestamp)
{
    // 对齐 Python HcclCalculator.generate_op_report_data：按 op_type 聚合统计 op 数据，结果追加到 reportList
    std::unordered_map<std::string, Stat> statisticsTable;
    for (const auto& op : ops)
    {
        // 过滤 warmup 算子（end 早于采集起始时间）
        if (op.end < startTimeRawTimestamp) continue;
        auto& record = statisticsTable[op.opType];
        double duration = op.end - op.start;
        record.opType = op.opType;
        record.count++;
        record.totalTime += duration;
        record.max = std::max(duration, record.max);
        record.min = std::min(duration, record.min);
    }
    if (!Utils::Reserve(reportList, statisticsTable.size()))
    {
        ERROR("Reserve for op report data failed.");
        return false;
    }
    for (auto& data : statisticsTable)
    {
        // 业务保证count非零
        data.second.avg = static_cast<double>(data.second.totalTime) / data.second.count;
        reportList.emplace_back(data.second);
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H
