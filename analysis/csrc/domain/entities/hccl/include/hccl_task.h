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
    uint64_t timestamp;
    uint64_t duration;
    std::string isDynamic;
    int64_t connectionId;
    int32_t relay;
    int32_t retry;
    std::string dataType;
    std::string algType;
    uint64_t count;
    std::string groupName;
    uint32_t rankSize;
};

struct HcclTask
{
    uint64_t modelId;
    int32_t indexId;
    std::string name;
    std::string groupName;
    int32_t planeId;
    uint64_t timestamp;
    double duration;
    uint32_t streamId;
    uint16_t taskId;
    uint32_t contextId;
    uint16_t batchId;
    uint16_t deviceId;
    uint16_t isMaster;
    uint32_t localRank;
    uint32_t remoteRank;
    uint32_t threadId;
    std::string transportType;
    double size;
    std::string dataType;
    std::string linkType;
    std::string notifyId;
    std::string rdmaType;
    uint32_t rankSize;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_ORI_HCCL_TASK_H
