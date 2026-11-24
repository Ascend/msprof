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

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;

namespace
{
const int DEPTH = 0;
const std::string BASE_PATH = "./ascend_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}  // namespace

class AscendHardwareAssemblerUTest : public testing::Test
{
   protected:
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH))
        {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }

   protected:
    DataInventory dataInventory_;
};

static std::vector<AscendTaskData> GenerateTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;                     // device id 0
    data.indexId = -1;                     // index_id -1
    data.streamId = 1;                     // streamId 1
    data.taskId = 10;                      // taskId 10
    data.contextId = 1;                    // contextId 1
    data.batchId = 1;                      // batchId 1
    data.connectionId = 2345;              // connectionId 2345
    data.timestamp = 1717575960208020758;  // start 1717575960208020758
    data.duration = 450.78;                // dur 450.78
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
    data.taskType = "KERNEL_AICORE";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
    data.connectionId = 1234;  // connectionId 1234
    data.hostType = "EVENT_RECORD";
    data.deviceType = "EVENT_RECORD_SQE";
    data.taskType = "EVENT_RECORD_SQE";
    data.taskId = 11;  // taskId 11
    res.push_back(data);
    data.duration = 50.0;
    data.timestamp = 1717575960208021200;
    data.connectionId = 1235;  // connectionId 1235
    data.hostType = "EVENT_WAIT";
    data.deviceType = "EVENT_WAIT_SQE";
    data.taskType = "EVENT_WAIT_SQE";
    data.taskId = 21;  // taskId 21
    res.push_back(data);
    data.taskId = 0;
    data.batchId = 0;
    data.timestamp = 1717575960208021758;  // start 1717575960208021758
    data.duration = 4241260;               // duration 4241260
    data.hostType = "MEMCPY_ASYNC";
    data.deviceType = "PCIE_DMA_SQE";
    data.taskType = "PCIE_DMA_SQE";
    data.connectionId = 181;  // connectionId 181
    res.push_back(data);
    data.taskId = 1;
    data.timestamp = 1717575960213021758;  // start 1717575960213021758
    data.duration = 1202500;               // duration 1202500
    res.push_back(data);
    data.taskId = 2;                       // taskId 2
    data.timestamp = 1717575960215021758;  // start 1717575960215021758
    data.duration = 3036900;               // duration 3036900
    data.connectionId = 189;               // connectionId 189
    res.push_back(data);
    return res;
}

static std::vector<TaskInfoData> GenerateTaskInfoData()
{
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    data.deviceId = 0;   // deviceId 0
    data.streamId = 1;   // streamId 1
    data.taskId = 10;    // taskId 10
    data.contextId = 1;  // contextId 1
    data.batchId = 1;    // batchId 1
    data.opName = "MatMulV3";
    data.taskType = "AI_VECTOR_CORE";
    res.push_back(data);
    data.contextId = UINT32_MAX;
    res.push_back(data);
    return res;
}

static std::vector<KfcTurnData> GenerateKfcTurnData()
{
    std::vector<KfcTurnData> res;
    KfcTurnData data;
    data.opName = "WaitExecute";
    data.deviceId = 0;                     // device id 0
    data.streamId = 1;                     // streamId 1
    data.taskId = 15;                      // taskId 15
    data.timestamp = 1717575960208020758;  // start 1717575960208020758
    res.push_back(data);
    return res;
}

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.structType = "ACL_RTS";
    data.id = "aclrtRecordEvent";
    data.level = 20000;      // level 20000
    data.threadId = 113991;  // threadId 113991
    data.apiName = "aclrtRecordEvent";
    data.itemId = "0";
    data.connectionId = 1234;  // connectionId 1234
    res.push_back(data);
    data.id = "aclrtStreamWaitEvent";
    data.apiName = "aclrtStreamWaitEvent";
    data.connectionId = 1235;  // connectionId 1235
    res.push_back(data);
    return res;
}

