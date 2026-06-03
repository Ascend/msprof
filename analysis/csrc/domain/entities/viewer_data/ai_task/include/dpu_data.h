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

#ifndef ANALYSIS_DOMAIN_DPU_DATA_H
#define ANALYSIS_DOMAIN_DPU_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct DPUData : public BasicData
{
    bool isHccl = false;
    uint16_t npuDeviceId = UINT16_MAX;
    uint16_t dpuDeviceId = UINT16_MAX;
    uint16_t localRank = UINT16_MAX;
    uint16_t remoteRank = UINT16_MAX;
    uint16_t planeId = UINT16_MAX;
    uint16_t streamId = UINT16_MAX;
    uint32_t aicpuTaskId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t threadId = UINT32_MAX;
    uint32_t rankSize = UINT32_MAX;
    uint64_t dataSize = UINT64_MAX;
    uint64_t endTime = UINT64_MAX;
    double durationEstimated = 0.0;
    std::string taskType;
    std::string dataType;
    std::string dstAddr;
    std::string srcAddr;
    std::string groupName;
    std::string groupNameId;
    std::string linkType;
    std::string notifyId;
    std::string opName;
    std::string opType;
    std::string rdmaType;
    std::string transportType;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_DPU_DATA_H
