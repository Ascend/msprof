/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/application/summary/task_time_assembler.h"

#include <algorithm>
#include <unordered_map>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Application;

TaskTimeAssembler::TaskTimeAssembler(const std::string& name, const std::string& profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "kernel_name",   "kernel_type",    "stream_id",
                "task_id",   "task_time(us)", "task_start(us)", "task_stop(us)"};
}

uint8_t TaskTimeAssembler::AssembleData(DataInventory& dataInventory)
{
    auto taskInfoData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto hostTaskData = dataInventory.GetPtr<std::vector<HostTask>>();
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (ascendTaskData == nullptr)
    {
        WARN("AscendTask not exists, can't export task time data.");
        return DATA_NOT_EXIST;
    }

    if (taskInfoData == nullptr)
    {
        WARN("No ge data collected, maybe the TaskInfo table is not created, now try to export data with no ge data");
    }
    else
    {
        FormatTaskInfoData(*taskInfoData);
    }
    if (hostTaskData == nullptr)
    {
        WARN(
            "No host data collected, maybe the HostTask table is not created, now try to export data with no host "
            "data");
    }
    else
    {
        FormatHostTaskData(*hostTaskData);
    }

    return AssembleTaskTime(*ascendTaskData);
}

void TaskTimeAssembler::FormatTaskInfoData(const std::vector<TaskInfoData>& taskInfoData)
{
    if (taskInfoData.empty())
    {
        WARN("task info data is empty, no task info data for task time, check ge info please.");
        return;
    }
    for (const auto& taskInfoDatum : taskInfoData)
    {
        TaskId taskId{taskInfoDatum.streamId, taskInfoDatum.batchId, taskInfoDatum.taskId, taskInfoDatum.contextId,
                      taskInfoDatum.deviceId};
        formatedTaskInfo_.insert({taskId, {taskInfoDatum.opName, taskInfoDatum.taskType}});
    }
}

void TaskTimeAssembler::FormatHostTaskData(const std::vector<HostTask>& hostTaskData)
{
    if (hostTaskData.empty())
    {
        WARN("host task data is empty, no host task data for task time, check host task please.");
        return;
    }
    for (const auto& taskInfoDatum : hostTaskData)
    {
        TaskId taskId{taskInfoDatum.streamId, taskInfoDatum.batchId, taskInfoDatum.taskId, taskInfoDatum.contextId,
                      taskInfoDatum.deviceId};
        opNameMap.insert({taskId, taskInfoDatum.kernelNameStr});
    }
}

