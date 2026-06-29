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

#include "analysis/csrc/application/summary/task_time_assembler.h"

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

    AssembleTaskTime(*ascendTaskData);

    if (res_.empty())
    {
        ERROR("Can't match any task time data, failed to generate task_time_*.csv");
        return ASSEMBLE_FAILED;
    }
    // keep res in the order of device && start time
    sortRes();

    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, TASK_TIME_SUMMARY_NAME}), {});

    return ASSEMBLE_SUCCESS;
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
        TaskId taskId{static_cast<uint16_t>(taskInfoDatum.streamId), static_cast<uint16_t>(taskInfoDatum.batchId),
                      taskInfoDatum.taskId, taskInfoDatum.contextId, taskInfoDatum.deviceId};
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
        TaskId taskId{static_cast<uint16_t>(taskInfoDatum.streamId), taskInfoDatum.batchId, taskInfoDatum.taskId,
                      taskInfoDatum.contextId, taskInfoDatum.deviceId};
        opNameMap.insert({taskId, taskInfoDatum.kernelNameStr});
    }
}

std::vector<AscendTaskData> FilterAscendTaskData(const std::vector<AscendTaskData>& ascendTaskDatas)
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
    std::unordered_map<KeyType, std::vector<AscendTaskData>, TupleHash, TupleEqual> groups_dict;
    groups_dict.reserve(ascendTaskDatas.size() / 2);
    for (const auto& item : ascendTaskDatas)
    {
        groups_dict[KeyType(item.streamId, item.taskId, item.batchId)].push_back(item);
    }
    std::vector<AscendTaskData> result;
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
        for (const auto& task : tasks)
        {
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
            for (const auto& task : tasks)
            {
                if (mix && task.contextId == 0)
                {
                    result.push_back(task);
                }
                if (staticGraph)
                {
                    result.push_back(task);
                }
            }
        }
    }
    return result;
}

void TaskTimeAssembler::AssembleTaskTime(const std::vector<AscendTaskData>& ascendTaskData)
{
    if (ascendTaskData.empty())
    {
        WARN("ascend task data is empty, no ascend task data for task time, check ascend task please.");
        return;
    }

    const std::string DIVIDE_CHAR = "\t";
    for (auto& ascendTaskDatum : FilterAscendTaskData(ascendTaskData))
    {
        TaskId taskId{static_cast<uint16_t>(ascendTaskDatum.streamId), static_cast<uint16_t>(ascendTaskDatum.batchId),
                      ascendTaskDatum.taskId, ascendTaskDatum.contextId, ascendTaskDatum.deviceId};
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

        auto row = {std::to_string(ascendTaskDatum.deviceId),
                    opName,
                    taskType,
                    std::to_string(taskId.streamId),
                    std::to_string(taskId.taskId),
                    DivideByPowersOfTenWithPrecision(static_cast<uint64_t>(ascendTaskDatum.duration)),
                    DivideByPowersOfTenWithPrecision(ascendTaskDatum.timestamp) + DIVIDE_CHAR,
                    DivideByPowersOfTenWithPrecision(ascendTaskDatum.end) + DIVIDE_CHAR};
        res_.emplace_back(row);
    }
}

void TaskTimeAssembler::sortRes()
{
    // 业务可以保证headers_有值的情况下，device_id与start_time的下标是有效的
    const auto DEVICE_INDEX = 0;
    const auto START_TIME_INDEX = 6;
    using rowType = std::vector<std::string>;
    std::sort(res_.begin(), res_.end(),
              [DEVICE_INDEX, START_TIME_INDEX](const rowType& lv, const rowType& rv)
              {
                  if (lv.at(DEVICE_INDEX) == rv.at(DEVICE_INDEX))
                  {
                      return lv.at(START_TIME_INDEX) < rv.at(START_TIME_INDEX);
                  }
                  return lv.at(START_TIME_INDEX) < rv.at(START_TIME_INDEX);
              });
}

}  // namespace Application
}  // namespace Analysis
