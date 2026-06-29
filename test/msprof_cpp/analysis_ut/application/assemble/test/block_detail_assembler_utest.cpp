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
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/block_detail_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task//include/block_detail_data.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;

namespace {
    const int DEPTH = 0;
    const std::string BASE_PATH = "./block_detail_assembler_utest";
    const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
    const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
    const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class BlockDetailAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
protected:
    static DataInventory dataInventory_;
};
DataInventory BlockDetailAssemblerUTest::dataInventory_;

static std::vector<AscendTaskData> GenerateAscendTaskData()
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
    data.duration = 450.78; // dur 450.78
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
    data.taskType = "KERNEL_AICORE";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
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
    data.taskType = "KERNEL_AICORE";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
    return res;
}

static std::vector<BlockDetailData> GenerateBlockDetailData()
{
    std::vector<BlockDetailData> res;
    BlockDetailData data;
    data.coreId = 31;
    data.streamId = 1;
    data.taskId = 10;
    data.subtaskId = 4294967295;
    data.batchId = 0;
    data.coreType = 0;
    data.duration = 450.60;
    data.timestamp = 1717575960208020759; // 本地时间 1724405892226799429
    res.push_back(data);

    data.coreId = 32;
    data.streamId = 1;
    data.taskId = 10;
    data.subtaskId = 4294967295;
    data.batchId = 0;
    data.coreType = 0;
    data.duration = 250.26;
    data.timestamp = 1717575960208020759; // 本地时间 1724405892226799498
    res.push_back(data);
    return res;
}

TEST_F(BlockDetailAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    BlockDetailAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(BlockDetailAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    BlockDetailAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<BlockDetailData>> blockDataS;
    auto task = GenerateAscendTaskData();
    auto info = GenerateTaskInfoData();
    auto blockData = GenerateBlockDetailData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(blockDataS, std::vector<BlockDetailData>, blockData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(blockDataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"AIC Earliest\"}},{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":2,\"ph\":\"M\","
                            "\"args\":{\"name\":\"AIC Latest\"}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,"
                            "\"tid\":1,\"ts\":\"1717575960208020.759\",\"dur\":450.0,\"ph\":\"X\",\"args\":{"
                            "\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id"
                            "\":31}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":2,\"ts\":"
                            "\"1717575960208020.759\",\"dur\":250.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,"
                            "\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id\":32}},{\"name\":"
                            "\"thread_name\",\"pid\":2383961152,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"AIC Earliest\"}},{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":2,\"ph\":\"M\","
                            "\"args\":{\"name\":\"AIC Latest\"}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,"
                            "\"tid\":1,\"ts\":\"1717575960208020.759\",\"dur\":450.0,\"ph\":\"X\",\"args\":{"
                            "\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id"
                            "\":31}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":2,\"ts\":"
                            "\"1717575960208020.759\",\"dur\":250.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,"
                            "\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id\":32}},{\"name\":"
                            "\"process_name\",\"pid\":2383961152,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Block Detail\"}},{\"name\":\"process_labels\",\"pid\":2383961152,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383961152,\"tid"
                            "\":0,\"ph\":\"M\",\"args\":{\"sort_index\":34}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(BlockDetailAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    BlockDetailAssembler assembler;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<BlockDetailData>> blockDataS;
    auto info = GenerateTaskInfoData();
    auto blockData = GenerateBlockDetailData();
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(blockDataS, std::vector<BlockDetailData>, blockData);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(blockDataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(BlockDetailAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithoutTaskInfo)
{
    BlockDetailAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<BlockDetailData>> blockDataS;
    auto task = GenerateAscendTaskData();
    auto blockData = GenerateBlockDetailData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(blockDataS, std::vector<BlockDetailData>, blockData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(blockDataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"AIC Earliest\"}},{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":2,\"ph\":\"M\","
                            "\"args\":{\"name\":\"AIC Latest\"}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,"
                            "\"tid\":1,\"ts\":\"1717575960208020.759\",\"dur\":450.0,\"ph\":\"X\",\"args\":{"
                            "\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id"
                            "\":31}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":2,\"ts\":"
                            "\"1717575960208020.759\",\"dur\":250.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,"
                            "\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id\":32}},{\"name\":"
                            "\"thread_name\",\"pid\":2383961152,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"AIC Earliest\"}},{\"name\":\"thread_name\",\"pid\":2383961152,\"tid\":2,\"ph\":\"M\","
                            "\"args\":{\"name\":\"AIC Latest\"}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,"
                            "\"tid\":1,\"ts\":\"1717575960208020.759\",\"dur\":450.0,\"ph\":\"X\",\"args\":{"
                            "\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id"
                            "\":31}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":2,\"ts\":"
                            "\"1717575960208020.759\",\"dur\":250.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,"
                            "\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id\":32}},{\"name\":"
                            "\"process_name\",\"pid\":2383961152,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Block Detail\"}},{\"name\":\"process_labels\",\"pid\":2383961152,\"tid\":0,\"ph\":\"M"
                            "\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383961152,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":34}},{\"name\":\"thread_name\",\"pid"
                            "\":2383961152,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"AIC Earliest\"}},{\"name\":"
                            "\"thread_name\",\"pid\":2383961152,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"AIC Latest"
                            "\"}},{\"name\":\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":1,\"ts\":"
                            "\"1717575960208020.759\",\"dur\":450.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,"
                            "\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,\"Core Id\":31}},{\"name\":"
                            "\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":2,\"ts\":\"1717575960208020.759\",\"dur"
                            "\":250.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,"
                            "\"Subtask Id\":4294967295,\"Core Id\":32}},{\"name\":\"thread_name\",\"pid\":2383961152,"
                            "\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"AIC Earliest\"}},{\"name\":\"thread_name\","
                            "\"pid\":2383961152,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"AIC Latest\"}},{\"name\":"
                            "\"Stream 1 MatMulV3\",\"pid\":2383961152,\"tid\":1,\"ts\":\"1717575960208020.759\",\"dur"
                            "\":450.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,"
                            "\"Subtask Id\":4294967295,\"Core Id\":31}},{\"name\":\"Stream 1 MatMulV3\",\"pid"
                            "\":2383961152,\"tid\":2,\"ts\":\"1717575960208020.759\",\"dur\":250.0,\"ph\":\"X\","
                            "\"args\":{\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":0,\"Subtask Id\":4294967295,"
                            "\"Core Id\":32}},{\"name\":\"process_name\",\"pid\":2383961152,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"name\":\"Block Detail\"}},{\"name\":\"process_labels\",\"pid\":2383961152,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\","
                            "\"pid\":2383961152,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":34}},";
    EXPECT_EQ(expectStr, res.back());
}
