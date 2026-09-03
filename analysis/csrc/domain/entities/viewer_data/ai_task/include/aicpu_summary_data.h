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

#ifndef ANALYSIS_DOMAIN_AICPU_SUMMARY_DATA_H
#define ANALYSIS_DOMAIN_AICPU_SUMMARY_DATA_H

#include <cstdint>
#include <string>

namespace Analysis
{
namespace Domain
{
struct AicpuSummaryData
{
    uint16_t deviceId = UINT16_MAX;
    uint64_t timestampNs = UINT64_MAX;
    uint64_t endNs = UINT64_MAX;
    std::string nodeName;
    double computeTimeUs = 0;
    double memcpyTimeUs = 0;
    double taskTimeUs = 0;
    double dispatchTimeUs = 0;
    double totalTimeUs = 0;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
};

struct AicpuDpData
{
    uint64_t timestamp = 0;
    std::string action;
    std::string source;
    uint64_t bufferSize = 0;
};

struct AicpuMiData
{
    std::string nodeName;
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint64_t queueSize = 0;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_AICPU_SUMMARY_DATA_H
