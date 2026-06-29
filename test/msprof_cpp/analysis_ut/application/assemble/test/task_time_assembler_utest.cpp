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
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/task_time_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;

namespace {
    const int DEPTH = 0;
    const std::string BASE_PATH = "./task_time_summary_test";
    const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_114514"});
    const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class TaskTimeAssemblerUTest : public testing::Test {
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
};

static std::vector<AscendTaskData> GenerateTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;  // device id 0
    data.indexId = -1; // index_id -1
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.connectionId = 2345; // connectionId 2345
    data.timestamp = 1717575960208020758; // start 1717575960208020758
    data.end = 1717575960208171758; // end 1717575960208171758
    data.duration = 151000.0; // dur 151000.0
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
    res.push_back(data); // 有taskInfo
    data.timestamp = 1717575960208040758; // start 1717575960208040758
    data.end = 1717575960208050758; // start 1717575960208050758
    res.push_back(data); // 无taskInfo
    data.deviceId = 0;  // device id 0
    data.indexId = -1; // index_id -1
    data.streamId = 1; // streamId 1
    data.taskId = 12; // taskId 12
    data.contextId = UINT32_MAX; // contextId UINT32_MAX
    data.batchId = 1; // batchId 1
    data.connectionId = 2500; // connectionId 2500
    data.timestamp = 1717575960208070758; // start 1717575960208070758
    data.end = 1717575960208085758; // end 1717575960208085758
    data.duration = 15000.0; // dur 15000.0
    res.push_back(data); // 有taskInfo
    data.deviceId = 0;  // device id 0
    data.indexId = -1; // index_id -1
    data.streamId = 1; // streamId 1
    data.taskId = 13; // taskId 13
    data.contextId = UINT32_MAX; // contextId UINT32_MAX
    data.batchId = 1; // batchId 1
    data.connectionId = 2550; // connectionId 2550
    data.timestamp = 1717575960208090758; // start 1717575960208090758
    data.end = 1717575960208090758; // end 1717575960208090758
    data.duration = 0.0; // dur 0.0
    res.push_back(data); // 有taskInfo
    return res;
}

static std::vector<TaskInfoData> GenerateTaskInfoData()
{
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    data.deviceId = 0; // deviceId 0
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.opName = "MatMulV3";
    res.push_back(data);
    data.deviceId = 0; // deviceId 0
    data.streamId = 1; // streamId 1
    data.taskId = 12; // taskId 12
    data.contextId = UINT32_MAX; // contextId UINT32_MAX
    data.batchId = 1; // batchId 1
    data.opName = "MatMulV3_1";
    res.push_back(data);
    data.deviceId = 0; // deviceId 0
    data.streamId = 1; // streamId 1
    data.taskId = 13; // taskId 13
    data.contextId = UINT32_MAX; // contextId UINT32_MAX
    data.batchId = 1; // batchId 1
    data.opName = "MatMulV3_2";
    res.push_back(data);
    return res;
}

TEST_F(TaskTimeAssemblerUTest, ShouldReturnTrueWhenTaskTimeDataNotExist)
{
    TaskTimeAssembler assembler(PROCESSOR_TASK_TIME_SUMMARY, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"task_time"}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(TaskTimeAssemblerUTest, ShouldReturnTrueWhenTaskTimeDataAssembleSuccess)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    TaskTimeAssembler assembler(PROCESSOR_TASK_TIME_SUMMARY, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"task_time"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(5ul, res.size());
    std::string expectHeader{"Device_id,kernel_name,kernel_type,stream_id,task_id,task_time(us),"
                             "task_start(us),task_stop(us)"};
    EXPECT_EQ(expectHeader, res[0]);
}

TEST_F(TaskTimeAssemblerUTest, ShouldReturnTrueWhenExistTaskInfoAndAscendTask)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    TaskTimeAssembler assembler(PROCESSOR_TASK_TIME_SUMMARY, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"task_time"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(5ul, res.size());

    std::string expectHeader{"Device_id,kernel_name,kernel_type,stream_id,task_id,task_time(us),"
                             "task_start(us),task_stop(us)"};
    EXPECT_EQ(expectHeader, res[0]);

    std::string expectOneRow{"0,MatMulV3,,1,10,151.000,1717575960208020.758\t,1717575960208171.758\t"};
    EXPECT_EQ(expectOneRow, res[1]);
}