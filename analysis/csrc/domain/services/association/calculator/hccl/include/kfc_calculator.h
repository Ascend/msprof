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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_KFC_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_KFC_CALCULATOR_H

#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hccl/include/kfc_task.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"

namespace Analysis
{
namespace Domain
{

// 对齐 Python msmodel/add_info/kfc_info_model.py 中 aicpu kernel(KfcInfo) 结构：
// kfc 算子原始数据，HCCL/MC2 两个分支共用
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct DeviceKfcOp
{
    uint64_t modelId = 0;
    uint64_t count = 0;
    int64_t rankSize = 0;
    int64_t kfcConnectionId = 0;  // kfc_connection_id
    int64_t connectionId = 0;     // 匹配到 HCCL_OP 后回填 connection_id，MC2 下等于 kfc_connection_id
    double start = 0.0;
    double end = 0.0;
    int32_t indexId = 0;
    int32_t relay = 0;
    int32_t retry = 0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t contextId = 0;
    uint32_t batchId = 0;
    uint32_t iterId = 0;
    std::string opName;
    std::string groupName;
    std::string opType;
    std::string dataType;
    std::string algType;
};

// 对齐 TABLE_AICPU_MASTER_STREAM_HCCL_TASK：主流/LAST/FIRST aicpu 任务，用于修正 kernel 的 start/end
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct DeviceMasterStreamHcclTask
{
    double timestamp = 0.0;
    double duration = 0.0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t batchId = 0;
    uint32_t aicpuStreamId = 0;
    uint32_t aicpuTaskId = 0;
    uint32_t aicpuBatchId = 0;
    uint16_t taskType = 0;  // 0=FIRST, 1=LAST
};

// 对齐 TABLE_HCCL_OP（主机侧）：用于按 kfc_connection_id 判定 HCCL/MC2
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct DeviceHcclHostOp
{
    int64_t connectionId = 0;
    std::string kfcConnectionIds;  // 逗号分隔，一行对应多个 kfc_connection_id
    std::string opName;
    std::string groupName;
};

// _serialize_kfc_task 的 25 列输出（对齐 KfcTaskMap）
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct KfcTaskRecord
{
    uint64_t modelId = 0;
    double timestamp = 0.0;
    double duration = 0.0;
    int64_t opId = 0;
    double size = 0.0;
    double bandwidth = 0.0;
    int64_t localRank = -1;
    int64_t remoteRank = -1;
    int64_t rankSize = -1;
    int32_t indexId = 0;
    int32_t planeId = 0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t contextId = 0;
    uint32_t batchId = 0;
    uint32_t iterId = 0;
    HcclType source = HcclType::INVALID;  // 对齐 CommunicationTaskData.source：HCCL / MC2
    uint16_t isMaster = 0;
    std::string hcclName;
    std::string groupName;
    std::string transportType;
    std::string dataType;
    std::string linkType;
    std::string rdmaType;
    std::string notifyId;
};

// _serialize_kfc_op 的 16 列输出（对齐 KfcOPMap）
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct KfcOpRecord
{
    uint64_t modelId = 0;
    double start = 0.0;
    double end = 0.0;
    int64_t connectionId = 0;  // kfc_connection_id
    uint64_t count = 0;
    int64_t rankSize = 0;
    int32_t indexId = 0;
    int32_t relay = 0;
    int32_t retry = 0;
    uint32_t iterId = 0;
    HcclType source = HcclType::INVALID;  // 对齐 CommunicationOpData.source：MC2
    std::string opName;
    std::string groupName;
    std::string opType;
    std::string dataType;
    std::string algType;
};

// 对齐 Python 报表数据：与 HcclStatistics 同形，KfcCalculator 的 mc2 算子报告。
// 不复用 std::vector<HcclStatistics>：DataInventory 同一 C++ 类型仅可注入一次，
// HcclCalculator 已注入该类型，KfcCalculator 需独立类型避免注入冲突。
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct KfcOpStatistics
{
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
    uint32_t count = 0;
    std::string opType;
};

// 上游数据：由 ProcessEntry 从 DataInventory / host DB 组装后传入 Calculate()
struct KfcUpstreamData
{
    std::vector<DeviceKfcOp> aicpuKernels;  // KfcInfo.get_kfc_op_data（已按 start_time 排序）
    std::vector<DeviceHcclTask> kfcTasks;   // get_kfc_info_with_task_by_stream_ids /
                                            // get_ascend_task_with_kfc_defaults（已按 comm_stream 过滤）
    std::vector<DeviceMasterStreamHcclTask> masterStreamHcclTask;  // TABLE_AICPU_MASTER_STREAM_HCCL_TASK
    std::vector<DeviceHcclHostOp> hcclHostOps;                     // HcclViewModel.get_hccl_ops（TABLE_HCCL_OP）
    std::vector<Mc2CommInfo> mc2CommInfo;                          // TABLE_MC2_COMM_INFO
    uint64_t startTimeRawTimestamp = 0;                            // 对齐 InfoConfReader 采集起始时间(dev raw)
    bool isLevel0 = false;                                         // 对齐 InfoConfReader.is_level0()
};

class KfcCalculator : public HcclCalculator
{
   public:
    static constexpr uint16_t FIRST_TASK_TYPE = 0;  // 对齐 Python KfcCalculator.FIRST_TASK_TYPE
    static constexpr uint16_t LAST_TASK_TYPE = 1;   // 对齐 Python KfcCalculator.LAST_TASK_TYPE

