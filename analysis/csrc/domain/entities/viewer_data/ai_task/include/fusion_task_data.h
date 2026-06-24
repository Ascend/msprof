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

#ifndef ANALYSIS_DOMAIN_FUSION_TASK_DATA_H
#define ANALYSIS_DOMAIN_FUSION_TASK_DATA_H

#include <stdint.h>

#include <string>

namespace Analysis
{
namespace Domain
{

struct FusionTaskTimelineData
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    double duration = 0.0;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t accId = UINT32_MAX;
    uint32_t missionId = UINT32_MAX;
    uint32_t ccuDieId = UINT32_MAX;
    uint16_t deviceId = UINT16_MAX;
    bool isCcu = false;
    bool hasCcuDie = false;
    std::string taskType;
    std::string fusionTaskType;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_FUSION_TASK_DATA_H
