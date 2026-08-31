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
#include <iostream>
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/domain/services/persistence/device/step_trace_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/trace_persistence.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"


namespace Analysis {
    using namespace Analysis;
    using namespace Analysis::Infra;
    using namespace Analysis::Domain;
    using namespace Analysis::Utils;
    using namespace Analysis::Application;
    namespace {
        const std::string DEVICE_PATH = "./device_1";
        const std::string TRACE_DB_PATH = File::PathJoin({DEVICE_PATH, "sqlite", "trace.db"});
    }
    class TracePersistenceUtest : public testing::Test {
    protected:
        void SetUp() override
        {
            EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
            EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        }
        void TearDown() override
        {
            dataInventory_.RemoveRestData({});
            EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
        }
        HalTrackData CreateHalTrackData(uint16_t moduleId, uint16_t tagId, uint64_t timestamp, uint16_t streamId = 2)
        {
            HalTrackData log;
            log.type = STEP_TRACE;
            log.stepTrace.modelId = moduleId;
            log.stepTrace.tagId = tagId;
            log.stepTrace.timestamp = timestamp;
            log.stepTrace.indexId = moduleId;
            log.hd.taskId.streamId = streamId;
            return log;
        }

    protected:
        DataInventory dataInventory_;
    };


//  modelId     tagId   timestamp   streamId
//  1           0       10          2
//  1           2       18          2
//  1           3       19          2
//  1           4       21          2
//  1           1       25          2
//  2           0       30          2
//  2           20000   33          2
//  2           20001   36          2
//  2           2       40          2
//  2           20002   42          2
//  2           20003   44          2
//  2           10000   46          6
//  2           20005   47          2
//  2           10001   48          6
//  2           3       55          2
//  2           10002   56          8
//  2           10003   58          8
//  2           10004   59          6   异常数据，无结束时间
//  2           4       60          2
//  2           1       65          2
//  2           0       66          2
//  2           60000   67          2
//  2           60001   68          2
//  2           4       69          2
//  2           0       70          2
//  2           0       77          2   异常数据，重复的model start
//  2           1       80          2
TEST_F(TracePersistenceUtest, ShouldSaveTraceDBSuccess)
{
    DataInventory dataInventory_;
    auto data = std::make_shared<std::vector<HalTrackData>>();
    data->emplace_back(CreateHalTrackData(2, 20000, 33)); // 2, 20000, 33
    data->emplace_back(CreateHalTrackData(2, 20001, 36)); // 2, 20001, 36
    data->emplace_back(CreateHalTrackData(1, 0, 10)); // 1, 0, 10
    data->emplace_back(CreateHalTrackData(1, 2, 18)); // 1, 2, 18
    data->emplace_back(CreateHalTrackData(2, 20003, 44)); // 2, 20003, 44
    data->emplace_back(CreateHalTrackData(2, 10000, 46, 6)); // 2, 10000, 46, 6
    data->emplace_back(CreateHalTrackData(2, 10001, 48, 6)); // 2, 10001, 48, 6
    data->emplace_back(CreateHalTrackData(2, 10004, 59, 6)); // 2, 10004, 59, 6
    data->emplace_back(CreateHalTrackData(1, 3, 19)); // 1, 3, 19
    data->emplace_back(CreateHalTrackData(1, 4, 21)); // 1, 4, 21
    data->emplace_back(CreateHalTrackData(1, 1, 25)); // 1, 1, 25
    data->emplace_back(CreateHalTrackData(2, 1, 65)); // 2, 1, 65
    data->emplace_back(CreateHalTrackData(2, 0, 30)); // 2, 0, 30
    data->emplace_back(CreateHalTrackData(2, 2, 40)); // 2, 2, 40
    data->emplace_back(CreateHalTrackData(2, 20002, 42)); // 2, 20002, 42
    data->emplace_back(CreateHalTrackData(2, 20005, 47)); // 2, 20005, 47
    data->emplace_back(CreateHalTrackData(2, 3, 55)); // 2, 3, 55
    data->emplace_back(CreateHalTrackData(2, 10002, 56, 8)); // 2, 10002, 56, 8
    data->emplace_back(CreateHalTrackData(2, 10003, 58, 8)); // 2, 10003, 58, 8
    data->emplace_back(CreateHalTrackData(2, 4, 60)); // 2, 4, 60
    data->emplace_back(CreateHalTrackData(2, 0, 66)); // 2, 0, 66
    data->emplace_back(CreateHalTrackData(2, 4, 69)); // 2, 4, 69
    data->emplace_back(CreateHalTrackData(2, 0, 70)); // 2, 0, 70
    data->emplace_back(CreateHalTrackData(2, 0, 77)); // 2, 0, 77
    data->emplace_back(CreateHalTrackData(2, 1, 80)); // 2, 1, 80
    data->emplace_back(CreateHalTrackData(2, 20005, 47)); // 2, 20005, 47
    data->emplace_back(CreateHalTrackData(2, 60000, 67)); // 2, 60000, 67
    data->emplace_back(CreateHalTrackData(2, 60001, 68)); // 2, 60001, 68
    dataInventory_.Inject(data);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;

    auto process = StepTraceProcess();
    ASSERT_EQ(ANALYSIS_OK, process.Run(dataInventory_, context));

    auto traceProcess = TracePersistence();
    ASSERT_EQ(ANALYSIS_OK, traceProcess.Run(dataInventory_, context));

    auto traceDb = TraceDB();
}

TEST_F(TracePersistenceUtest, ShouldSaveTraceDBSuccessWhenDataIsAged)
{
    DataInventory dataInventory_;
    auto data = std::make_shared<std::vector<HalTrackData>>();
    data->emplace_back(CreateHalTrackData(1, 0, 10)); // 1, 0, 10
    data->emplace_back(CreateHalTrackData(1, 3, 19)); // 1, 3, 19
    data->emplace_back(CreateHalTrackData(2, 0, 30)); // 2, 0, 30
    data->emplace_back(CreateHalTrackData(2, 20001, 36)); // 2, 20001, 36
    data->emplace_back(CreateHalTrackData(2, 20002, 37, 12)); // 2, 20002, 37, 12
    data->emplace_back(CreateHalTrackData(2, 20003, 44)); // 2, 20003, 44
    data->emplace_back(CreateHalTrackData(2, 10001, 48, 6)); // 2, 10001, 48, 6
    data->emplace_back(CreateHalTrackData(2, 10002, 49, 9)); // 2, 10002, 49, 9
    data->emplace_back(CreateHalTrackData(2, 10003, 58, 10)); // 2, 10003, 58, 10
    data->emplace_back(CreateHalTrackData(2, 60001, 68)); // 2, 60001, 68
    dataInventory_.Inject(data);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;

    auto process = StepTraceProcess();
    ASSERT_EQ(ANALYSIS_OK, process.Run(dataInventory_, context));

    auto traceProcess = TracePersistence();
    ASSERT_EQ(ANALYSIS_OK, traceProcess.Run(dataInventory_, context));
}

TEST_F(TracePersistenceUtest, ShouldPersistPythonCompatibleTraceSemantics)
{
    auto stepTasks = std::make_shared<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    StepTraceTasks task;
    task.indexId = 7;
    task.stepTrace.start = 1;
    task.stepTrace.end = 20;
    TimePair completedReduce;
    completedReduce.start = 11;
    completedReduce.end = 12;
    task.allReduceTable[3].push_back(completedReduce);
    TimePair incompleteReduce;
    incompleteReduce.start = 13;
    task.allReduceTable[3].push_back(incompleteReduce);
    task.getNextTable[1].starts = {3, 7};
    task.getNextTable[1].ends = {6, 10};
    (*stepTasks)[5].push_back(task);
    ASSERT_TRUE(dataInventory_.Inject(stepTasks));

    auto halData = std::make_shared<std::vector<HalTrackData>>();
    HalTrackData nonStepData{};
    nonStepData.type = TS_TASK_FLIP;
    halData->push_back(nonStepData);
    ASSERT_TRUE(dataInventory_.Inject(halData));

    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.deviceId = 2;
    TracePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, TRACE_DB_PATH);
    ASSERT_NE(nullptr, dbRunner);

