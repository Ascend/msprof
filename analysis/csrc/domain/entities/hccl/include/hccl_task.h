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
struct HcclOp
{
    uint16_t deviceId;
    uint64_t modelId;
    int32_t indexId;
    uint32_t threadId;
    std::string opName;
    std::string taskType;
    std::string opType;
    int64_t connectionId;
    int32_t relay;
    int32_t retry;
    std::string dataType;
    std::string algType;
    uint64_t count;
    std::string groupName;
};

struct HcclTask
{
    uint64_t modelId;
    int32_t indexId;
    std::string name;
    std::string groupName;
    int32_t planeId;
    uint64_t timestamp;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t contextId;
    uint32_t batchId;
    uint16_t deviceId;
    uint16_t isMaster;
    int64_t localRank;
    int64_t remoteRank;
    uint32_t threadId;
    std::string transportType;
    double size;
    std::string dataType;
    std::string linkType;
    std::string notifyId;
    std::string rdmaType;
    int64_t rankSize;
    int64_t opId;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_ORI_HCCL_TASK_H
