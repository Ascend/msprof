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

#include "analysis/csrc/domain/data_process/ai_task/op_statistic_processor.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>

#include "analysis/csrc/application/database/db_constant.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Analysis::Utils;

namespace
{
struct OpStatisticAggKey
{
    uint16_t deviceId;
    std::string opType;
    std::string taskType;

    bool operator<(const OpStatisticAggKey &other) const
    {
        if (deviceId != other.deviceId)
        {
            return deviceId < other.deviceId;
        }
        if (opType != other.opType)
        {
            return opType < other.opType;
        }
        return taskType < other.taskType;
    }
};

struct OpStatisticAgg
{
    uint64_t count = 0;
    double totalTimeNs = 0.0;
    double minNs = std::numeric_limits<double>::infinity();
    double maxNs = 0.0;
};

using OpStatisticAggMap = std::map<OpStatisticAggKey, OpStatisticAgg>;
using DeviceTotalNsMap = std::unordered_map<uint16_t, double>;

// Match the Python pipeline: op_report excludes communication tasks before aggregation, while N/A,
// WRITE_BACK and INVALID remain in the ratio denominator and are filtered only from op_statistic.csv.
bool isOpReportRequired(const AssociatedTaskData &associatedTask)
{
    return associatedTask.taskInfo->taskType != TASK_TYPE_COMMUNICATION &&
           associatedTask.taskInfo->taskType != TASK_TYPE_HCCL_AI_CPU;
}

bool isOpStatisticRequired(const OpStatisticAggKey &key)
{
    return key.opType != NA && key.taskType != TASK_TYPE_WRITE_BACK && key.taskType != TASK_TYPE_INVALID;
}

double getTaskDurationNs(const AscendTaskData &ascendTask)
{
    return static_cast<double>(ascendTask.end - ascendTask.timestamp);
}

void aggregate(const AssociatedTaskCollection &associatedTasks, OpStatisticAggMap &aggregatedData,
               DeviceTotalNsMap &deviceTotalNs)
{
    for (const auto &associatedTask : associatedTasks.records)
    {
        if (!isOpReportRequired(associatedTask))
        {
            continue;
        }
        const auto &taskInfo = *associatedTask.taskInfo;
        const auto &ascendTask = *associatedTask.ascendTask;
        const double durationNs = getTaskDurationNs(ascendTask);
        OpStatisticAggKey key{ascendTask.deviceId, taskInfo.opType, taskInfo.taskType};
        OpStatisticAgg &aggregated = aggregatedData[key];
        ++aggregated.count;
        aggregated.totalTimeNs += durationNs;
        aggregated.minNs = std::min(aggregated.minNs, durationNs);
        aggregated.maxNs = std::max(aggregated.maxNs, durationNs);
        deviceTotalNs[ascendTask.deviceId] += durationNs;
    }
}

double getDeviceTotalNs(const DeviceTotalNsMap &deviceTotalNs, uint16_t deviceId)
{
    auto total = deviceTotalNs.find(deviceId);
    return total == deviceTotalNs.end() ? 0.0 : total->second;
}

bool formatData(const OpStatisticAggMap &aggregatedData, const DeviceTotalNsMap &deviceTotalNs,
                std::vector<OpStatisticData> &result)
{
    if (!Reserve(result, aggregatedData.size()))
    {
        ERROR("Reserve for op statistic data failed.");
        return false;
    }
    const double nsToUs = static_cast<double>(NS_TO_US);
    for (const auto &item : aggregatedData)
    {
        const OpStatisticAggKey &key = item.first;
        const OpStatisticAgg &aggregated = item.second;
        if (!isOpStatisticRequired(key))
        {
            continue;
        }

        const double totalNs = getDeviceTotalNs(deviceTotalNs, key.deviceId);
        const double ratio = totalNs > 0.0 ? aggregated.totalTimeNs * static_cast<double>(PERCENTAGE) / totalNs : 0.0;

        OpStatisticData data;
        data.deviceId = key.deviceId;
        data.opType = key.opType;
        data.coreType = key.taskType;
        data.count = std::to_string(aggregated.count);
        data.totalTime = RoundToDecimalPlaces(aggregated.totalTimeNs / nsToUs);
        data.min = RoundToDecimalPlaces(aggregated.minNs / nsToUs);
        data.avg = aggregated.count == 0
                       ? 0.0
                       : RoundToDecimalPlaces(aggregated.totalTimeNs / static_cast<double>(aggregated.count) / nsToUs);
        data.max = RoundToDecimalPlaces(aggregated.maxNs / nsToUs);
        data.ratio = RoundToDecimalPlaces(ratio);
        result.push_back(std::move(data));
    }
    return true;
}
}  // namespace

OpStatisticProcessor::OpStatisticProcessor(const std::string &profPaths) : DataProcessor(profPaths) {}

bool OpStatisticProcessor::Process(DataInventory &dataInventory)
{
    auto associatedTasks = dataInventory.GetPtr<AssociatedTaskCollection>();
    if (associatedTasks == nullptr)
    {
        WARN("Op Statistic source data not exist.");
        return true;
    }

    OpStatisticAggMap aggregatedData;
    DeviceTotalNsMap deviceTotalNs;
    aggregate(*associatedTasks, aggregatedData, deviceTotalNs);
    std::vector<OpStatisticData> result;
    if (!formatData(aggregatedData, deviceTotalNs, result))
    {
        return false;
    }
    if (!SaveToDataInventory<OpStatisticData>(std::move(result), dataInventory, PROCESSOR_NAME_OP_STATISTIC))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_OP_STATISTIC);
        return false;
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
