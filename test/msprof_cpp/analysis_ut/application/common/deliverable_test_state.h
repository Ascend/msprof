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

#ifndef ANALYSIS_UT_DELIVERABLE_TEST_STATE_H
#define ANALYSIS_UT_DELIVERABLE_TEST_STATE_H

#include "analysis/csrc/application/deliverable_consumer.h"
#include "gtest/gtest.h"

namespace Analysis
{
namespace Test
{
// The deliverable consumer is process wide: an open JSON stream, a DB writer thread or a sticky
// failure flag would leak from one case into the next. Every test binary that builds the export
// path registers this listener instead of clearing the state by hand.
class ResetDeliverableState final : public testing::EmptyTestEventListener
{
   public:
    void OnTestStart(const testing::TestInfo &) override
    {
        auto &consumer = Application::DeliverableConsumer::Instance();
        consumer.FinishTimeline();
        consumer.CloseAllDb();
        consumer.ResetOk();
    }
};

inline void RegisterDeliverableStateReset()
{
    testing::UnitTest::GetInstance()->listeners().Append(new ResetDeliverableState());
}
}  // namespace Test
}  // namespace Analysis

#endif  // ANALYSIS_UT_DELIVERABLE_TEST_STATE_H
