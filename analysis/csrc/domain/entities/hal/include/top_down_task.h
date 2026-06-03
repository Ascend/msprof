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

#ifndef ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H
#define ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H

#include <stdint.h>

#include <string>

struct TopDownTask
{
    bool isFirst = false;  // taskId-streamId-batchId-contextId都相同的任务中的第一个

    uint16_t batchId = 0;
    uint32_t taskId = 0;
    uint32_t streamId = 0;
    uint32_t contextId = 0;
    int32_t indexId = 0;
    std::string deviceTaskType = "";
    std::string hostTaskType = "";
    uint64_t modelId = 0;
    int64_t connectionId = 0;
    double startTime = 0;
    double endTime = 0;

    TopDownTask() = default;
    TopDownTask(bool isFirst, uint32_t taskId, uint16_t batchId, uint32_t streamId, uint32_t contextId, int32_t indexId,
                std::string deviceType, std::string hostType, uint64_t modelId, int64_t connectionId, double startTime,
                double endTime)
        : taskId(taskId),
          batchId(batchId),
          streamId(streamId),
          contextId(contextId),
          indexId(indexId),
          deviceTaskType(deviceType),
          hostTaskType(hostType),
          modelId(modelId),
          connectionId(connectionId),
          startTime(startTime),
          endTime(endTime),
          isFirst(isFirst)
    {
    }
};
#endif  // ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H
