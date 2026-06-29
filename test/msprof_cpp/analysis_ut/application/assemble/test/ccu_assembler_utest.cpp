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

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/timeline/ccu_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/application/database/db_constant.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Application;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ccu_assembler_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});

std::vector<CCUMissionTimelineData> BuildFullData()
{
    std::vector<CCUMissionTimelineData> data;

    CCUMissionTimelineData loop;
    loop.deviceId = 0;
    loop.streamId = 1;
    loop.taskId = 10;
    loop.instructionId = 100;
    loop.timestamp = 1717575960208020758;
    loop.duration = 2000.0;
    loop.timeType = CCU_TIME_TYPE_LOOP_GROUP;
    loop.hasDieId = true;
    loop.dieId = 0;
    loop.hasDataSize = true;
    loop.dataSize = 1024;
    loop.hasBandwidth = true;
    loop.bandwidth = 300.5;
    loop.hasReduceInfo = true;
    loop.reduceOpType = "SUM";
    loop.inputDataType = "FP16";
    loop.outputDataType = "FP16";
    data.push_back(loop);

    CCUMissionTimelineData wait;
    wait.deviceId = 0;
    wait.streamId = 1;
    wait.taskId = 10;
    wait.instructionId = 200;
    wait.notifyRankId = 9;
    wait.timestamp = 1717575960208120758;
    wait.duration = 1500.0;
    wait.timeType = CCU_TIME_TYPE_WAIT;
    wait.hasDieId = true;
    wait.dieId = 0;
    wait.hasMask = true;
    wait.mask = 255;
    wait.hasDelayChannel = true;
    wait.maxDelayChannel = 4;
    wait.maxChannelDelay = 120;
    data.push_back(wait);

    return data;
}

std::vector<CCUMissionTimelineData> BuildDataWithoutOptionalFields()
{
    std::vector<CCUMissionTimelineData> data;

    CCUMissionTimelineData loop;
    loop.deviceId = 0;
    loop.streamId = 1;
    loop.taskId = 10;
    loop.instructionId = 100;
    loop.timestamp = 1717575960208020758;
    loop.duration = 2000.0;
    loop.timeType = CCU_TIME_TYPE_LOOP_GROUP;
    data.push_back(loop);

    CCUMissionTimelineData wait;
    wait.deviceId = 0;
    wait.streamId = 1;
    wait.taskId = 10;
    wait.instructionId = 200;
    wait.notifyRankId = 9;
    wait.timestamp = 1717575960208120758;
    wait.duration = 1500.0;
    wait.timeType = CCU_TIME_TYPE_WAIT;
    data.push_back(wait);

    return data;
}
}

class CcuAssemblerUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }

    void TearDown() override
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        MOCKER_CPP(&Context::GetPidFromInfoJson).reset();
        GlobalMockObject::verify();
    }

protected:
    DataInventory dataInventory_;
};

TEST_F(CcuAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(CcuAssemblerUTest, ShouldAssembleSuccessWhenDataValid)
{
    auto fullData = BuildFullData();
    std::shared_ptr<std::vector<CCUMissionTimelineData>> ccuData;
    MAKE_SHARED_NO_OPERATION(ccuData, std::vector<CCUMissionTimelineData>, fullData);
    dataInventory_.Inject(ccuData);

    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10089));

    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());

    FileReader reader(files.back());
    std::vector<std::string> content;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(content));
    ASSERT_FALSE(content.empty());
    const std::string jsonContent = content.back();

    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"LoopGroup\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"Wait\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Physic Stream Id\":1"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Instruction ID\":100"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Notify Instruction ID\":200"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Notify Rank ID\":9"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Die Id\":0"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Data Size\":1024"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Bandwidth (MB/s)\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Reduce Op Type\":\"SUM\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Input Data Type\":\"FP16\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Output Data Type\":\"FP16\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Mask\":255"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Maximum Delay Channel\":4"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"Maximum Channel Delay\":120"));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"process_name\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"thread_name\""));
}

TEST_F(CcuAssemblerUTest, ShouldNotDumpOptionalFieldsWhenFlagsAreFalse)
{
    auto smallData = BuildDataWithoutOptionalFields();
    std::shared_ptr<std::vector<CCUMissionTimelineData>> ccuData;
    MAKE_SHARED_NO_OPERATION(ccuData, std::vector<CCUMissionTimelineData>, smallData);
    dataInventory_.Inject(ccuData);

    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10089));

    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());

    FileReader reader(files.back());
    std::vector<std::string> content;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(content));
    ASSERT_FALSE(content.empty());
    const std::string jsonContent = content.back();

    EXPECT_NE(std::string::npos, jsonContent.find("\"Notify Instruction ID\":200"));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Die Id\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Data Size\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Bandwidth (MB/s)\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Reduce Op Type\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Input Data Type\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Output Data Type\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Mask\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Maximum Delay Channel\""));
    EXPECT_EQ(std::string::npos, jsonContent.find("\"Maximum Channel Delay\""));
}

TEST_F(CcuAssemblerUTest, ShouldOutputMetadataForEachDevice)
{
    auto fullData = BuildFullData();
    fullData[0].deviceId = 0;
    fullData[1].deviceId = 1;

    std::shared_ptr<std::vector<CCUMissionTimelineData>> ccuData;
    MAKE_SHARED_NO_OPERATION(ccuData, std::vector<CCUMissionTimelineData>, fullData);
    dataInventory_.Inject(ccuData);

    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10089));

    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    ASSERT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> content;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(content));
    ASSERT_FALSE(content.empty());
    const std::string jsonContent = content.back();

    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"process_name\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"process_labels\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"process_sort_index\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"thread_name\""));
    EXPECT_NE(std::string::npos, jsonContent.find("\"name\":\"thread_sort_index\""));
}

