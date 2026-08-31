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

#ifndef MSPROF_ANALYSIS_ORI_KFC_TASK_H
#define MSPROF_ANALYSIS_ORI_KFC_TASK_H

#include <stdint.h>

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Domain
{
// 对齐 kfc_info.db 的 KfcInfo 表：KfcCalculator 构建 kfcTasks（非 level0 分支）所需的列。
// 由 AicpuPersistence 在 ComputeAicpuBatchId 后注入 DataInventory，KfcCalculator 直接消费，
// 与 KfcInfo 落盘行同源同值（不重复走 kfc_info.db 读取）。
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct KfcInfoData
{
    int64_t localRank = -1;
    int64_t remoteRank = -1;
    int64_t rankSize = -1;
    double size = 0.0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t contextId = UINT32_MAX;  // GE 默认 context（与 KfcInfo 落库值一致）
    uint32_t batchId = 0;             // ComputeAicpuBatchId 修正后的值
    int32_t planeId = 0;
    std::string hcclName;  // op_name
    std::string notifyId;
    std::string opType;
    std::string dataType;
    std::string linkType;
    std::string transportType;
    std::string rdmaType;
};

// 对齐 kfc_info.db 的 AicpuMasterStreamHcclTask 表：主流/LAST/FIRST aicpu 任务，
// 用于修正 aicpu kernel 的 start/end。由 AicpuPersistence 注入 DataInventory。
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct MasterStreamTaskData
{
    double timeStamp = 0.0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t batchId = 0;  // ComputeAicpuBatchId 修正后
    uint32_t aicpuStreamId = 0;
    uint32_t aicpuTaskId = 0;
    uint32_t aicpuBatchId = 0;  // ComputeAicpuBatchId aicpu 侧（device flip）修正后
    uint16_t taskType = 0;      // 0=FIRST, 1=LAST
};

// ge_info.db TaskInfo 中 AicpuKernel 的 op_name：TaskId(stream_id, batch_id, task_id, context_id) → op_name。
// 由 LoadHostData 读取注入 DataInventory，KfcCalculator 消费与 ASCEND_TASK 做内存 join
using AicpuOpNameMap = std::map<TaskId, std::string>;

// 对齐 host mc2_comm_info.db 的 Mc2CommInfo 表：mc2 通信域信息，由 LoadHostData 读取注入 DataInventory
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct Mc2CommInfo
{
    int64_t rankSize = 0;
    uint32_t aicpuKfcStreamId = 0;
    std::string groupName;
    std::vector<uint32_t> commStreamIds;  // 由 LoadHostData 将逗号分隔串解析后的展开流
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_ORI_KFC_TASK_H
