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

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/op_statistic_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/task_association_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/associated_task_data.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string PROF_PATH = "./op_statistic_processor_test/PROF_0";

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

AscendTaskData makeAscendTask(uint16_t deviceId, uint32_t streamId, uint32_t taskId, uint64_t timestamp,
                              uint64_t end, uint32_t batchId = 0, uint32_t contextId = 0)
{
    AscendTaskData data;
    data.deviceId = deviceId;
    data.streamId = streamId;
    data.taskId = taskId;
    data.batchId = batchId;
    data.contextId = contextId;
    data.timestamp = timestamp;
    data.end = end;
    data.duration = 1.0;
    return data;
}

struct AssociatedTaskInput
{
    AssociatedTaskInput(TaskInfoData taskInfoData, AscendTaskData ascendTaskData, bool isOpSummaryRequired = true)
        : taskInfo(std::move(taskInfoData)), ascendTask(std::move(ascendTaskData)),
          opSummaryRequired(isOpSummaryRequired)
    {
    }

    TaskInfoData taskInfo;
    AscendTaskData ascendTask;
    bool opSummaryRequired;
};

AssociatedTaskInput makeAssociatedTask(uint16_t deviceId, uint32_t streamId, uint32_t taskId,
                                       const std::string &opType, const std::string &taskType, uint64_t durationNs)
{
    return {makeTaskInfo(deviceId, streamId, taskId, opType, taskType),
            makeAscendTask(deviceId, streamId, taskId, 100, 100 + durationNs), true};
}

void injectAssociatedTasks(DataInventory &dataInventory, std::vector<AssociatedTaskInput> associatedTasks)
{
    std::vector<TaskInfoData> taskInfoData;
    std::vector<AscendTaskData> ascendTaskData;
    taskInfoData.reserve(associatedTasks.size());
    ascendTaskData.reserve(associatedTasks.size());
    for (auto &associatedTask : associatedTasks)
    {
        taskInfoData.emplace_back(std::move(associatedTask.taskInfo));
        ascendTaskData.emplace_back(std::move(associatedTask.ascendTask));
    }

    std::shared_ptr<std::vector<TaskInfoData>> taskInfoDataPtr;
    std::shared_ptr<std::vector<AscendTaskData>> ascendTaskDataPtr;
    std::shared_ptr<AssociatedTaskCollection> associatedTaskCollection;
    MAKE_SHARED_NO_OPERATION(taskInfoDataPtr, std::vector<TaskInfoData>, std::move(taskInfoData));
    MAKE_SHARED_NO_OPERATION(ascendTaskDataPtr, std::vector<AscendTaskData>, std::move(ascendTaskData));
    MAKE_SHARED_NO_OPERATION(associatedTaskCollection, AssociatedTaskCollection);
    associatedTaskCollection->taskInfoData = taskInfoDataPtr;
    associatedTaskCollection->ascendTaskData = ascendTaskDataPtr;
    for (size_t index = 0; index < associatedTasks.size(); ++index)
    {
        associatedTaskCollection->records.push_back(AssociatedTaskData{
            &associatedTaskCollection->taskInfoData->at(index), &associatedTaskCollection->ascendTaskData->at(index),
            associatedTasks.at(index).opSummaryRequired});
    }
    dataInventory.Inject(associatedTaskCollection);
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

const OpStatisticData *findStatistic(const std::vector<OpStatisticData> &data, uint16_t deviceId,
                                     const std::string &opType, const std::string &coreType)
{
    for (const auto &item : data)
    {
        if (item.deviceId == deviceId && item.opType == opType && item.coreType == coreType)
        {
            return &item;
        }
    }
    return nullptr;
}
}  // namespace

TEST(OpStatisticProcessorUTest, TestRunShouldReturnTrueWhenSourceDataNotExist)
{
    DataInventory dataInventory;
    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<OpStatisticData>>());
}

TEST(OpStatisticProcessorUTest, TestRunShouldReturnTrueWhenAssociatedTaskDataEmpty)
{
    DataInventory dataInventory;
    injectAssociatedTasks(dataInventory, {});
    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<OpStatisticData>>());
}

