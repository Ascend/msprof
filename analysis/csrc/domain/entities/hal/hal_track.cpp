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

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"

#include <algorithm>
#include <map>

namespace Analysis
{
namespace Domain
{
namespace
{
const uint16_t STEP_START_TAG = 60000;
const uint16_t STEP_END_TAG = 60001;
const uint16_t PAIR = 2;
}  // namespace

std::unordered_map<uint32_t, std::vector<HalTrackData*>> GetFlipData(std::vector<HalTrackData>& trackData)
{
    std::unordered_map<uint32_t, std::vector<HalTrackData*>> trackDataMap;
    for (auto& data : trackData)
    {
        if (data.type == TS_TASK_FLIP)
        {
            trackDataMap[data.hd.taskId.streamId].push_back(&data);
        }
    }
    return trackDataMap;
}

std::vector<HalTrackData> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type)
{
    std::vector<HalTrackData> trackDataRefer;
    for (auto& data : trackData)
    {
        if (data.type == type)
        {
            trackDataRefer.push_back(data);
        }
    }
    return trackDataRefer;
}

StepTraceDataVectorFormat GenerateStepTime(std::vector<HalTrackData>& halTraceTasks)
{
    StepTraceDataVectorFormat processedData;
    std::map<uint32_t, std::vector<HalTrackData>> stepTime;
    std::vector<HalTrackData> halTraceTasksBackup = halTraceTasks;
    std::sort(halTraceTasksBackup.begin(), halTraceTasksBackup.end(), [](const HalTrackData& t1, const HalTrackData& t2)
              { return t1.stepTrace.timestamp < t2.stepTrace.timestamp; });
    for (auto trackData : halTraceTasksBackup)
    {
        if (trackData.type == STEP_TRACE &&
            (trackData.stepTrace.tagId == STEP_START_TAG || trackData.stepTrace.tagId == STEP_END_TAG))
        {
            stepTime[trackData.stepTrace.indexId].emplace_back(trackData);
        }
    }
    for (auto& it : stepTime)
    {
        if (it.second.size() != PAIR)
        {
            continue;
        }
        processedData.emplace_back(it.first, it.second[0].stepTrace.modelId, it.second[0].stepTrace.timestamp,
                                   it.second[1].stepTrace.timestamp, it.first);
    }
    return processedData;
}
}  // namespace Domain
}  // namespace Analysis
