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

#include "analysis/csrc/domain/services/modeling/step_trace/fsm.h"

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Domain
{
void Fsm::Init()
{
    state_ = &StartState::GetInstance();
    indexId_ = 1;
}

void Fsm::OnEvent(EventLabel event, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (state_ == nullptr)
    {
        ERROR("Unknown state: %", state_);
        return;
    }
    switch (event)
    {
        case EventLabel::ModelStart:
            state_ = &state_->ModelStartEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::InnerProcess:
            state_ = &state_->InnerProcessEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::StepEnd:
            state_ = &state_->StepEndEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::ModelEnd:
            state_ = &state_->ModeLEndEvent(indexId_, step, baseSteps);
            break;
        default:
            return;
    }
}

}  // namespace Domain
}  // namespace Analysis