TEST(OpStatisticProcessorUTest, TestRunShouldAggregateAssociatedTasks)
{
    DataInventory dataInventory;
    injectAssociatedTasks(dataInventory,
                          {makeAssociatedTask(0, 1, 1, "MatMul", "AI_CORE", 2000),
                           makeAssociatedTask(0, 1, 2, "MatMul", "AI_CORE", 4000),
                           makeAssociatedTask(0, 2, 1, "Add", "MIX_AIC", 4000)});

    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    auto result = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2ul, result->size());

    auto matmul = findStatistic(*result, 0, "MatMul", "AI_CORE");
    ASSERT_NE(nullptr, matmul);
    EXPECT_EQ("2", matmul->count);
    EXPECT_DOUBLE_EQ(6.0, matmul->totalTime);
    EXPECT_DOUBLE_EQ(2.0, matmul->min);
    EXPECT_DOUBLE_EQ(3.0, matmul->avg);
    EXPECT_DOUBLE_EQ(4.0, matmul->max);
    EXPECT_DOUBLE_EQ(60.0, matmul->ratio);

    auto add = findStatistic(*result, 0, "Add", "MIX_AIC");
    ASSERT_NE(nullptr, add);
    EXPECT_EQ("1", add->count);
    EXPECT_DOUBLE_EQ(4.0, add->totalTime);
    EXPECT_DOUBLE_EQ(40.0, add->ratio);
}

TEST(OpStatisticProcessorUTest, TestRunShouldComputeRatioPerDevice)
{
    DataInventory dataInventory;
    injectAssociatedTasks(dataInventory,
                          {makeAssociatedTask(0, 1, 1, "MatMul", "AI_CORE", 1000),
                           makeAssociatedTask(1, 1, 1, "Add", "AI_CORE", 2000)});

    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    auto result = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2ul, result->size());
    EXPECT_DOUBLE_EQ(100.0, findStatistic(*result, 0, "MatMul", "AI_CORE")->ratio);
    EXPECT_DOUBLE_EQ(100.0, findStatistic(*result, 1, "Add", "AI_CORE")->ratio);
}

TEST(OpStatisticProcessorUTest, TestRunShouldUseCompleteOpReportForRatioAndFilterCsvRows)
{
    DataInventory dataInventory;
    auto communication = makeAssociatedTask(0, 1, 2, "AllReduce", "COMMUNICATION", 3000);
    communication.taskInfo.opName = "opAivKernel";
    injectAssociatedTasks(
        dataInventory,
        {makeAssociatedTask(0, 1, 1, "N/A", "AI_CORE", 1000), communication,
         makeAssociatedTask(0, 1, 3, "MatMul", "AI_CORE", 2000),
         makeAssociatedTask(0, 1, 4, "Write", "WRITE_BACK", 3000),
         makeAssociatedTask(0, 1, 5, "Invalid", "INVALID", 4000),
         makeAssociatedTask(0, 1, 6, "CpuTask", "HCCL_AI_CPU", 5000)});

    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    auto result = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1ul, result->size());
    auto matmul = findStatistic(*result, 0, "MatMul", "AI_CORE");
    ASSERT_NE(nullptr, matmul);
    EXPECT_DOUBLE_EQ(20.0, matmul->ratio);
}

TEST(OpStatisticProcessorUTest, TestRunShouldConsumeAssociatedTasksFromPredecessor)
{
    DataInventory dataInventory;
    injectSourceData(dataInventory,
                     {makeTaskInfo(0, 1, 1, "MatMul", "AI_CORE"),
                      makeTaskInfo(0, 1, 2, "Write", "WRITE_BACK")},
                     {makeAscendTask(0, 1, 1, 0, 1000), makeAscendTask(0, 1, 2, 0, 3000)});

    EXPECT_TRUE(TaskAssociationProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_TASK_ASSOCIATION));
    dataInventory.RemoveRestData({typeid(AssociatedTaskCollection)});
    EXPECT_TRUE(OpStatisticProcessor(PROF_PATH).Run(dataInventory, PROCESSOR_NAME_OP_STATISTIC));
    auto result = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1ul, result->size());
    auto matmul = findStatistic(*result, 0, "MatMul", "AI_CORE");
    ASSERT_NE(nullptr, matmul);
    EXPECT_DOUBLE_EQ(25.0, matmul->ratio);
}
