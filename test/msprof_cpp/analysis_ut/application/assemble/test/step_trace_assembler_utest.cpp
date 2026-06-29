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
#include "analysis/csrc/application/timeline/step_trace_assembler.h"
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
const std::string BASE_PATH = "./step_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
std::string expectStr = "{\"name\":\"Iteration 1\",\"pid\":10248640,\"tid\":70013,\"ts\":\"830073224.426\",\"dur\":"
                        "9598.772,\"ph\":\"X\",\"cat\":\"Iteration Time\",\"args\":{\"Iteration ID\":1,\"FP Start\":\""
                        "830074691.065\",\"BP End\":\"830082803.479\",\"Iteration End\":\"830082823.198\",\""
                        "Iteration Time(us)\":9598.772}},{\"name\":\"FP_BP Time 1\",\"pid\":10248640,\"tid\":70013,\""
                        "ts\":\"830074691.065\",\"dur\":8112.414,\"ph\":\"X\",\"cat\":\"FP_BP Time\",\"args\":{\""
                        "Iteration ID\":1,\"FP Start\":\"830074691.065\",\"BP End\":\"830082803.479\",\""
                        "FP_BP Time(us)\":8112.414}},{\"name\":\"Iteration Refresh 1\",\"pid\":10248640,\"tid\":70013,"
                        "\"ts\":\"830082803.479\",\"dur\":19.719,\"ph\":\"X\",\"cat\":\"Iteration Refresh\",\"args"
                        "\":{\"Iteration ID\":1,\"BP End\":\"830082803.479\",\"Iteration End\":\"830082823.198\""
                        ",\"Iteration Refresh(us)\":19.719}},{\"name\":\"Data_aug Bound 0\",\"pid\":10248640,\"tid\":"
                        "70013,\"ph\":\"t\",\"cat\":\"Data_aug Bound\",\"id\":\"Data_aug Bound 10248640_70013\",\"ts\""
                        ":\"830074691.065\",\"args\":{\"Iteration ID\":1}},{\"name\":\"Data_aug Bound 1\",\"pid\":"
                        "10248640,\"tid\":70013,\"ph\":\"s\",\"cat\":\"Data_aug Bound\",\"id\":\""
                        "Data_aug Bound 10248640_70013\",\"ts\":\"833650265.121\",\"args\":{\"Data_aug Bound(us)\":"
                        "7134923.284}},{\"name\":\"process_name\",\"pid\":10248640,\"tid\":0,\"ph\":\"M\",\"args\":{\""
                        "name\":\"Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10248640,\"tid\":0,\"ph\":"
                        "\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":10248640,\"tid"
                        "\":0,\"ph\":\"M\",\"args\":{\"sort_index\":14}},{\"name\":\"thread_name\",\"pid\":10248640,\""
                        "tid\":70013,\"ph\":\"M\",\"args\":{\"name\":\"Step Trace(Model ID:13)\"}},{\"name\":\""
                        "thread_sort_index\",\"pid\":10248640,\"tid\":70013,\"ph\":\"M\",\"args\":{\"sort_index\":"
                        "70013}},{\"name\":\"Reduce_1_0\",\"pid\":10248640,\"tid\":70013,\"ts\":\"830082637.434\","
                        "\"dur\":30.312,\"ph\":\"X\",\"cat\":\"Reduce\",\"args\":{\"Iteration ID\":1,\"Reduce End 0\":"
                        "\"830082667746\",\"Reduce Start 0\":\"830082637434\"}},{\"name\":\"Reduce_1_1\",\"pid\":"
                        "10248640,\"tid\":70013,\"ts\":\"830082804.063\",\"dur\":6.556,\"ph\":\"X\",\"cat\":\""
                        "Reduce\",\"args\":{\"Iteration ID\":1,\"Reduce End 1\":\"830082810619\",\"Reduce Start 1\":\""
                        "830082804063\"}},{\"name\":\"GetNext\",\"pid\":10248640,\"tid\":70013,\"ts\":\""
                        "830074693.926\",\"dur\":31.601,\"ph\":\"X\",\"cat\":\"GetNext Time\",\"args\":{\""
                        "GetNext Start\":\"830074693.926\",\"GetNext End\":\"830074725.527\",\"GetNext Time(us)"
                        "\":31.601}},{\"name\":\"GetNext\",\"pid\":10248640,\"tid\":70013,\"ts\":\"830074767.363\","
                        "\"dur\":19.74,\"ph\":\"X\",\"cat\":\"GetNext Time\",\"args\":{\"GetNext Start\":\""
                        "830074767.363\",\"GetNext End\":\"830074787.103\",\"GetNext Time(us)\":19.74}},{\"name"
                        "\":\"GetNext\",\"pid\":10248640,\"tid\":70013,\"ts\":\"830074801.783\",\"dur\":18.966,\"ph"
                        "\":\"X\",\"cat\":\"GetNext Time\",\"args\":{\"GetNext Start\":\"830074801.783\",\""
                        "GetNext End\":\"830074820.749\",\"GetNext Time(us)\":18.966}},";
}

class StepTraceAssemblerUTest : public testing::Test {
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

static std::vector<GetNextData> GenerateNextData()
{
    std::vector<GetNextData> res;
    GetNextData d1;
    d1.deviceId = 0; // deviceId 0
    d1.modelId = 13; // 13
    d1.indexId = 1; // 1
    d1.timestamp = 830074693926; // start 830074693926
    d1.end = 830074725527; // end 830074725527
    res.push_back(d1);
    d1.timestamp = 830074767363; // start 830074767363
    d1.end = 830074787103; // end 830074787103
    res.push_back(d1);
    d1.timestamp = 830074801783; // start 830074801783
    d1.end = 830074820749; // end 830074820749
    res.push_back(d1);
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

TEST_F(StepTraceAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    StepTraceAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(StepTraceAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    StepTraceAssembler assembler;
    std::shared_ptr<std::vector<TrainTraceData>> trainS;
    std::shared_ptr<std::vector<GetNextData>> nextS;
    std::shared_ptr<std::vector<AllReduceData>> reduceS;
    auto train = GenerateTrainData();
    auto next = GenerateNextData();
    auto reduce = GenerateReduceData();
    MAKE_SHARED_NO_OPERATION(trainS, std::vector<TrainTraceData>, train);
    MAKE_SHARED_NO_OPERATION(nextS, std::vector<GetNextData>, next);
    MAKE_SHARED_NO_OPERATION(reduceS, std::vector<AllReduceData>, reduce);
    dataInventory_.Inject(trainS);
    dataInventory_.Inject(nextS);
    dataInventory_.Inject(reduceS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10008)); // pid 10008
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(StepTraceAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    StepTraceAssembler assembler;
    std::shared_ptr<std::vector<TrainTraceData>> trainS;
    std::shared_ptr<std::vector<GetNextData>> nextS;
    std::shared_ptr<std::vector<AllReduceData>> reduceS;
    auto train = GenerateTrainData();
    auto next = GenerateNextData();
    auto reduce = GenerateReduceData();
    MAKE_SHARED_NO_OPERATION(trainS, std::vector<TrainTraceData>, train);
    MAKE_SHARED_NO_OPERATION(nextS, std::vector<GetNextData>, next);
    MAKE_SHARED_NO_OPERATION(reduceS, std::vector<AllReduceData>, reduce);
    dataInventory_.Inject(trainS);
    dataInventory_.Inject(nextS);
    dataInventory_.Inject(reduceS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10008)); // pid 10008
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
