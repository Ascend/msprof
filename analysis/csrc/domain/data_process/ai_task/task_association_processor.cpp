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

#include "analysis/csrc/domain/data_process/ai_task/task_association_processor.h"

#include <unordered_map>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/associated_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Analysis::Utils;

namespace
{
template <typename TaskData>
TaskId makeTaskId(const TaskData &taskData)
{
    return TaskId{taskData.streamId, taskData.batchId, taskData.taskId, taskData.contextId, taskData.deviceId};
}

bool isOpSummaryRequired(const TaskInfoData &taskInfo)
{
    const bool isExcludedTaskType =
        taskInfo.taskType == TASK_TYPE_WRITE_BACK || taskInfo.taskType == TASK_TYPE_INVALID ||
        taskInfo.taskType == TASK_TYPE_HCCL_AI_CPU || taskInfo.taskType == TASK_TYPE_COMMUNICATION;
    const bool isCommunicationAivKernel =
        taskInfo.taskType == TASK_TYPE_COMMUNICATION && EndsWith(taskInfo.opName, AIV_KERNEL);
    return !isExcludedTaskType || isCommunicationAivKernel;
}

bool associateTasks(const std::vector<TaskInfoData> &taskInfoData, const std::vector<AscendTaskData> &ascendTaskData,
                    std::vector<AssociatedTaskData> &associatedTasks)
{
    std::unordered_map<TaskId, const TaskInfoData *, IDHasher> taskInfoIndex;
    for (const auto &taskInfo : taskInfoData)
    {
        taskInfoIndex[makeTaskId(taskInfo)] = &taskInfo;
    }

    if (!Reserve(associatedTasks, ascendTaskData.size()))
    {
        ERROR("Reserve for associated task data failed.");
        return false;
    }
    size_t invalidTimeRangeCount = 0;
    for (const auto &ascendTask : ascendTaskData)
    {
        auto taskInfo = taskInfoIndex.find(makeTaskId(ascendTask));
        if (taskInfo != taskInfoIndex.end())
        {
            // Validate once before sharing to prevent unsigned duration subtraction in every consumer.
            if (ascendTask.end < ascendTask.timestamp)
            {
                ++invalidTimeRangeCount;
                continue;
            }
            associatedTasks.push_back(
                AssociatedTaskData{taskInfo->second, &ascendTask, isOpSummaryRequired(*taskInfo->second)});
        }
    }
    if (invalidTimeRangeCount > 0)
    {
        WARN("Task association skipped % matched records with reversed time range.", invalidTimeRangeCount);
    }
    return true;
}
}  // namespace

TaskAssociationProcessor::TaskAssociationProcessor(const std::string &profPaths) : DataProcessor(profPaths) {}

bool TaskAssociationProcessor::Process(DataInventory &dataInventory)
{
    auto taskInfoData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (taskInfoData == nullptr || ascendTaskData == nullptr)
    {
        WARN("Task association source data not exist.");
        return true;
    }
    std::shared_ptr<AssociatedTaskCollection> sharedData;
    MAKE_SHARED_RETURN_VALUE(sharedData, AssociatedTaskCollection, false);
    sharedData->taskInfoData = taskInfoData;
    sharedData->ascendTaskData = ascendTaskData;
    if (!associateTasks(*sharedData->taskInfoData, *sharedData->ascendTaskData, sharedData->records))
    {
        return false;
    }
    INFO("Task association matched % records.", sharedData->records.size());
    if (!dataInventory.Inject(sharedData))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK_ASSOCIATION);
        return false;
    }
    return true;
}
}  // namespace Domain
}  // namespace Analysis
