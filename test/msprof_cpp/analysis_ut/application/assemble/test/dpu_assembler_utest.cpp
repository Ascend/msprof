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

#include "analysis/csrc/application/timeline/dpu_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/application/database/db_constant.h"
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
const std::string BASE_PATH = "./dpu_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}  // namespace

class DPUAssemblerUTest : public testing::Test
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

static std::vector<DPUData> GenerateDPUTrackData()
{
    std::vector<DPUData> res;
    DPUData data;
    data.isHccl = false;
    data.dpuDeviceId = 0;
    data.threadId = 12345;
    data.streamId = 1;
    data.taskId = 100;
    data.taskType = "AI_CPU";
    data.opName = "dpu_conv2d";
    data.timestamp = 1717575960208020750;
    data.endTime = 1717575960209010750;
    res.push_back(data);
    return res;
}

static std::vector<DPUData> GenerateDPUHcclTrackData()
{
    std::vector<DPUData> res;
    DPUData data;
    data.isHccl = true;
    data.npuDeviceId = 0;
    data.dpuDeviceId = 0;
    data.threadId = 12345;
    data.streamId = 2;
    data.taskId = 200;
    data.aicpuTaskId = 300;
    data.planeId = 1;
    data.opType = "MAX";
    data.opName = "allreduce_op";
    data.groupName = "hccl_world_group";
    data.groupNameId = "1";
    data.localRank = 0;
    data.remoteRank = 1;
    data.rankSize = 8;
    data.durationEstimated = 100.5;
    data.srcAddr = "777777777";
    data.dstAddr = "888888888888";
    data.dataSize = 1048576;
    data.notifyId = "5555555";
    data.dataType = "FP32";
    data.linkType = "RoCE";
    data.transportType = "RDMA";
    data.rdmaType = "RDMA";
    data.timestamp = 1717575960208020750;
    data.endTime = 1717575960209010750;
    res.push_back(data);
    return res;
}

static std::vector<DPUData> GenerateMixedDPUData()
{
    std::vector<DPUData> res;

    DPUData trackData;
    trackData.isHccl = false;
    trackData.dpuDeviceId = 0;
    trackData.threadId = 12345;
    trackData.streamId = 1;
    trackData.taskId = 100;
    trackData.taskType = "AI_CPU";
    trackData.opName = "dpu_conv2d";
    trackData.timestamp = 1717575960208020750;
    trackData.endTime = 1717575960209010750;
    res.push_back(trackData);

    DPUData hcclData;
    hcclData.isHccl = true;
    hcclData.npuDeviceId = 0;
    hcclData.dpuDeviceId = 0;
    hcclData.threadId = 12345;
    hcclData.streamId = 2;
    hcclData.taskId = 200;
    hcclData.aicpuTaskId = 300;
    hcclData.planeId = 1;
    hcclData.opType = "MAX";
    hcclData.opName = "allreduce_op";
    hcclData.groupName = "hccl_world_group";
    hcclData.groupNameId = "1";
    hcclData.localRank = 0;
    hcclData.remoteRank = 1;
    hcclData.rankSize = 8;
    hcclData.durationEstimated = 100.5;
    hcclData.srcAddr = "777777777";
    hcclData.dstAddr = "888888888888";
    hcclData.dataSize = 1048576;
    hcclData.notifyId = "5555555";
    hcclData.dataType = "FP32";
    hcclData.linkType = "RoCE";
    hcclData.transportType = "RDMA";
    hcclData.rdmaType = "RDMA";
    hcclData.timestamp = 1717575960210000000;
    hcclData.endTime = 1717575960211000000;
    res.push_back(hcclData);

    return res;
}

