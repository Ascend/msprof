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

#ifndef ANALYSIS_DOMAIN_ASCENDTASK_DATA_H
#define ANALYSIS_DOMAIN_ASCENDTASK_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
const std::string TASK_TYPE_MSTX = "MsTx";
struct AscendTaskData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    int32_t indexId = INT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    double duration = 0.0;
    double writeLatency = 0.0;
    std::string hostType;
    std::string deviceType;
    std::string taskType;
};

struct MsprofTxDeviceData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = 0;
    uint64_t connectionId = UINT64_MAX;
    double duration = 0.0;
    std::string taskType = TASK_TYPE_MSTX;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_ASCENDTASK_DATA_H