    std::vector<std::tuple<int64_t>> counts;
    ASSERT_TRUE(dbRunner->QueryData("SELECT count(*) FROM all_reduce", counts));
    ASSERT_EQ(2, std::get<0>(counts.front()));
    counts.clear();
    ASSERT_TRUE(dbRunner->QueryData("SELECT count(*) FROM all_reduce WHERE end IS NULL", counts));
    EXPECT_EQ(1, std::get<0>(counts.front()));
    counts.clear();
    ASSERT_TRUE(dbRunner->QueryData("SELECT count(*) FROM pragma_table_info('all_reduce') WHERE pk > 0", counts));
    EXPECT_EQ(3, std::get<0>(counts.front()));

    using GetNextFormat = std::tuple<uint64_t, uint32_t, uint64_t, uint64_t>;
    std::vector<GetNextFormat> getNext;
    ASSERT_TRUE(dbRunner->QueryData(
        "SELECT model_id, index_id, start_time, end_time FROM get_next ORDER BY start_time", getNext));
    ASSERT_EQ(2ul, getNext.size());
    EXPECT_EQ(3ul, std::get<2>(getNext[0]));
    EXPECT_EQ(6ul, std::get<3>(getNext[0]));
    EXPECT_EQ(7ul, std::get<2>(getNext[1]));
    EXPECT_EQ(10ul, std::get<3>(getNext[1]));

