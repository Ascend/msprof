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

#include <functional>
#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{
enum class HcclType
{
    HCCL = 0,
    MC2 = 1,
    INVALID = 65535
};

struct CommunicationTaskData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t isMaster = 0;
    HcclType source = HcclType::INVALID;
    int32_t planeId = INT32_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    int64_t srcRank = -1;
    int64_t dstRank = -1;
    int64_t opId = -1;
    uint32_t iterId = 0;
    std::string hcclName;
    std::string transportType;
    uint64_t size = UINT64_MAX;
    std::string dataType;
    std::string linkType;
    std::string rdmaType;
    double duration = 0.0;
    double durationEstimated = 0.0;
    double bandwidth = 0.0;
    std::string taskType;
    std::string groupName;
    std::string notifyId;
};
struct CommunicationOpData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    HcclType source = HcclType::INVALID;
    int64_t rankSize = -1;
    int32_t relay = 0;
    int32_t retry = 0;
    uint32_t modelId = 0;
    int64_t connectionId = -1;
    uint32_t iterId = 0;
    uint64_t count = UINT64_MAX;
    std::string dataType;
    uint64_t end = UINT64_MAX;
    std::string opName;
    std::string groupName;
    std::string algType;
    std::string opType;
};
}  // namespace Domain
}  // namespace Analysis

namespace std
{
template <>
struct hash<Analysis::Domain::HcclType>
{
    size_t operator()(const Analysis::Domain::HcclType& type) const noexcept
    {
        return std::hash<int>()(static_cast<int>(type));
    }
};
}  // namespace std

#endif  // ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H