static std::vector<MemcpyInfoData> GenerateMemcpyInfoData()
{
    std::vector<MemcpyInfoData> res;
    MemcpyInfoData data;
    TaskId taskId1{1, 0, 0, UINT32_MAX, 0};  // streamId:1, taskId:0
    TaskId taskId2{1, 0, 1, UINT32_MAX, 0};  // streamId:1, taskId:1
    TaskId taskId3{1, 0, 2, UINT32_MAX, 0};  // streamId:1, taskId:2
    data.taskId = taskId1;
    data.dataSize = 67108864;  // dataSize 67108864
    data.memcpyOperation = 1;
    res.push_back(data);
    data.taskId = taskId2;
    data.dataSize = 16777216;  // dataSize 16777216
    res.push_back(data);
    data.taskId = taskId3;
    data.dataSize = 35651584;  // dataSize 35651584
    res.push_back(data);
    return res;
}

static std::vector<AscendTaskData> GenerateSimtTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;                     // deviceId 0
    data.indexId = -1;                     // index_id -1
    data.streamId = 1;                     // streamId 1
    data.taskId = 10;                      // taskId 10
    data.contextId = 1;                    // contextId 1
    data.batchId = 1;                      // batchId 1
    data.connectionId = 2345;              // connectionId 2345
    data.timestamp = 1717575960208020758;  // start 1717575960208020758
    data.duration = 450.78;                // dur 450.78
    data.hostType = "KERNEL_SIMT";
    data.deviceType = "AI_CORE";
    data.taskType = "KERNEL_SIMT";
    res.push_back(data);
    return res;
}