    using TrainingFormat =
        std::tuple<uint32_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t>;
    std::vector<TrainingFormat> training;
    ASSERT_TRUE(dbRunner->QueryData("SELECT * FROM training_trace", training));
    ASSERT_EQ(1ul, training.size());
    EXPECT_EQ(0ul, std::get<3>(training.front()));
    EXPECT_EQ(0ul, std::get<4>(training.front()));
    EXPECT_EQ(20ul, std::get<5>(training.front()));
    EXPECT_EQ(19ul, std::get<6>(training.front()));
}

TEST_F(TracePersistenceUtest, ShouldCreateEmptyPythonCompatibleTraceTables)
{
    auto stepTasks = std::make_shared<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    StepTraceTasks task;
    task.indexId = 7;
    task.stepTrace.start = 1;
    task.stepTrace.end = 20;
    (*stepTasks)[5].push_back(task);
    ASSERT_TRUE(dataInventory_.Inject(stepTasks));

    auto halData = std::make_shared<std::vector<HalTrackData>>();
    HalTrackData nonStepData{};
    nonStepData.type = TS_TASK_FLIP;
    halData->push_back(nonStepData);
    ASSERT_TRUE(dataInventory_.Inject(halData));

    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    TracePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, TRACE_DB_PATH);
    ASSERT_NE(nullptr, dbRunner);
    EXPECT_TRUE(dbRunner->CheckTableExists("all_reduce"));
    EXPECT_TRUE(dbRunner->CheckTableExists("get_next"));
    EXPECT_TRUE(dbRunner->CheckTableExists("training_trace"));

    std::vector<std::tuple<int64_t>> counts;
    ASSERT_TRUE(dbRunner->QueryData("SELECT count(*) FROM all_reduce", counts));
    EXPECT_EQ(0, std::get<0>(counts.front()));
    counts.clear();
    ASSERT_TRUE(dbRunner->QueryData("SELECT count(*) FROM get_next", counts));
    EXPECT_EQ(0, std::get<0>(counts.front()));
}

}