    KfcCalculator() = default;
    ~KfcCalculator() override = default;

    // 入口：对齐 Python calculate()，upstream 由 ProcessEntry 组装。
    // 返回 false 表示 mc2 op 报告聚合失败（对齐 HcclCalculator::GetHcclStatisticsData 的失败透传）
    bool Calculate(const KfcUpstreamData& upstream);

    const std::vector<KfcTaskRecord>& GetKfcTaskData() const { return kfcTaskData_; }
    const std::vector<KfcOpRecord>& GetKfcOpData() const { return kfcOpData_; }
    const std::vector<KfcOpStatistics>& GetMc2OpReportData() const { return mc2OpReportData_; }

   private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;

    // 上游数据组装（对齐 Python get_kfc_op_data / get_kfc_info_with_task_by_stream_ids /
    // get_ascend_task_with_kfc_defaults / get_mc2_comm_info_data / get_aicpu_master_stream_hccl_task）
    bool BuildUpstreamData(DataInventory& dataInventory, const DeviceContext& deviceContext,
                           const std::vector<TopDownTask>& ascendTasks, const std::vector<HcclOp>& hcclOps,
                           KfcUpstreamData& upstream);
    bool InjectData(DataInventory& inventory);
    // 组装 aicpu kernel（对齐 get_kfc_op_data：GE_TASK(注入的 op_name) + ASCEND_TASK 内存 join）
    void BuildAicpuKernels(const std::vector<TopDownTask>& ascendTasks, const AicpuOpNameMap& opNameByTask,
                           std::vector<DeviceKfcOp>& aicpuKernels);
    // 组装 kfcTasks（对齐 get_kfc_info_with_task_by_stream_ids / get_ascend_task_with_kfc_defaults）
    void BuildKfcTasks(bool isLevel0, const std::vector<KfcInfoData>& kfcInfos,
                       const std::vector<TopDownTask>& ascendTasks, const std::vector<Mc2CommInfo>& commInfo,
                       std::vector<DeviceHcclTask>& kfcTasks);

    // 对齐 Python get_hccl_and_mc2_op / get_kfc_host_hccl_op
    void GetHcclAndMc2Op(const std::vector<DeviceKfcOp>& aicpuKernels, const std::vector<DeviceHcclHostOp>& hcclHostOps,
                         std::vector<DeviceKfcOp>& hcclKernels, std::vector<DeviceKfcOp>& mc2Kernels) const;
    std::unordered_map<int64_t, const DeviceHcclHostOp*> GetKfcHostHcclOp(
        const std::vector<DeviceHcclHostOp>& hcclHostOps) const;

    // 对齐 Python _refine_kernel_times_with_master_stream / _assign_op_and_plane
    void RefineKernelTimesWithMasterStream(const std::vector<DeviceMasterStreamHcclTask>& masterStreamTasks,
                                           const std::vector<DeviceHcclTask>& kfcTaskData,
                                           std::vector<DeviceKfcOp>& hcclKernels) const;
    std::map<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>, std::pair<double, double>> AssignOpAndPlane(
        std::map<std::string, std::vector<DeviceHcclTask>>& groupedTasks,
        const std::map<std::string, std::vector<DeviceKfcOp>>& kernelByGroup, int planeIdBase) const;

    // 对齐 Python generate_hccl_kernels / generate_mc2_kernels
    void GenerateHcclKernels(std::vector<DeviceKfcOp>& hcclKernels, bool isLevel0, const KfcUpstreamData& upstream);
    void GenerateMc2Kernels(std::vector<DeviceKfcOp>& mc2Kernels, bool isLevel0, const KfcUpstreamData& upstream);

    // 对齐 Python get_mc2_comm_info_data：{aicpu_stream_id -> (group_name, rank_size)} + {comm_stream_id ->
    // group_names}
    std::pair<std::unordered_map<uint32_t, std::pair<std::string, int64_t>>,
              std::unordered_map<uint32_t, std::set<std::string>>>
    GetMc2CommInfoData(const std::vector<Mc2CommInfo>& commInfo) const;

    // 对齐 Python 各 @staticmethod
    static std::map<std::string, std::vector<DeviceKfcOp>> GroupKernelsByGroupName(
        const std::vector<DeviceKfcOp>& kernels);
    static std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t> MakeKernelKey(const DeviceKfcOp& kernel);
    static std::map<std::string, std::vector<DeviceHcclTask>> GroupTasksByCommStream(
        const std::vector<DeviceHcclTask>& taskData,
        const std::unordered_map<uint32_t, std::set<std::string>>& commStreamIdGroupTable);
    static std::vector<DeviceHcclTask> FilterMatchedTasks(const std::vector<DeviceHcclTask>& tasks);
    static KfcTaskRecord SerializeKfcTask(const DeviceHcclTask& data, HcclType source);
    static KfcOpRecord SerializeKfcOp(const DeviceKfcOp& kernel, HcclType source);

   private:
    std::vector<KfcTaskRecord> kfcTaskData_;
    std::vector<KfcOpRecord> kfcOpData_;
    std::vector<KfcOpStatistics> mc2OpReportData_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_KFC_CALCULATOR_H