static std::vector<TaskInfoData> GenerateSimtTaskInfoData()
{
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    data.deviceId = 0;   // deviceId 0
    data.streamId = 1;   // streamId 1
    data.taskId = 10;    // taskId 10
    data.contextId = 1;  // contextId 1
    data.batchId = 1;    // batchId 1
    data.opName = "MatMulV3";
    data.taskType = "KERNEL_SIMT";
    data.gridDim = "2,3,4";
    data.blockDim = "5,6,7";
    res.push_back(data);
    return res;
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    AscendHardwareAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    std::shared_ptr<std::vector<ApiData>> apiS;
    std::shared_ptr<std::vector<MemcpyInfoData>> memcpyS;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    task.front().writeLatency = 7.5;
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    auto memcpyInfoData = GenerateMemcpyInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    MAKE_SHARED_NO_OPERATION(apiS, std::vector<ApiData>, api);
    MAKE_SHARED_NO_OPERATION(memcpyS, std::vector<MemcpyInfoData>, memcpyInfoData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    dataInventory_.Inject(apiS);
    dataInventory_.Inject(memcpyS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));  // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr =
        "{\"name\":\"MatMulV3\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\""
        ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
        "AI_VECTOR_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,"
        "\"connection_id\":2345,\"Write "
        "Latency(ns)\":7.5}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328512,\""
        "tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":\""
        "1717575960208020.758\",\"bp\":\"e\"},{\"name\":\"EVENT_RECORD\",\"pid\":10328512,\"tid\""
        ":1,\"ts\":\"1717575960208020.758\",\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\""
        "Model Id\":4294967295,\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\""
        "Task Id\":11,\"Batch Id\":1,\"Subtask Id\":4294967295,\"connection_id\":1234,\"Write "
        "Latency(ns)\":0.0}},{\"name\""
        ":\"HostToDevice5299989643264\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"5299989643264\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"},{\""
        "name\":\"EVENT_WAIT\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208021.200\",\"dur\""
        ":0.05,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"EVENT_WAIT_SQE\","
        "\"Physic Stream Id\":1,\"Task Id\":21,\"Batch Id\":1,\"Subtask Id\":4294967295,\""
        "connection_id\":1235,\"Write Latency(ns)\":0.0}},{\"name\":\"HostToDevice5304284610560\",\"pid\":10328512,\""
        "tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"5304284610560\",\"ts\":\""
        "1717575960208021.200\",\"bp\":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\""
        "tid\":1,\"ts\":\"1717575960208021.758\",\"dur\":4241.26,\"ph\":\"X\",\"args\":{\""
        "Model Id\":4294967295,\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\""
        ":0,\"Batch Id\":0,\"Subtask Id\":4294967295,\"connection_id\":181,\"Write "
        "Latency(ns)\":0.0,\"size(B)\":67108864,"
        "\"bandwidth(GB/s)\":14.736186887858798,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960208021.758\",\"bp\":\""
        "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1,\"ts\":\""
        "1717575960213021.758\",\"dur\":1202.5,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":1,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":181,\"Write "
        "Latency(ns)\":0.0,\"size(B)\":16777216,\"bandwidth(GB/s)\""
        ":12.993762993762992,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960213021.758\",\"bp\":\""
        "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1,\"ts\":\""
        "1717575960215021.758\",\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":189,\"Write "
        "Latency(ns)\":0.0,\"size(B)\":35651584,\"bandwidth(GB/s)\""
        ":10.9332296091409,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice811748818944\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"811748818944\",\"ts\":\"1717575960215021.758\",\"bp\":\"e\""
        "},{\"name\":\"WaitExecute\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208020.758\",\""
        "dur\":0.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":15}},{\"name\":\""
        "process_name\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
        "Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\","
        "\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":10328512,\"tid\""
        ":0,\"ph\":\"M\",\"args\":{\"sort_index\":15}},{\"name\":\"thread_name\",\"pid\":10328512,"
        "\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\","
        "\"pid\":10328512,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(std::string("[") + expectStr.substr(0, expectStr.size() - 1), res.back());
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087));  // pid 10087
    MOCKER_CPP(&JsonEventSink::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithoutApi)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));  // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr =
        "{\"name\":\"MatMulV3\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\""
        ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
        "AI_VECTOR_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,"
        "\"connection_id\":2345,\"Write "
        "Latency(ns)\":0.0}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328512,\""
        "tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":\""
        "1717575960208020.758\",\"bp\":\"e\"},{\"name\":\"EVENT_RECORD\",\"pid\":10328512,\"tid\""
        ":1,\"ts\":\"1717575960208020.758\",\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\""
        "Model Id\":4294967295,\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\""
        "Task Id\":11,\"Batch Id\":1,\"Subtask Id\":4294967295,\"connection_id\":1234,\"Write "
        "Latency(ns)\":0.0}},{\"name\""
        ":\"EVENT_WAIT\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208021.200\",\"dur\":0.05,"
        "\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"EVENT_WAIT_SQE\",\""
        "Physic Stream Id\":1,\"Task Id\":21,\"Batch Id\":1,\"Subtask Id\":4294967295,\""
        "connection_id\":1235,\"Write "
        "Latency(ns)\":0.0}},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1,\"ts\":\""
        "1717575960208021.758\",\"dur\":4241.26,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":0,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":181,\"Write Latency(ns)\":0.0}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960208021.758\",\"bp\":\""
        "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1,\"ts\":\""
        "1717575960213021.758\",\"dur\":1202.5,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":1,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":181,\"Write Latency(ns)\":0.0}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960213021.758\",\"bp\":\""
        "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1,\"ts\":\""
        "1717575960215021.758\",\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":189,\"Write Latency(ns)\":0.0}},{\"name\":\""
        "HostToDevice811748818944\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"811748818944\",\"ts\":\"1717575960215021.758\",\"bp\":\""
        "e\"},{\"name\":\"WaitExecute\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208020.758\""
        ",\"dur\":0.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":15}},{\"name\":\""
        "process_name\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
        "Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\","
        "\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":10328512,\""
        "tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":15}},{\"name\":\"thread_name\",\"pid\":"
        "10328512,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":\""
        "thread_sort_index\",\"pid\":10328512,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(std::string("[") + expectStr.substr(0, expectStr.size() - 1), res.back());
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithLogicStreamSuccess)
{
    AscendHardwareAssembler assembler;
    using LogicStreamFormat = std::unordered_map<uint32_t, uint32_t>;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    std::shared_ptr<std::vector<ApiData>> apiS;
    std::shared_ptr<std::vector<MemcpyInfoData>> memcpyS;
    std::shared_ptr<LogicStreamFormat> logicStreamS;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    auto memcpyInfoData = GenerateMemcpyInfoData();
    LogicStreamFormat logicStream = {{1, 1337}};
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    MAKE_SHARED_NO_OPERATION(apiS, std::vector<ApiData>, api);
    MAKE_SHARED_NO_OPERATION(memcpyS, std::vector<MemcpyInfoData>, memcpyInfoData);
    MAKE_SHARED_NO_OPERATION(logicStreamS, LogicStreamFormat, logicStream);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    dataInventory_.Inject(apiS);
    dataInventory_.Inject(memcpyS);
    dataInventory_.Inject(logicStreamS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));  // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expStr =
        "{\"name\":\"MatMulV3\",\"pid\":10328512,\"tid\":1337,\"ts\":\"1717575960208020.758\",\"dur\""
        ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
        "AI_VECTOR_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,\""
        "connection_id\":2345,\"Write "
        "Latency(ns)\":0.0}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328512,\"tid\""
        ":1337,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":\""
        "1717575960208020.758\",\"bp\":\"e\"},{\"name\":\"EVENT_RECORD\",\"pid\":10328512,\"tid\""
        ":1337,\"ts\":\"1717575960208020.758\",\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\""
        "Model Id\":4294967295,\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\"Task Id\""
        ":11,\"Batch Id\":1,\"Subtask Id\":4294967295,\"connection_id\":1234,\"Write Latency(ns)\":0.0}},{\"name\":\""
        "HostToDevice5299989643264\",\"pid\":10328512,\"tid\":1337,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"5299989643264\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"},{\""
        "name\":\"EVENT_WAIT\",\"pid\":10328512,\"tid\":1337,\"ts\":\"1717575960208021.200\",\"dur\""
        ":0.05,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"EVENT_WAIT_SQE\",\""
        "Physic Stream Id\":1,\"Task Id\":21,\"Batch Id\":1,\"Subtask Id\":4294967295,\""
        "connection_id\":1235,\"Write "
        "Latency(ns)\":0.0}},{\"name\":\"HostToDevice5304284610560\",\"pid\":10328512,\"tid\""
        ":1337,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"5304284610560\",\"ts\":\""
        "1717575960208021.200\",\"bp\":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\""
        ":1337,\"ts\":\"1717575960208021.758\",\"dur\":4241.26,\"ph\":\"X\",\"args\":{\"Model Id\""
        ":4294967295,\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":0,\""
        "Batch Id\":0,\"Subtask Id\":4294967295,\"connection_id\":181,\"Write Latency(ns)\":0.0,\"size(B)\":67108864,\""
        "bandwidth(GB/s)\":14.736186887858798,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1337,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960208021.758\",\"bp\":\""
        "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1337,\"ts\":\""
        "1717575960213021.758\",\"dur\":1202.5,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":1,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":181,\"Write "
        "Latency(ns)\":0.0,\"size(B)\":16777216,\"bandwidth(GB/s)\""
        ":12.993762993762992,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice777389080576\",\"pid\":10328512,\"tid\":1337,\"ph\":\"f\",\"cat\""
        ":\"HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960213021.758\",\"bp\""
        ":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328512,\"tid\":1337,\"ts\":\""
        "1717575960215021.758\",\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\""
        "Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id\":0,\""
        "Subtask Id\":4294967295,\"connection_id\":189,\"Write "
        "Latency(ns)\":0.0,\"size(B)\":35651584,\"bandwidth(GB/s)\""
        ":10.9332296091409,\"operation\":\"host to device\"}},{\"name\":\""
        "HostToDevice811748818944\",\"pid\":10328512,\"tid\":1337,\"ph\":\"f\",\"cat\""
        ":\"HostToDevice\",\"id\":\"811748818944\",\"ts\":\"1717575960215021.758\",\"bp\""
        ":\"e\"},{\"name\":\"WaitExecute\",\"pid\":10328512,\"tid\":1337,\"ts\":\""
        "1717575960208020.758\",\"dur\":0.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\""
        "Task Id\":15}},{\"name\":\"process_name\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\",\"args\""
        ":{\"name\":\"Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328512,\"tid\":0,\""
        "ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\""
        ":10328512,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":15}},{\"name\":\"thread_name\""
        ",\"pid\":10328512,\"tid\":1337,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1337\"}},{\"name\""
        ":\"thread_sort_index\",\"pid\":10328512,\"tid\":1337,\"ph\":\"M\",\"args\""
        ":{\"sort_index\":1337}},";
    EXPECT_EQ(std::string("[") + expStr.substr(0, expStr.size() - 1), res.back());
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithSimtTaskType)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    auto task = GenerateSimtTaskData();
    auto info = GenerateSimtTaskInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));  // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr =
        "{\"name\":\"MatMulV3\",\"pid\":10328512,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\""
        ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
        "KERNEL_SIMT\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,\""
        "connection_id\":2345,\"Write Latency(ns)\":0.0,\"Grid Dim\":\"2,3,4\",\"Block Dim\":\"5,6,7\"}},{\"name\":\""
        "HostToDevice10071698309120\",\"pid\":10328512,\"tid\":1,\"ph\":\"f\",\"cat\":\""
        "HostToDevice\",\"id\":\"10071698309120\",\"ts\":\"1717575960208020.758\",\"bp\":\""
        "e\"},{\"name\":\"process_name\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\""
        ":\"Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328512,\"tid\":0,\"ph\":\"M\""
        ",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":10328512,\"tid\""
        ":0,\"ph\":\"M\",\"args\":{\"sort_index\":15}},{\"name\":\"thread_name\",\"pid\":10328512,\""
        "tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\",\""
        "pid\":10328512,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(std::string("[") + expectStr.substr(0, expectStr.size() - 1), res.back());
}

