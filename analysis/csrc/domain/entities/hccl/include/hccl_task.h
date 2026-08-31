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

#ifndef MSPROF_ANALYSIS_ORI_HCCL_TASK_H
#define MSPROF_ANALYSIS_ORI_HCCL_TASK_H

#include <stdint.h>

#include <string>

namespace Analysis
{
namespace Domain
{
// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct HcclOp
{
    uint64_t modelId;
    int64_t connectionId;
    uint64_t count;
    int32_t indexId;
    uint32_t threadId;
    int32_t relay;
    int32_t retry;
    uint16_t deviceId;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string dataType;
    std::string algType;
    std::string groupName;
    std::string kfcConnectionIds;  // 逗号分隔，一行对应多个 kfc_connection_id
};

// 成员按对齐降序排列（8B → 4B → 2B → string），减少结构体内存填充
struct HcclTask
{
    uint64_t modelId;
    uint64_t timestamp;
    int64_t localRank;
    int64_t remoteRank;
    int64_t rankSize;
    int64_t opId;
    double size;
    int32_t indexId;
    int32_t planeId;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t contextId;
    uint32_t batchId;
    uint32_t threadId;
    uint16_t deviceId;
    uint16_t isMaster;
    std::string name;
    std::string groupName;
    std::string transportType;
    std::string dataType;
    std::string linkType;
    std::string notifyId;
    std::string rdmaType;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_ORI_HCCL_TASK_H
