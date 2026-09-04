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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/overlap_analysis_processor.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;

TEST(OverlapAnalysisProcessorUTest, WideMc2StreamIdShouldNotFilterDifferentUint16TaskStream)
{
    AscendTaskData ascendTask;
    ascendTask.deviceId = 0;
    ascendTask.streamId = 4467;
    ascendTask.taskId = 1;
    ascendTask.contextId = 2;
    ascendTask.batchId = 3;
    ascendTask.timestamp = 10;
    ascendTask.duration = 10;

    TaskInfoData computeTask;
    computeTask.deviceId = ascendTask.deviceId;
    computeTask.streamId = ascendTask.streamId;
    computeTask.taskId = ascendTask.taskId;
    computeTask.contextId = ascendTask.contextId;
    computeTask.batchId = ascendTask.batchId;
    computeTask.opName = "compute_op";

    MC2CommInfoData mc2CommInfo;
    mc2CommInfo.aiCpuKfcStreamId = 70003;

    DataInventory dataInventory;
    ASSERT_TRUE(dataInventory.Inject(std::make_shared<std::vector<AscendTaskData>>(1, ascendTask)));
    ASSERT_TRUE(dataInventory.Inject(std::make_shared<std::vector<TaskInfoData>>(1, computeTask)));
    ASSERT_TRUE(dataInventory.Inject(std::make_shared<std::vector<MC2CommInfoData>>(1, mc2CommInfo)));

    OverlapAnalysisProcessor processor;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OVERLAP_ANALYSIS));
    auto result = dataInventory.GetPtr<std::vector<OverlapAnalysisData>>();
    ASSERT_TRUE(result);
    ASSERT_EQ(1UL, result->size());
    EXPECT_EQ(OverlapAnalysisType::COMPUTE, result->front().type);
    EXPECT_EQ(10UL, result->front().timestamp);
    EXPECT_EQ(10UL, result->front().duration);
}
