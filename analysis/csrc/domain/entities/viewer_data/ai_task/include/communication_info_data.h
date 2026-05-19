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

#ifndef ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H
#define ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
struct CommunicationTaskData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t isMaster = 0;
    int32_t planeId = INT32_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t srcRank = UINT32_MAX;
    uint32_t dstRank = UINT32_MAX;
    uint64_t transportType = UINT64_MAX;
    uint64_t size = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t linkType = UINT64_MAX;
    uint64_t rdmaType = UINT64_MAX;
    double duration = 0.0;
    double durationEstimated = 0.0;
    double bandwidth = 0.0;
    std::string opName;
    std::string taskType;
    std::string groupName;
    std::string notifyId;
    std::string opKey;
};
struct CommunicationOpData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t rankSize = 0;
    int32_t relay = 0;
    int32_t retry = 0;
    uint32_t modelId = 0;
    uint64_t count = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    std::string opKey;
    std::string opName;
    std::string groupName;
    std::string algType;
    std::string opType;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H