TEST_F(DPUAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    DPUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(DPUAssemblerUTest, ShouldReturnTrueWhenDPUTrackDataAssembleSuccess)
{
    DPUAssembler assembler;
    std::shared_ptr<std::vector<DPUData>> dataS;
    auto data = GenerateDPUTrackData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr =
        "{\"name\":\"process_name\",\"pid\":1024256,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
        "\"CANN\"}},{\"name\":\"process_labels\",\"pid\":1024256,\"tid\":0,\"ph\":\"M\",\"args\":{"
        "\"labels\":\"DPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":1024256,\"tid\":0,\"ph\":"
        "\"M\",\"args\":{\"sort_index\":8}},{\"name\":\"thread_name\",\"pid\":1024256,\"tid\":"
        "1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\","
        "\"pid\":1024256,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},{\"name\":"
        "\"dpu_conv2d\",\"pid\":1024256,\"tid\":1,\"ts\":\"1717575960208020.750\",\"dur\":"
        "990.0,\"ph\":\"X\",\"args\":{\"Thread Id\":12345,\"Physic Stream Id\":1,"
        "\"Task Id\":100,\"Task Type\":\"AI_CPU\"}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DPUAssemblerUTest, ShouldReturnTrueWhenDPUHcclTrackDataAssembleSuccess)
{
    DPUAssembler assembler;
    std::shared_ptr<std::vector<DPUData>> dataS;
    auto data = GenerateDPUHcclTrackData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr =
        "{\"name\":\"process_name\",\"pid\":1024256,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
        "\"CANN\"}},{\"name\":\"process_labels\",\"pid\":1024256,\"tid\":0,\"ph\":\"M\",\"args\":{"
        "\"labels\":\"DPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":1024256,\"tid\":0,\"ph\":"
        "\"M\",\"args\":{\"sort_index\":8}},{\"name\":\"thread_name\",\"pid\":1024256,\"tid\":"
        "2,\"ph\":\"M\",\"args\":{\"name\":\"Stream 2\"}},{\"name\":\"thread_sort_index\","
        "\"pid\":1024256,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},{\"name\":"
        "\"allreduce_op\",\"pid\":1024256,\"tid\":2,\"ts\":\"1717575960208020.750\",\"dur\":"
        "990.0,\"ph\":\"X\",\"args\":{\"Thread Id\":12345,\"Physic Stream Id\":2,\"Task Id\":"
        "200,\"OP Type\":\"MAX\",\"AI CPU Device Id\":0,\"AI CPU Task Id\":300,"
        "\"Group Name\":\"hccl_world_group(1)\",\"Plane Id\":1,\"Notify Id\":\"5555555\","
        "\"Duration Estimated(us)\":100.5,\"Rank Size\":8,"
        "\"Src Rank\":0,\"Dst Rank\":1,\"Transport Type\":\"RDMA\",\"Size(Byte)\":1048576,"
        "\"Bandwidth(GB/s)\":1.0591676767676768,\"Data Type\":\"FP32\",\"Link Type\":\"RoCE\","
        "\"Rdma Type\":\"RDMA\"}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DPUAssemblerUTest, ShouldReturnTrueWhenMixedDPUDataAssembleSuccess)
{
    DPUAssembler assembler;
    std::shared_ptr<std::vector<DPUData>> dataS;
    auto data = GenerateMixedDPUData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_GT(res.size(), 0ul);
    std::string result = res.back();
    EXPECT_TRUE(result.find("dpu_conv2d") != std::string::npos);
    EXPECT_TRUE(result.find("allreduce_op") != std::string::npos);
    EXPECT_TRUE(result.find("Stream 1") != std::string::npos);
    EXPECT_TRUE(result.find("Stream 2") != std::string::npos);
}

TEST_F(DPUAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    DPUAssembler assembler;
    std::shared_ptr<std::vector<DPUData>> dataS;
    auto data = GenerateDPUTrackData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000));
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(DPUAssemblerUTest, ShouldReturnTrueWhenMultipleDevicesAndStreams)
{
    DPUAssembler assembler;
    std::shared_ptr<std::vector<DPUData>> dataS;
    std::vector<DPUData> data;

    DPUData data1;
    data1.isHccl = false;
    data1.dpuDeviceId = 0;
    data1.threadId = 12345;
    data1.streamId = 1;
    data1.taskId = 100;
    data1.taskType = "AI_CPU";
    data1.opName = "kernel_0";
    data1.timestamp = 1717575960208020750;
    data1.endTime = 1717575960209010750;
    data.push_back(data1);

    DPUData data2;
    data2.isHccl = false;
    data2.dpuDeviceId = 0;
    data2.threadId = 12345;
    data2.streamId = 2;
    data2.taskId = 101;
    data2.taskType = "AI_CPU";
    data2.opName = "kernel_1";
    data2.timestamp = 1717575960210000000;
    data2.endTime = 1717575960211000000;
    data.push_back(data2);

    DPUData data3;
    data3.isHccl = false;
    data3.dpuDeviceId = 1;
    data3.threadId = 12346;
    data3.streamId = 1;
    data3.taskId = 102;
    data3.taskType = "AI_CPU";
    data3.opName = "kernel_2";
    data3.timestamp = 1717575960212000000;
    data3.endTime = 1717575960213000000;
    data.push_back(data3);

    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DPUData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string result = res.back();
    EXPECT_TRUE(result.find("kernel_0") != std::string::npos);
    EXPECT_TRUE(result.find("kernel_1") != std::string::npos);
    EXPECT_TRUE(result.find("kernel_2") != std::string::npos);
    EXPECT_TRUE(result.find("Stream 1") != std::string::npos);
    EXPECT_TRUE(result.find("Stream 2") != std::string::npos);
}