static std::vector<AscendTaskData> GenerateModelExecuteTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;                     // deviceId 0
    data.indexId = -1;                     // index_id -1
    data.streamId = 46;                    // streamId 46
    data.taskId = 33024;                   // taskId 33024
    data.contextId = UINT32_MAX;           // contextId invalid
    data.batchId = 0;                      // batchId 0
    data.connectionId = 251178;            // connectionId 251178
    data.timestamp = 1717575960208020758;  // start 1717575960208020758
    data.duration = 3.28;                  // dur 3.28
    data.hostType = "MODEL_EXECUTE";
    data.deviceType = "MODEL_EXECUTE";
    data.taskType = "MODEL_EXECUTE";
    res.push_back(data);
    return res;
}

TEST_F(AscendHardwareAssemblerUTest, ShouldGenerateModelExecuteFlowWhenTaskTypeIsModelExecute)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    auto task = GenerateModelExecuteTaskData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    dataInventory_.Inject(taskS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));  // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string taskStr =
        "{\"name\":\"MODEL_EXECUTE\",\"pid\":10328512,\"tid\":46,\"ts\":\"1717575960208020.758\",\"dur\""
        ":0.00328,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"MODEL_EXECUTE\",\"Physic "
        "Stream Id\":46,\"Task Id\":33024,\"Batch Id\":0,\"Subtask Id\":4294967295,\"connection_id\":251178,"
        "\"Write Latency(ns)\":0.0}}";
    std::string flowStr =
        "{\"name\":\"HostToDevice(Graph)1078801295474688\",\"pid\":10328512,\"tid\":46,\"ph\":\"f\",\"cat\":"
        "\"HostToDevice(Graph)\",\"id\":\"1078801295474688\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"}";
    const std::string &content = res.back();
    EXPECT_NE(std::string::npos, content.find(taskStr));
    EXPECT_NE(std::string::npos, content.find(flowStr));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldNotGenerateModelExecuteFlowWhenConnectionIdIsInvalid)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    auto task = GenerateModelExecuteTaskData();
    task.front().connectionId = UINT64_MAX;  // 结构体默认值，表示无 connection id
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    dataInventory_.Inject(taskS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    const std::string &content = res.back();
    // task 事件本身仍输出，但不允许出现 ModelExecute 的 flow 终点
    EXPECT_NE(std::string::npos, content.find("\"Task Type\":\"MODEL_EXECUTE\""));
    EXPECT_EQ(std::string::npos, content.find("\"cat\":\"HostToDevice(Graph)\""));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldMatchPmuStagesByDeviceAndTaskIdentityWithinTaskDuration)
{
    auto tasks = GenerateTaskData();
    tasks.resize(1);
    tasks.front().writeLatency = 12.5;
    auto taskData = std::make_shared<std::vector<AscendTaskData>>(tasks);
    BlockDetailData block;
    block.deviceId = tasks.front().deviceId;
    block.streamId = tasks.front().streamId;
    block.taskId = tasks.front().taskId;
    block.batchId = tasks.front().batchId;
    block.subtaskId = tasks.front().contextId;
    block.coreType = 0;
    block.timestamp = tasks.front().timestamp + 10;
    block.fakeEndTime = tasks.front().timestamp + 125;
    std::vector<BlockDetailData> blocks{block};
    auto other = block;
    other.timestamp--;
    other.fakeEndTime = tasks.front().timestamp + 400;
    other.deviceId++;
    blocks.push_back(other);
    other = block;
    other.streamId++;
    blocks.push_back(other);
    other = block;
    other.batchId++;
    blocks.push_back(other);
    other = block;
    other.subtaskId++;
    blocks.push_back(other);
    other = block;
    other.coreType = 1;
    other.fakeEndTime = tasks.front().timestamp + 1000;
    blocks.push_back(other);
    dataInventory_.Inject(taskData);
    dataInventory_.Inject(std::make_shared<std::vector<BlockDetailData>>(blocks));
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));
    AscendHardwareAssembler assembler;
    ASSERT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    const auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_FALSE(lines.empty());
    EXPECT_NE(std::string::npos, lines.back().find("\"AIC Preprocessing Stage(ns)\":125"));
    EXPECT_NE(std::string::npos, lines.back().find("\"Write Latency(ns)\":12.5"));
    EXPECT_EQ(std::string::npos, lines.back().find("AIV Preprocessing Stage(ns)"));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldNotReplaceEarliestBlockWithMissingFakeEndTime)
{
    auto tasks = GenerateTaskData();
    tasks.resize(1);
    BlockDetailData block;
    block.deviceId = tasks.front().deviceId;
    block.streamId = tasks.front().streamId;
    block.taskId = tasks.front().taskId;
    block.batchId = tasks.front().batchId;
    block.subtaskId = tasks.front().contextId;
    block.coreType = 0;
    block.timestamp = tasks.front().timestamp + 10;
    block.fakeEndTime = tasks.front().timestamp + 125;
    std::vector<BlockDetailData> blocks{block};
    block.timestamp--;
    block.fakeEndTime = 0;
    blocks.push_back(block);
    dataInventory_.Inject(std::make_shared<std::vector<AscendTaskData>>(tasks));
    dataInventory_.Inject(std::make_shared<std::vector<BlockDetailData>>(blocks));
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));
    AscendHardwareAssembler assembler;
    ASSERT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    const auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(std::string::npos, lines.back().find("AIC Preprocessing Stage(ns)"));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldMatchRepeatedExecutionsToTheirOwnPmuBlocks)
{
    auto prototype = GenerateTaskData().front();
    prototype.timestamp = 1000;
    prototype.duration = 500;
    std::vector<AscendTaskData> tasks;
    std::vector<BlockDetailData> blocks;
    for (uint64_t round = 0; round < 33; ++round)
    {
        auto task = prototype;
        task.timestamp += round * 1000;
        tasks.insert(tasks.begin(), task);  // Task input need not be chronological.
        for (uint64_t coreType = 0; coreType < 2; ++coreType)
        {
            BlockDetailData block;
            block.deviceId = task.deviceId;
            block.streamId = task.streamId;
            block.taskId = task.taskId;
            block.batchId = task.batchId;
            block.subtaskId = task.contextId;
            block.coreType = coreType;
            block.timestamp = task.timestamp;
            block.fakeEndTime = task.timestamp + 100 + round + coreType;
            blocks.insert(blocks.begin(), block);
            block.timestamp += 10;
            block.fakeEndTime = task.timestamp + 400;
            blocks.insert(blocks.begin(), block);  // A later core must not replace the first block.
        }
    }
    // An old block must not match merely because its fake end overlaps the first task.
    auto stale = blocks.back();
    stale.timestamp = prototype.timestamp - 1;
    stale.fakeEndTime = prototype.timestamp + 499;
    blocks.push_back(stale);
    auto missing = prototype;
    missing.timestamp = 50000;  // No block in this execution's time window.
    tasks.push_back(missing);
    dataInventory_.Inject(std::make_shared<std::vector<AscendTaskData>>(tasks));
    dataInventory_.Inject(std::make_shared<std::vector<BlockDetailData>>(blocks));
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));
    AscendHardwareAssembler assembler;
    ASSERT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    DeliverableConsumer::Instance().FinishTimeline();
    const auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_FALSE(lines.empty());
    const auto events = nlohmann::json::parse(lines.back());
    size_t taskCount = 0;
    size_t stageCount = 0;
    for (const auto &event : events)
    {
        if (!event.contains("args") || !event["args"].contains("Task Id"))
        {
            continue;
        }
        ASSERT_LT(taskCount, tasks.size());
        const auto &args = event["args"];
        for (const std::string core : {"AIC", "AIV"})
        {
            const auto field = core + " Preprocessing Stage(ns)";
            if (tasks[taskCount].timestamp == missing.timestamp)
            {
                EXPECT_FALSE(args.contains(field));
                continue;
            }
            ASSERT_TRUE(args.contains(field));
            const auto expected = 100 + (tasks[taskCount].timestamp - prototype.timestamp) / 1000 + (core == "AIV");
            EXPECT_EQ(expected, args[field].get<uint64_t>());
            EXPECT_EQ(500 - expected, args[core + " ComputeExecution Stage(ns)"].get<uint64_t>());
            ++stageCount;
        }
        ++taskCount;
    }
    EXPECT_EQ(34ul, taskCount);
    EXPECT_EQ(66ul, stageCount);
}

