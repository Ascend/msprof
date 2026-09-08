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

#ifndef MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H
#define MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis
{
namespace Domain
{
struct UnifiedTaskPmu
{
    uint16_t deviceId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t subtaskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    double value = 0.0;
    std::string header;
    // 算子 context PMU 时间，换算后的 wall-clock ns，随每行 metric 重复落盘到 TASK_PMU_INFO.timestampNs
    uint64_t timestamp = 0;

    UnifiedTaskPmu() = default;
    UnifiedTaskPmu(uint16_t deviceId_, uint32_t streamId_, uint32_t taskId_, uint32_t subtaskId_, uint32_t batchId_,
                   std::string header_, double value_, uint64_t timestamp_ = 0)
        : deviceId(deviceId_),
          streamId(streamId_),
          taskId(taskId_),
          subtaskId(subtaskId_),
          batchId(batchId_),
          header(std::move(header_)),
          value(value_),
          timestamp(timestamp_) {};
};

struct UnifiedSampleTimelinePmu : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t coreId = UINT16_MAX;
    double usage = 0.0;
    double freq = 0.0;
    uint64_t totalCycle = UINT64_MAX;
    uint64_t coreType = UINT64_MAX;

    UnifiedSampleTimelinePmu() = default;
    UnifiedSampleTimelinePmu(uint16_t deviceId_, uint64_t timestamp_, uint64_t totalCycle_, double usage_, double freq_,
                             uint16_t coreId_, uint64_t coreType_)
        : BasicData(timestamp_),
          deviceId(deviceId_),
          totalCycle(totalCycle_),
          usage(usage_),
          freq(freq_),
          coreId(coreId_),
          coreType(coreType_) {};
};

struct UnifiedSampleSummaryPmu
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t coreId = UINT16_MAX;
    double value = 0.0;
    uint64_t coreType = UINT64_MAX;
    std::string metric;

    UnifiedSampleSummaryPmu() = default;
    UnifiedSampleSummaryPmu(uint16_t deviceId_, std::string metric_, double value_, uint16_t coreId_,
                            uint64_t coreType_)
        : deviceId(deviceId_), metric(std::move(metric_)), value(value_), coreId(coreId_), coreType(coreType_) {};
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H
