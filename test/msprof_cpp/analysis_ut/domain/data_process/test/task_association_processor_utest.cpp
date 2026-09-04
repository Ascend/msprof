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

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/task_association_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/associated_task_data.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Application;

namespace
{
TaskInfoData makeTaskInfo(uint16_t deviceId, uint32_t streamId, uint32_t taskId, const std::string &opType,
                          const std::string &taskType, uint32_t batchId = 0, uint32_t contextId = 0,
                          const std::string &opName = "")
{
    TaskInfoData data;
    data.deviceId = deviceId;
    data.streamId = streamId;
    data.taskId = taskId;
    data.batchId = batchId;
    data.contextId = contextId;
    data.opType = opType;
    data.taskType = taskType;
    data.opName = opName;
    return data;
}

AscendTaskData makeAscendTask(uint16_t deviceId, uint32_t streamId, uint32_t taskId, double durationNs,
                              uint32_t batchId = 0, uint32_t contextId = 0)
{
    AscendTaskData data;
    data.deviceId = deviceId;
    data.streamId = streamId;
    data.taskId = taskId;
    data.batchId = batchId;
    data.contextId = contextId;
    data.timestamp = 0;
    data.end = static_cast<uint64_t>(durationNs);
    data.duration = durationNs;
    return data;
}

void injectSourceData(DataInventory &dataInventory, std::vector<TaskInfoData> taskInfo,
                      std::vector<AscendTaskData> ascendTask)
{
    std::shared_ptr<std::vector<TaskInfoData>> taskInfoPtr;
    MAKE_SHARED_NO_OPERATION(taskInfoPtr, std::vector<TaskInfoData>, std::move(taskInfo));
    dataInventory.Inject(taskInfoPtr);
    std::shared_ptr<std::vector<AscendTaskData>> ascendTaskPtr;
    MAKE_SHARED_NO_OPERATION(ascendTaskPtr, std::vector<AscendTaskData>, std::move(ascendTask));
    dataInventory.Inject(ascendTaskPtr);
}
}  // namespace

class TaskAssociationProcessorUTest : public testing::Test
{
};

TEST_F(TaskAssociationProcessorUTest, TestRunShouldReturnTrueWhenSourceDataNotExist)
{
    auto processor = TaskAssociationProcessor("./task_association");
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<AssociatedTaskCollection>());
}

TEST_F(TaskAssociationProcessorUTest, TestRunShouldReturnTrueWhenOnlyTaskInfoExist)
{
    auto dataInventory = DataInventory();
    std::vector<TaskInfoData> taskInfo{makeTaskInfo(0, 1, 1, "MatMul", "AI_CORE")};
    std::shared_ptr<std::vector<TaskInfoData>> taskInfoPtr;
    MAKE_SHARED_NO_OPERATION(taskInfoPtr, std::vector<TaskInfoData>, std::move(taskInfo));
    dataInventory.Inject(taskInfoPtr);
    auto processor = TaskAssociationProcessor("./task_association");
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<AssociatedTaskCollection>());
}

TEST_F(TaskAssociationProcessorUTest, TestRunShouldKeepMatchedTaskExcludedFromOpSummary)
{
    auto dataInventory = DataInventory();
    injectSourceData(dataInventory, {makeTaskInfo(0, 1, 1, "MatMul", "COMMUNICATION")},
                     {makeAscendTask(0, 1, 1, 1000.0)});
    auto processor = TaskAssociationProcessor("./task_association");
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    auto res = dataInventory.GetPtr<AssociatedTaskCollection>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(1ul, res->records.size());
    EXPECT_FALSE(res->records.front().opSummaryRequired);
}

