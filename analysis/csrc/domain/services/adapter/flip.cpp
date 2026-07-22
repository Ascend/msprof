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

#include "flip.h"

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Adapter
{
using namespace Analysis::Utils;

namespace
{
const uint16_t STREAM_DESTROY_FLIP = UINT16_MAX;
const uint32_t STREAM_DESTROY_FLIP_V2 = UINT32_MAX;
const uint32_t TASK_ID_BIT = 0x0000FFFF;
const uint16_t BIT_NUM = 16;
const uint16_t BIT_V2_NUM = 32;
}  // namespace

uint16_t Flip::GetTaskId(const MsprofCompactInfo &task)
{
    return task.data.runtimeTrackV2.taskId & TASK_ID_BIT;  // taskId是低16位
}

uint16_t Flip::GetBatchId(const MsprofCompactInfo &task)
{
    return task.data.runtimeTrackV2.taskId >> BIT_NUM;  // batchId/flipNum是高16位
}

void Flip::SetBatchId(MsprofCompactInfo &task, uint32_t batchId)
{
    task.data.runtimeTrackV2.taskId = (task.data.runtimeTrackV2.taskId & TASK_ID_BIT) + (batchId << BIT_NUM);
}

std::shared_ptr<FlipTask> Flip::CreateFlipTask(MsprofCompactInfo *taskTrack)
{
    if (!taskTrack)
    {
        ERROR("Task track is null.");
        return nullptr;
    }
    std::shared_ptr<FlipTask> flipTask;
    MAKE_SHARED0_RETURN_VALUE(flipTask, FlipTask, nullptr);

    flipTask->deviceId = taskTrack->data.runtimeTrackV2.deviceId;
    flipTask->streamId = taskTrack->data.runtimeTrackV2.streamId;
    flipTask->taskId = GetTaskId(*taskTrack);
    flipTask->timeStamp = taskTrack->timeStamp;
    flipTask->flipNum = GetBatchId(*taskTrack);
    return flipTask;
}

void Flip::ComputeBatchId(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                          std::vector<std::shared_ptr<FlipTask>> &flipTask, bool isV2)
{
    if (taskTrack.empty())
    {
        WARN("Task tracks is empty.");
        return;
    }
    auto taskTrackBin = SepTaskTrack(taskTrack);
    auto flipTaskBin = SepFlipTask(flipTask);
    std::shared_ptr<FlipTask> maxFlip;
    MAKE_SHARED0_RETURN_VOID(maxFlip, FlipTask);

    maxFlip->timeStamp = std::numeric_limits<uint64_t>::max();
    for (auto &item : taskTrackBin)
    {
        const auto &key = item.first;
        auto &taskTrackInStream = item.second;
        std::vector<std::shared_ptr<FlipTask>> flipTaskInStream;
        auto iter = flipTaskBin.find(key);
        if (iter != flipTaskBin.end())
        {
            flipTaskInStream = iter->second;
        }
        flipTaskInStream.emplace_back(maxFlip);
        if (!isV2 && flipTaskInStream.size() > UINT16_MAX)
        {
            ERROR("The size of flip task in device_id % and stream_id % is greater than %.",
                  flipTaskInStream[0]->deviceId, flipTaskInStream[0]->streamId, UINT16_MAX);
            continue;
        }
        if (isV2 && flipTaskInStream.size() > UINT32_MAX)
        {
            ERROR("The size of flip task in device_id % and stream_id % is greater than %.",
                  flipTaskInStream[0]->deviceId, flipTaskInStream[0]->streamId, UINT32_MAX);
            continue;
        }
        uint32_t batchId = 0;
        uint32_t taskIdx = 0;
        while (taskIdx < taskTrackInStream.size() && batchId < flipTaskInStream.size())
        {
            auto &task = taskTrackInStream[taskIdx];
            const auto &flip = flipTaskInStream[batchId];
            if (task->timeStamp > flip->timeStamp)
            {
                ++batchId;  // next flip
                CalibrateFlipTaskIdNotZero(taskTrackInStream, flip, taskIdx, batchId, isV2);
                continue;
            }
            SetBatchId(*task, batchId);
            ++taskIdx;  // next task
        }
    }
}

Flip::FlipTaskMap Flip::SepFlipTask(const std::vector<std::shared_ptr<FlipTask>> &flipTask)
{
    FlipTaskMap data;
    for (const auto &flip : flipTask)
    {
        if (!flip)
        {
            ERROR("Flip task is null.");
            continue;
        }
        uint64_t key = (static_cast<uint64_t>(flip->deviceId) << BIT_V2_NUM) + static_cast<uint64_t>(flip->streamId);
        data[key].emplace_back(flip);
    }
    return data;
}

Flip::CompactInfoMap Flip::SepTaskTrack(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack)
{
    CompactInfoMap data;
    for (const auto &task : taskTrack)
    {
        if (!task)
        {
            ERROR("Task track is null.");
            continue;
        }
        // deviceId为高32bit, streamId为低32bit，拼接为key
        uint64_t key = (static_cast<uint64_t>(task->data.runtimeTrackV2.deviceId) << BIT_V2_NUM) +
                       static_cast<uint64_t>(task->data.runtimeTrackV2.streamId);
        data[key].emplace_back(task);
    }
    return data;
}

void Flip::CalibrateFlipTaskIdNotZero(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                                      const std::shared_ptr<FlipTask> &flip, uint32_t taskIdx, uint32_t batchId,
                                      bool isV2)
{
    if (!isV2 && flip->flipNum == STREAM_DESTROY_FLIP)
    {
        return;
    }
    if (isV2 && flip->flipNum == STREAM_DESTROY_FLIP_V2)
    {
        return;
    }
    // Because tasks in multi-threads will apply for task id 0 simultaneously,
    // the flip may not get the task_id 0, we should search backward to calibrate the task
    // which task id is less than flip's task_id, and set these tasks the next batch id.
    while (taskIdx >= 1 && GetTaskId(*taskTrack[taskIdx - 1]) < flip->taskId)
    {
        SetBatchId(*taskTrack[taskIdx - 1], batchId);
        --taskIdx;
    }
}
}  // namespace Adapter
}  // namespace Domain
}  // namespace Analysis
