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
#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H

#include "analysis/csrc/domain/services/modeling/step_trace/state.h"

namespace Analysis
{
namespace Domain
{
// Fsm为状态机类，依据当前事件和状态机状态调用不同方法处理数据，其依赖State状态机状态类
class Fsm
{
   public:
    void Init();
    void OnEvent(EventLabel event, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);

   private:
    State* state_;      // 状态机当前状态
    uint32_t indexId_;  // 状态机当前indexId
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H
