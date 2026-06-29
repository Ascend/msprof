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
#include "analysis/csrc/application/summary/summary_step_trace_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
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
    const std::string BASE_PATH = "./summary_step_trace_summary_test";
    const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_114514"});
    const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class SummaryStepTraceAssemblerUTest : public testing::Test {
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

static std::vector<TrainTraceData> GenerateTrainData()
{
    std::vector<TrainTraceData> res;
    TrainTraceData data;
    data.deviceId = 0; // deviceId 0
    data.modelId = 13; // modelId 13
    data.indexId = 1; // indexId 1
    data.fpStart = 830074691065; // fp 830074691065
    data.bpEnd = 830082803479; // bp 830082803479
    data.iterEnd = 830082823198; // iterEnd 830082823198
    data.iterTime = 9598772; // iterTime 9598772
    data.fpBpTime = 8112414; // fpBpTime 8112414
    data.gradRefreshBound = 19719; // refresh 19719
    data.dataAugBound = 7134923284; // aug 7134923284
    data.timestamp = data.iterEnd - data.iterTime; // start time
    res.push_back(data);
    return res;
}

static std::vector<AllReduceData> GenerateReduceData()
{
    std::vector<AllReduceData> res;
    AllReduceData data;
    data.deviceId = 0; // deviceId 0
    data.modelId = 13; // modelId 13
    data.indexId = 1; // indexId 1
    data.iterEnd = 830082823198; // iterEnd 830082823198
    data.timestamp = 830082637434; // start 830082637434
    data.end = 830082667746; // end 830082667746
    res.push_back(data);
    data.timestamp = 830082804063; // start 830082804063
    data.end = 830082810619; // end 830082810619
    res.push_back(data);
    return res;
}

TEST_F(SummaryStepTraceAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    SummaryStepTraceAssembler assembler(PROCESSOR_NAME_STEP_TRACE, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"step_trace"}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(SummaryStepTraceAssemblerUTest, ShouldReturnTrueWhenStepTraceAssembleSuccess)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<TrainTraceData>> trainTraceS;
    std::shared_ptr<std::vector<AllReduceData>> allReduceS;
    auto trainData = GenerateTrainData();
    auto reduceData = GenerateReduceData();
    MAKE_SHARED_NO_OPERATION(trainTraceS, std::vector<TrainTraceData>, trainData);
    MAKE_SHARED_NO_OPERATION(allReduceS, std::vector<AllReduceData>, reduceData);
    dataInventory.Inject(trainTraceS);
    dataInventory.Inject(allReduceS);
    SummaryStepTraceAssembler assembler(PROCESSOR_NAME_STEP_TRACE, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"step_trace"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Iteration ID,FP Start(us),BP End(us),Iteration End(us),Iteration Time(us),"
                             "FP to BP Time(us),Iteration Refresh(us),Data Aug Bound(us),Model ID,Reduce Start(us),"
                             "Reduce Duration(us),Reduce Start(us),Reduce Duration(us)"};
    EXPECT_EQ(expectHeader, res[0]);
}

TEST_F(SummaryStepTraceAssemblerUTest, ShouldReturnTrueWhenExistTraceAndAllReduce)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<TrainTraceData>> trainTraceS;
    std::shared_ptr<std::vector<AllReduceData>> allReduceS;
    auto trainData = GenerateTrainData();
    auto reduceData = GenerateReduceData();
    MAKE_SHARED_NO_OPERATION(trainTraceS, std::vector<TrainTraceData>, trainData);
    MAKE_SHARED_NO_OPERATION(allReduceS, std::vector<AllReduceData>, reduceData);
    dataInventory.Inject(trainTraceS);
    dataInventory.Inject(allReduceS);
    SummaryStepTraceAssembler assembler(PROCESSOR_NAME_STEP_TRACE, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"step_trace"}, {});

    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());

    std::string expectHeader{"Device_id,Iteration ID,FP Start(us),BP End(us),Iteration End(us),Iteration Time(us),"
                             "FP to BP Time(us),Iteration Refresh(us),Data Aug Bound(us),Model ID,Reduce Start(us),"
                             "Reduce Duration(us),Reduce Start(us),Reduce Duration(us)"};
    EXPECT_EQ(expectHeader, res[0]);

    std::string expectOneRow = {"0,1,830074691.065\t,830082803.479\t,830082823.198\t,9598.772,8112.414,19.719,"
                                "7134923.284,13,830082637.434\t,18446744073709521.304,830082804.063\t,6.556"};
    EXPECT_EQ(expectOneRow, res[1]);
}