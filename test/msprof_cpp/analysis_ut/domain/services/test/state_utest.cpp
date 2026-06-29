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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/modeling/step_trace/state.h"
namespace Analysis {
namespace Domain {
namespace {
}
class StateUtest : public testing::Test {
public:
    static HalTrackData getHalTrackData()
    {
        HalTrackData track_data;
        HalStepTrace stepTrace;
        stepTrace.tagId = 0;
        track_data.stepTrace = stepTrace;
        return track_data;
    }
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
    State state;
    PreEndState preEndState;
    StartState startState;
    EndState endState;
};
TEST_F(StateUtest, TestInnerProcessEventWhenTagIdInvalid)
{
    HalTrackData track_data = StateUtest::getHalTrackData();
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    state.InnerProcessEvent(index, track_data, tasks);
    EXPECT_EQ(track_data.stepTrace.tagId, 0);
}
TEST_F(StateUtest, TestStepEndEventSuccess)
{
    uint32_t index = 1;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = state.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &state);
}
TEST_F(StateUtest, TestPreEndStateModelStartWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = preEndState.ModelStartEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &preEndState);
}
TEST_F(StateUtest, TestPreEndStateStepEndEventSuccess)
{
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = preEndState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(tasks.size(), 2ul);
    EXPECT_NE(&result, &preEndState);
}
TEST_F(StateUtest, TestPreEndStateStepEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = preEndState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &preEndState);
}
TEST_F(StateUtest, TestEndStateStepEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = endState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &endState);
}
TEST_F(StateUtest, TestEndStateModelEndEventSuccess)
{
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = endState.ModeLEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_NE(&result, &endState);
}
TEST_F(StateUtest, TestEndStateModelEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = endState.ModeLEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &endState);
}
}
}