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

#include <iostream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/op_summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Common;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./op_summary_test";
const std::string PROF_PATH = File::PathJoin(std::vector<std::string>{BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin(std::vector<std::string>{PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class OpSummaryAssemblerUTest : public testing::Test {
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

static std::vector<CommunicationOpData> GenerateOpData()
{
    std::vector<CommunicationOpData> res;
    CommunicationOpData data;
    data.deviceId = 0; // deviceId 0
    data.modelId = UINT32_MAX;
    data.opName = "hcom_broadcast__515_1_1";
    data.opType = NA;
    data.timestamp = 1717575960208030858; // start 1717575960208030858
    data.end = 1717575960208031858; // end 1717575960208031858
    res.push_back(data);
    return res;
}

static MetricSummary GenerateMetricSummary()
{
    MetricSummary summary;
    std::map<TaskId, std::vector<std::vector<std::string>>> data;
    std::vector<std::string> row{"140.106", "5043798", "33.09", "0.236", "7.419", "0.053", "81.861", "0.584", "83.023",
                                 "0.593", "17.645", "0.126", "0.006", "0.0", "0", "0.0", "0.0", "0.0", "0.0", "0.0",
                                 "0.0", "0.0", "0.0", "0.0", "14010.535"};
    data[{1, 1, 10, 1, 0}].emplace_back(row);
    row = {"140.106", "5043798", "33.09", "0.236", "7.419", "0.053", "81.861", "0.584", "83.023",
           "0.593", "17.645", "0.126", "0.006", "0.0", "0", "0.0", "0.0", "0.0", "0.0", "0.0",
           "0.0", "0.0", "0.0", "0.0", "N/A"};
    data[{1, 1, 12, UINT32_MAX, 0}].emplace_back(row);
    row = {"140.106", "5043798", "33.09", "0.236", "7.419", "0.053", "81.861", "0.584", "83.023",
           "0.593", "17.645", "0.126", "0.006", "0.0", "0", "0.0", "0.0", "0.0", "0.0", "0.0",
           "0.0", "0.0", "0.0", "0.0", "14010.535"};
    data[{1, 1, 13, UINT32_MAX, 0}].emplace_back(row);
    summary.data = data;
    summary.labels = {"aicore_time(us)", "aic_total_cycles", "aic_mac_time(us)", "aic_mac_ratio",
                      "aic_scalar_time(us)", "aic_scalar_ratio", "aic_mte1_time(us)", "aic_mte1_ratio",
                      "aic_mte2_time(us)", "aic_mte2_ratio", "aic_fixpipe_time(us)", "aic_fixpipe_ratio",
                      "aic_icache_miss_rate", "aiv_time(us)", "aiv_total_cycles", "aiv_vec_time(us)", "aiv_vec_ratio",
                      "aiv_scalar_time(us)", "aiv_scalar_ratio", "aiv_mte2_time(us)", "aiv_mte2_ratio",
                      "aiv_mte3_time(us)", "aiv_mte3_ratio", "aiv_icache_miss_rate", "cube_utilization(%)"};
    return summary;
}

TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(0ul, files.size());
}

// 非Stars芯片HCCL+Task（L0，L1一样）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenTaskAndHcclExistWithNoStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto opData = GenerateOpData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    dataInventory.Inject(opDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(6ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Num,HF32 Eligible,"
                             "Input Shapes,Input Data Types,Input Formats,Output Shapes,Output Data Types,"
                             "Output Formats"};
    EXPECT_EQ(expectHeader, res[0]);
}

// Stars芯片HCCL（L0,L1一样）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenOnlyHcclWithStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    auto opData = GenerateOpData();
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    dataInventory.Inject(opDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    MOCKER_CPP(&Context::IsStarsChip).stubs().will(returnValue(true));
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Context ID"};
    EXPECT_EQ(expectHeader, res[0]);
}

// Stars芯片HCCL+Task+Pmu（最长的表头）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenTaskAndHcclAndPmuExistWithStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    std::shared_ptr<MetricSummary> metricDataS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto opData = GenerateOpData();
    auto metricSummary = GenerateMetricSummary();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    MAKE_SHARED_NO_OPERATION(metricDataS, MetricSummary, metricSummary);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    dataInventory.Inject(opDataS);
    dataInventory.Inject(metricDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    nlohmann::json record = {
        {"DeviceInfo", {{{"hwts_frequency", "1800"}, {"aic_frequency", "1800"}, {"ai_core_num", 20}}}},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(6ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Num,Mix Block Num,"
                             "HF32 Eligible,Input Shapes,Input Data Types,Input Formats,Output Shapes,"
                             "Output Data Types,Output Formats,Context ID,aicore_time(us),aic_total_cycles,"
                             "aic_mac_time(us),aic_mac_ratio,aic_scalar_time(us),aic_scalar_ratio,aic_mte1_time(us),"
                             "aic_mte1_ratio,aic_mte2_time(us),aic_mte2_ratio,aic_fixpipe_time(us),aic_fixpipe_ratio,"
                             "aic_icache_miss_rate,aiv_time(us),aiv_total_cycles,aiv_vec_time(us),aiv_vec_ratio,"
                             "aiv_scalar_time(us),aiv_scalar_ratio,aiv_mte2_time(us),aiv_mte2_ratio,aiv_mte3_time(us),"
                             "aiv_mte3_ratio,aiv_icache_miss_rate,cube_utilization(%)"};
    EXPECT_EQ(expectHeader, res[0]); // 0 表头
    std::string expectRowOne{"0,4294967295,10,1,MatMulV3,,,,1717575960208020.758\t,151.000,0,4294967295"
                             ",4294967295,,,,,,,,1,140.106,5043798,33.09,0.236,7.419,0.053,81.861,0.584,83.023,0.593,"
                             "17.645,0.126,0.006,0.0,0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,92.785"};
    EXPECT_EQ(expectRowOne, res[1]); // 1 首条数据 MatMulV3 正常的算子

    expectRowOne = "0,4294967295,12,1,MatMulV3_1,,,,1717575960208070.758\t,15.000,20.000000"
                   ",4294967295,4294967295,,,,,,,,N/A,140.106,5043798,33.09,0.236,7.419,0.053,81.861,0.584,"
                   "83.023,0.593,17.645,0.126,0.006,0.0,0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,N/A";
    EXPECT_EQ(expectRowOne, res[4]); // 4  第四条数据 MatMulV3_1 cube_utilization 为NA

    expectRowOne = "0,4294967295,13,1,MatMulV3_2,,,,1717575960208090.758\t,0.000,5.000000,4294967295,"
                   "4294967295,,,,,,,,N/A,140.106,5043798,33.09,0.236,7.419,0.053,81.861,0.584,83.023,"
                   "0.593,17.645,0.126,0.006,0.0,0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0";
    EXPECT_EQ(expectRowOne, res[5]); // 5 第五条数据MatMulV3_2 task_dur 为0
}
