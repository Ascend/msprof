/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/services/modeling/step_trace/model_step_trace.h"

namespace Analysis
{
namespace Domain
{

bool IsModelStart(int tag)
{
    if (tag == MODEL_START_TAG)
    {
        return true;
    }
    return false;
}

bool IsInnerProcess(int tag)
{
    if (tag == FP_TAG || tag == BP_TAG)
    {
        return true;
    }
    else if (tag >= ALL_REDUCE_START && tag < GET_NEXT_START_TAG)
    {
        return true;
    }
    else if (tag >= GET_NEXT_START_TAG)
    {
        return true;
    }
    return false;
}

bool IsStepEnd(int tag)
{
    if (tag == ITER_END_TAG)
    {
        return true;
    }
    return false;
}

bool IsModelEnd(int tag)
{
    if (tag == MODEL_END_TAG)
    {
        return true;
    }
    return false;
}

EventLabel TagToEvent(int tag)
{
    if (IsModelStart(tag))
    {
        return EventLabel::ModelStart;
    }
    else if (IsInnerProcess(tag))
    {
        return EventLabel::InnerProcess;
    }
    else if (IsStepEnd(tag))
    {
        return EventLabel::StepEnd;
    }
    else if (IsModelEnd(tag))
    {
        return EventLabel::ModelEnd;
    }
    return EventLabel::InvalidEvent;
}

void ModelStepTrace::Init() { fsm_.Init(); }

void ModelStepTrace::OnStep(const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    EventLabel event = TagToEvent(step.stepTrace.tagId);
    fsm_.OnEvent(event, step, baseSteps);
}

}  // namespace Domain
}  // namespace Analysis