TEST_F(AscendHardwareAssemblerUTest, ShouldValidatePmuStagesBeforeUnsignedConversion)
{
    const double durations[] = {0, 500, 500.25, 9007199254740992.0};
    const uint64_t stages[] = {1, 500, 125, 9007199254740993ULL};
    std::vector<AscendTaskData> tasks;
    std::vector<BlockDetailData> blocks;
    for (size_t i = 0; i < 4; ++i)
    {
        auto task = GenerateTaskData().front();
        task.taskId = i;
        task.timestamp = 1000;
        task.duration = durations[i];
        tasks.push_back(task);
        BlockDetailData block;
        block.deviceId = task.deviceId;
        block.streamId = task.streamId;
        block.taskId = task.taskId;
        block.batchId = task.batchId;
        block.subtaskId = task.contextId;
        block.coreType = 0;
        block.timestamp = task.timestamp;
        block.fakeEndTime = task.timestamp + stages[i];
        blocks.push_back(block);
    }
    dataInventory_.Inject(std::make_shared<std::vector<AscendTaskData>>(tasks));
    dataInventory_.Inject(std::make_shared<std::vector<BlockDetailData>>(blocks));
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086));
    AscendHardwareAssembler assembler;
    ASSERT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    DeliverableConsumer::Instance().FinishTimeline();
    const auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());
    FileReader reader(files.front());
    std::vector<std::string> lines;
    ASSERT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_FALSE(lines.empty());
    size_t count = 0;
    for (const auto &event : nlohmann::json::parse(lines.back()))
    {
        if (!event.contains("args") || !event["args"].contains("Task Id"))
        {
            continue;
        }
        const auto &args = event["args"];
        const auto id = args["Task Id"].get<uint32_t>();
        if (id == 0 || id == 3)
        {
            EXPECT_FALSE(args.contains("AIC Preprocessing Stage(ns)"));
            EXPECT_FALSE(args.contains("AIC ComputeExecution Stage(ns)"));
        }
        else
        {
            EXPECT_EQ(stages[id], args["AIC Preprocessing Stage(ns)"].get<uint64_t>());
            EXPECT_EQ(id == 1 ? 0ul : 375ul, args["AIC ComputeExecution Stage(ns)"].get<uint64_t>());
        }
        ++count;
    }
    EXPECT_EQ(4ul, count);
}