TEST_F(TaskAssociationProcessorUTest, TestRunShouldKeepAllMatchedTasksAndMarkOpSummaryRecords)
{
    auto dataInventory = DataInventory();
    injectSourceData(dataInventory,
                     {makeTaskInfo(0, 1, 1, "MatMul", "AI_CORE", 0, 0, "mm"),
                      makeTaskInfo(0, 1, 2, "AllReduce", "COMMUNICATION", 0, 0, "opAivKernel"),
                      makeTaskInfo(0, 1, 3, "AllReduce", "COMMUNICATION", 0, 0, "opNormal"),
                      makeTaskInfo(0, 1, 4, "Write", "WRITE_BACK"),
                      makeTaskInfo(0, 1, 5, "CpuTask", "HCCL_AI_CPU")},
                     {makeAscendTask(0, 1, 1, 2000.0), makeAscendTask(0, 1, 2, 3000.0),
                      makeAscendTask(0, 1, 3, 7000.0), makeAscendTask(0, 1, 4, 1000.0),
                      makeAscendTask(0, 1, 5, 5000.0), makeAscendTask(0, 9, 9, 4000.0)});
    auto processor = TaskAssociationProcessor("./task_association");
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    auto res = dataInventory.GetPtr<AssociatedTaskCollection>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(5ul, res->records.size());
    EXPECT_EQ("MatMul", res->records.at(0).taskInfo->opType);
    EXPECT_EQ("AI_CORE", res->records.at(0).taskInfo->taskType);
    EXPECT_EQ(2000ul, res->records.at(0).ascendTask->end);
    EXPECT_TRUE(res->records.at(0).opSummaryRequired);
    EXPECT_EQ("AllReduce", res->records.at(1).taskInfo->opType);
    EXPECT_EQ("COMMUNICATION", res->records.at(1).taskInfo->taskType);
    EXPECT_EQ("opAivKernel", res->records.at(1).taskInfo->opName);
    EXPECT_TRUE(res->records.at(1).opSummaryRequired);
    EXPECT_FALSE(res->records.at(2).opSummaryRequired);
    EXPECT_FALSE(res->records.at(3).opSummaryRequired);
    EXPECT_FALSE(res->records.at(4).opSummaryRequired);
}

TEST_F(TaskAssociationProcessorUTest, TestRunShouldSkipMatchedTaskWithReversedTimeRange)
{
    auto invalidTask = makeAscendTask(0, 1, 1, 1000.0);
    invalidTask.timestamp = 2000;
    invalidTask.end = 1000;
    auto dataInventory = DataInventory();
    injectSourceData(dataInventory,
                     {makeTaskInfo(0, 1, 1, "InvalidTime", "AI_CORE"),
                      makeTaskInfo(0, 1, 2, "MatMul", "AI_CORE")},
                     {invalidTask, makeAscendTask(0, 1, 2, 3000.0)});

    auto processor = TaskAssociationProcessor("./task_association");
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    auto res = dataInventory.GetPtr<AssociatedTaskCollection>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(1ul, res->records.size());
    EXPECT_EQ(2ul, res->records.front().ascendTask->taskId);
    EXPECT_EQ("MatMul", res->records.front().taskInfo->opType);
}

TEST_F(TaskAssociationProcessorUTest, TestRunShouldMatchOnlyCompleteTaskId)
{
    auto dataInventory = DataInventory();
    injectSourceData(dataInventory,
                     {makeTaskInfo(0, 1, 7, "DeviceZero", "AI_CORE", 0, 0),
                      makeTaskInfo(1, 1, 7, "DeviceOne", "AI_CORE", 0, 0),
                      makeTaskInfo(0, 1, 7, "BatchOne", "AI_CORE", 1, 0),
                      makeTaskInfo(0, 1, 7, "ContextOne", "AI_CORE", 0, 1)},
                     {makeAscendTask(0, 1, 7, 1000.0, 0, 0), makeAscendTask(1, 1, 7, 1000.0, 0, 0),
                      makeAscendTask(0, 1, 7, 1000.0, 1, 0), makeAscendTask(0, 1, 7, 1000.0, 0, 1)});

    auto processor = TaskAssociationProcessor("./task_association");
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    auto res = dataInventory.GetPtr<AssociatedTaskCollection>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(4ul, res->records.size());
    EXPECT_EQ("DeviceZero", res->records.at(0).taskInfo->opType);
    EXPECT_EQ("DeviceOne", res->records.at(1).taskInfo->opType);
    EXPECT_EQ("BatchOne", res->records.at(2).taskInfo->opType);
    EXPECT_EQ("ContextOne", res->records.at(3).taskInfo->opType);
}