std::vector<size_t> FilterAscendTaskData(const std::vector<AscendTaskData>& ascendTaskDatas)
{
    struct TupleHash
    {
        size_t operator()(const std::tuple<uint32_t, uint32_t, uint32_t>& key) const
        {
            const size_t h1 = std::hash<uint32_t>()(std::get<0>(key));
            const size_t h2 = std::hash<uint32_t>()(std::get<1>(key));
            const size_t h3 = std::hash<uint32_t>()(std::get<2>(key));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
    struct TupleEqual
    {
        bool operator()(const std::tuple<uint32_t, uint32_t, uint32_t>& a,
                        const std::tuple<uint32_t, uint32_t, uint32_t>& b) const
        {
            return std::get<0>(a) == std::get<0>(b) && std::get<1>(a) == std::get<1>(b) &&
                   std::get<2>(a) == std::get<2>(b);
        }
    };
    typedef std::tuple<uint32_t, uint32_t, uint32_t> KeyType;
    std::unordered_map<KeyType, std::vector<size_t>, TupleHash, TupleEqual> groups_dict;
    groups_dict.reserve(ascendTaskDatas.size() / 2);
    for (size_t i = 0; i < ascendTaskDatas.size(); ++i)
    {
        const auto& item = ascendTaskDatas[i];
        groups_dict[KeyType(item.streamId, item.taskId, item.batchId)].push_back(i);
    }
    std::vector<size_t> result;
    size_t estimated_size = 0;
    for (const auto& pair : groups_dict)
    {
        estimated_size += pair.second.size();
    }
    result.reserve(estimated_size);
    for (const auto& pair : groups_dict)
    {
        const auto& tasks = pair.second;
        if (tasks.size() == 1)
        {
            result.push_back(tasks[0]);
            continue;
        }
        bool mix = false;
        bool staticGraph = true;
        for (size_t idx : tasks)
        {
            const auto& task = ascendTaskDatas[idx];
            if (task.contextId > 0 && task.contextId != INVALID_CONTEXT_ID)
            {
                break;
            }
            if (task.contextId == 0)
            {
                mix = true;
                staticGraph = false;
            }
        }
        if (mix || staticGraph)
        {
            for (size_t idx : tasks)
            {
                const auto& task = ascendTaskDatas[idx];
                if (mix && task.contextId == 0)
                {
                    result.push_back(idx);
                }
                if (staticGraph)
                {
                    result.push_back(idx);
                }
            }
        }
    }
    return result;
}

struct TaskTimeKey
{
    size_t index = 0;
    uint64_t timestamp = 0;
};

uint8_t TaskTimeAssembler::AssembleTaskTime(const std::vector<AscendTaskData>& ascendTaskData)
{
    if (ascendTaskData.empty())
    {
        WARN("ascend task data is empty, no ascend task data for task time, check ascend task please.");
        ERROR("Can't match any task time data, failed to generate task_time_*.csv");
        return ASSEMBLE_FAILED;
    }
    const std::vector<size_t> indices = FilterAscendTaskData(ascendTaskData);
    std::vector<TaskTimeKey> keys;
    keys.reserve(indices.size());
    for (size_t idx : indices)
    {
        const auto& item = ascendTaskData[idx];
        TaskTimeKey key;
        key.index = idx;
        key.timestamp = item.timestamp;
        keys.emplace_back(std::move(key));
    }
    if (keys.empty())
    {
        ERROR("Can't match any task time data, failed to generate task_time_*.csv");
        return ASSEMBLE_FAILED;
    }
    std::sort(keys.begin(), keys.end(),
              [](const TaskTimeKey& lv, const TaskTimeKey& rv) { return lv.timestamp < rv.timestamp; });
    const std::string fileName = File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, TASK_TIME_SUMMARY_NAME});
    auto& consumer = DeliverableConsumer::Instance();
    consumer.StartCsv(fileName, headers_, {}, "", keys.size());
    std::vector<std::vector<std::string>> batch;
    batch.reserve(std::min(CSV_ROW_BATCH, keys.size()));
    auto flush = [&consumer, &fileName, &batch]()
    {
        if (batch.empty())
        {
            return;
        }
        consumer.AppendCsv(fileName, batch);
        batch.clear();
    };
    for (const auto& key : keys)
    {
        const auto& ascendTaskDatum = ascendTaskData[key.index];
        TaskId taskId{ascendTaskDatum.streamId, ascendTaskDatum.batchId, ascendTaskDatum.taskId,
                      ascendTaskDatum.contextId, ascendTaskDatum.deviceId};
        auto taskInfoIt = formatedTaskInfo_.find(taskId);
        const std::string opName = [&]() -> std::string
        {
            auto opNameIt = opNameMap.find(taskId);
            if (opNameIt != opNameMap.end())
            {
                return opNameIt->second;
            }
            if (taskInfoIt != formatedTaskInfo_.end())
            {
                return taskInfoIt->second.first;
            }
            return NA;
        }();
        const std::string taskType = (taskInfoIt != formatedTaskInfo_.end() && taskInfoIt->second.second != NA)
                                         ? taskInfoIt->second.second
                                         : ascendTaskDatum.taskType;
        std::vector<std::string> row;
        row.reserve(headers_.size());
        row.emplace_back(std::to_string(ascendTaskDatum.deviceId));
        row.emplace_back(opName);
        row.emplace_back(taskType);
        row.emplace_back(std::to_string(taskId.streamId));
        row.emplace_back(std::to_string(taskId.taskId));
        row.emplace_back(DivideByPowersOfTenWithPrecision(static_cast<uint64_t>(ascendTaskDatum.duration)));
        row.emplace_back(DivideByPowersOfTenWithPrecision(ascendTaskDatum.timestamp, true));
        row.emplace_back(DivideByPowersOfTenWithPrecision(ascendTaskDatum.end, true));
        batch.emplace_back(std::move(row));
        if (batch.size() >= CSV_ROW_BATCH)
        {
            flush();
        }
    }
    flush();
    consumer.FinishCsv(fileName);
    return ASSEMBLE_SUCCESS;
}

}  // namespace Application
}  // namespace Analysis
