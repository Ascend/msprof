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

#include "analysis/csrc/domain/services/modeling/step_trace/state.h"

#include <functional>
#include <unordered_map>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
const int TWO = 2;
void HandleTrainingTrace(const HalTrackData& step, StepTraceTasks& baseStep)
{
    if (step.stepTrace.tagId == FP_TAG)
    {
        TimePair timePair = {};
        timePair.start = step.stepTrace.timestamp;
        baseStep.trainingTrace.emplace_back(timePair);
    }
    else if (step.stepTrace.tagId == BP_TAG)
    {
        if (baseStep.trainingTrace.empty())
        {
            WARN("TrainingTrace: There is no start tag before the end of step: %.", step.stepTrace.indexId);
            return;
        }
        baseStep.trainingTrace.back().end = step.stepTrace.timestamp;
    }
}

void HandleAllReduceTable(const HalTrackData& step, StepTraceTasks& baseStep)
{
    if (step.stepTrace.tagId % TWO == 0)
    {
        // 偶数作为开始时间
        TimePair timePair = {};
        timePair.start = step.stepTrace.timestamp;
        baseStep.allReduceTable[step.hd.taskId.streamId].emplace_back(timePair);
    }
    else
    {
        if (baseStep.allReduceTable.empty())
        {
            WARN("allReduceTable is empty");
            return;
        }
        if (baseStep.allReduceTable.find(step.hd.taskId.streamId) != baseStep.allReduceTable.end())
        {
            auto& allReduce = baseStep.allReduceTable[step.hd.taskId.streamId];
            allReduce.back().end = step.stepTrace.timestamp;
        }
        else
        {
            WARN("StreamId % is not exist in allReduceTable", step.hd.taskId.streamId);
            return;
        }
    }
}

void HandleGetNextTable(const HalTrackData& step, StepTraceTasks& baseStep)
{
    if (step.stepTrace.tagId % TWO == 0)
    {
        // 偶数作为开始时间
        TimePair timePair = {};
        timePair.start = step.stepTrace.timestamp;
        baseStep.getNextTable[step.hd.taskId.streamId].emplace_back(timePair);
    }
    else
    {
        if (baseStep.getNextTable.empty())
        {
            WARN("getNextTable is empty");
            return;
        }
        if (baseStep.getNextTable.find(step.hd.taskId.streamId) != baseStep.getNextTable.end())
        {
            auto& getNext = baseStep.getNextTable[step.hd.taskId.streamId];
            getNext.back().end = step.stepTrace.timestamp;
        }
        else
        {
            WARN("StreamId % is not exist in getNextTable", step.hd.taskId.streamId);
            return;
        }
    }
}

StepLabel TransTagIdToLabel(uint16_t tagId)
{
    if (tagId == FP_TAG || tagId == BP_TAG)
    {
        return StepLabel::TrainingTraceLabel;
    }
    else if (tagId >= ALL_REDUCE_START && tagId < GET_NEXT_START_TAG)
    {
        return StepLabel::AllReduceLabel;
    }
    else if (tagId >= GET_NEXT_START_TAG)
    {
        return StepLabel::GetNextLabel;
    }
    else
    {
        return StepLabel::InvalidLabel;
    }
}

std::unordered_map<StepLabel, std::function<void(const HalTrackData&, StepTraceTasks&)>> g_actionMap{
    {StepLabel::TrainingTraceLabel, HandleTrainingTrace},
    {StepLabel::AllReduceLabel, HandleAllReduceTable},
    {StepLabel::GetNextLabel, HandleGetNextTable}};

State& State::ModelStartEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    return *this;
}

State& State::InnerProcessEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (baseSteps.empty())
    {
        WARN("State queue is empty");
        return *this;
    }
    auto& baseStep = baseSteps.back();
    auto handler = g_actionMap.find(TransTagIdToLabel(step.stepTrace.tagId));
    if (handler != g_actionMap.end())
    {
        handler->second(step, baseStep);
    }
    else
    {
        ERROR("No matching event processing function.");
    }
    return *this;
}

State& State::StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    return *this;
}

State& State::ModeLEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    return *this;
}

State& StartState::ModelStartEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    StepTraceTasks baseStep;
    baseStep.indexId = indexId;
    baseStep.stepTrace.start = step.stepTrace.timestamp;
    baseSteps.push_back(baseStep);
    return EndState::GetInstance();
}

State& PreEndState::ModelStartEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (baseSteps.empty())
    {
        ERROR("State queue is empty");
        return *this;
    }
    // 更新index的开始时间
    baseSteps.back().stepTrace.start = step.stepTrace.timestamp;
    // 设置end状态
    return EndState::GetInstance();
}

State& PreEndState::StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (baseSteps.empty())
    {
        ERROR("State queue is empty");
        return *this;
    }
    // 更新index的结束时间
    baseSteps.back().stepTrace.end = step.stepTrace.timestamp;
    indexId++;
    // 设置开始时间
    StepTraceTasks baseStep;
    baseStep.indexId = indexId;
    baseStep.stepTrace.start = step.stepTrace.timestamp;
    baseSteps.emplace_back(baseStep);
    // 设置preEnd状态
    return PreEndState::GetInstance();
}

State& EndState::StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (baseSteps.empty())
    {
        ERROR("State queue is empty");
        return *this;
    }
    // 更新index的结束时间
    baseSteps.back().stepTrace.end = step.stepTrace.timestamp;
    indexId++;
    // 设置开始状态
    StepTraceTasks baseStep;
    baseStep.indexId = indexId;
    baseStep.stepTrace.start = step.stepTrace.timestamp;
    baseSteps.emplace_back(baseStep);
    // 设置preEnd状态
    return PreEndState::GetInstance();
}

State& EndState::ModeLEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (baseSteps.empty())
    {
        ERROR("State queue is empty");
        return *this;
    }
    // 更新index的结束时间
    baseSteps.back().stepTrace.end = step.stepTrace.timestamp;
    indexId++;
    // 设置start状态
    return StartState::GetInstance();
}

}  // namespace Domain
}  // namespace Analysis
