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
#include "analysis/csrc/application/timeline/hccl_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
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
const std::string BASE_PATH = "./hccl_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class HcclAssemblerUTest : public testing::Test {
protected:
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
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
protected:
    DataInventory dataInventory_;
};

static std::vector<CommunicationOpData> GenerateOpData()
{
    std::vector<CommunicationOpData> res;
    CommunicationOpData data;
    data.opName = "hcom_broadcast__674_0_1";
    data.groupName = "16898834563344171674";
    data.connectionId = 2762; // connectionId 2762
    data.opKey = "16898834563344171674-1-1-1-1";
    data.timestamp = 1717575960213957957; // start 1717575960213957957
    data.end = 1717575960214957957; // end 1717575960214957957
    data.relay = 0; // relay 0
    data.retry = 0; // retry 0
    data.dataType = 1; // dataType 1
    data.algType = "MESH-RING";
    data.count = 5; // count 5
    data.opType = "hcom_broadcast_";
    data.modelId = 4294967295; // modelId 4294967295
    data.deviceId = 0; // device 0
    data.rankSize = 8;
    res.push_back(data);
    return res;
}

static std::vector<CommunicationTaskData> GenerateTaskData()
{
    std::vector<CommunicationTaskData> res;
    CommunicationTaskData data;
    data.planeId = 0; // planeId 0
    data.modelId = 4294967295; // modelId 4294967295
    data.streamId = 1; // streamId 1
    data.taskId = 1; // taskId 1
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.srcRank = 0; // src 0
    data.dstRank = 1; // dst 1
    data.opKey = "16898834563344171674-1-1-1-1";
    data.deviceId = 0; // device 0
    data.opName = "hcom_broadcast__674_0_1";
    data.taskType = "Notify_Wait";
    data.groupName = "16898834563344171674";
    data.transportType = 2; // transport 2
    data.size = 3200; // size 3200
    data.dataType = UINT16_MAX;
    data.linkType = UINT16_MAX;
    data.notifyId = "456";
    data.rdmaType = UINT16_MAX;
    data.timestamp = 1717575960213957957; // start 1717575960213957957
    data.duration = 1000000.0; // dur 1000000.0
    data.durationEstimated = 20.0; // es_dur 20.0
    data.bandwidth = 0.0; // bw 0.0
    res.push_back(data);
    data.planeId = 1; // planeId 1
    data.modelId = 4294967295; // modelId 4294967295
    data.streamId = 1; // streamId 1
    data.taskId = 1; // taskId 1
    data.contextId = 2; // contextId 2
    data.batchId = 1; // batchId 1
    res.push_back(data);
    return res;
}

TEST_F(HcclAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    HcclAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(HcclAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    HcclAssembler assembler;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    std::shared_ptr<std::vector<CommunicationTaskData>> taskS;
    auto opData = GenerateOpData();
    auto task = GenerateTaskData();
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<CommunicationTaskData>, task);
    dataInventory_.Inject(opDataS);
    dataInventory_.Inject(taskS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10085)); // pid 10085
    MOCKER_CPP(&Context::IsLevel0).stubs().will(returnValue(false));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":10327552,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
                            "Communication\"}},{\"name\":\"process_labels\",\"pid\":10327552,\"tid\":0,\"ph\":\"M\",\""
                            "args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":10327552,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":16}},{\"name\":\"thread_name\","
                            "\"pid\":10327552,\""
                            "tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Group 16898834563344171674 Communication\"}},{"
                            "\"name\":\"thread_sort_index\",\"pid\":10327552,\"tid\":0,\"ph\":\"M\",\"args\":{\""
                            "sort_index\":0}},{\"name\":\"thread_name\",\"pid\":10327552,\"tid\":1,\"ph\":\"M\",\"args"
                            "\":{\"name\":\"Plane 0\"}},{\"name\":\"thread_sort_index\",\"pid\":10327552,\"tid\":1,\""
                            "ph\":\"M\",\"args\":{\"sort_index\":1}},{\"name\":\"thread_name\",\"pid\":10327552,\"tid"
                            "\":2,\"ph\":\"M\",\"args\":{\"name\":\"Plane 1\"}},{\"name\":\"thread_sort_index\",\"pid"
                            "\":10327552,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},{\"name\":\"Notify_Wait\""
                            ",\"pid\":10327552,\"tid\":1,\"ts\":\"1717575960213957.957\",\"dur\":1000.0,\"ph\":\"X\","
                            "\"args\":{\"notify_id\":\"456\",\"duration estimated(us)\":20.0,\"stream id\":1,\"task id"
                            "\":1,\"context id\":1,\"task type\":\"Notify_Wait\",\"src rank\":0,\"dst rank\":1,\""
                            "transport type\":\"LOCAL\",\"size(Byte)\":3200,\"data type\":\"INVALID_TYPE\",\"link type"
                            "\":\"INVALID_TYPE\",\"bandwidth(GB/s)\":0.0,\"model id\":4294967295}},{\"name\":\""
                            "Notify_Wait\",\"pid\":10327552,\"tid\":2,\"ts\":\"1717575960213957.957\",\"dur\":1000.0,"
                            "\"ph\":\"X\",\"args\":{\"notify_id\":\"456\",\"duration estimated(us)\":20.0,\"stream id"
                            "\":1,\"task id\":1,\"context id\":2,\"task type\":\"Notify_Wait\",\"src rank\":0,\""
                            "dst rank\":1,\"transport type\":\"LOCAL\",\"size(Byte)\":3200,\"data type\":\""
                            "INVALID_TYPE\",\"link type\":\"INVALID_TYPE\",\"bandwidth(GB/s)\":0.0,\"model id\":"
                            "4294967295}},{\"name\":\"hcom_broadcast__674_0_1\",\"pid\":10327552,\"tid\":0,\"ts\":\""
                            "1717575960213957.957\",\"dur\":1000.0,\"ph\":\"X\",\"args\":{\"rank_size\":8,\"connection_id\":2762,\""
                            "model id\":4294967295,\"data_type\":\"INT16\",\"alg_type\":\"MESH-RING\",\"count\":5,"
                            "\"relay\":\"no\",\"retry\":\"no\"}},{"
                            "\"name\":\"HostToDevice11862699671552\",\"pid\":10327552,\"tid\":0,\"ph\":\"f\",\"cat\":"
                            "\"HostToDevice\",\"id\":\"11862699671552\",\"ts\":\"1717575960213957.957\",\"bp\":"
                            "\"e\"},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HcclAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    HcclAssembler assembler;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    std::shared_ptr<std::vector<CommunicationTaskData>> taskS;
    auto opData = GenerateOpData();
    auto task = GenerateTaskData();
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<CommunicationTaskData>, task);
    dataInventory_.Inject(opDataS);
    dataInventory_.Inject(taskS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
