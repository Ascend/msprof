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
#include "analysis/csrc/application/timeline/ccu_assembler.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ccu_assembler_info";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string OUTPUT_PATH = File::PathJoin({PROF_PATH, "mindstudio_profiler_output"});
const uint32_t TEST_PID = 1000;

std::vector<CCUMissionTimelineData> MakeV61Data()
{
    std::vector<CCUMissionTimelineData> data;
    // LoopGroup event
    CCUMissionTimelineData loop;
    loop.timeType = CCU_TIME_TYPE_LOOP_GROUP;
    loop.deviceId = 0;
    loop.taskId = 1;
    loop.instructionId = 100;
    loop.timestamp = 1000000000;
    loop.duration = 500000.0;
    loop.streamId = 42;
    loop.hasDieId = true;
    loop.dieId = 3;
    loop.hasDataSize = true;
    loop.dataSize = 8192;
    loop.hasBandwidth = true;
    loop.bandwidth = 16.384;
    data.push_back(loop);
    // Wait event
    CCUMissionTimelineData wait;
    wait.timeType = CCU_TIME_TYPE_WAIT;
    wait.deviceId = 0;
    wait.taskId = 2;
    wait.instructionId = 200;
    wait.notifyRankId = 15;
    wait.timestamp = 2000000000;
    wait.duration = 300000.0;
    wait.streamId = 43;
    wait.hasMask = true;
    wait.mask = 0xFF;
    wait.hasDieId = true;
    wait.dieId = 1;
    wait.hasDelayChannel = true;
    wait.maxDelayChannel = 10;
    wait.maxChannelDelay = 500;
    data.push_back(wait);
    return data;
}
std::vector<CCUV62TimelineData> MakeV62Data()
{
    std::vector<CCUV62TimelineData> data;
    // LoopGroup
    CCUV62TimelineData loop;
    loop.timeType = CCU_TIME_TYPE_LOOP_GROUP;
    loop.deviceId = 0;
    loop.taskId = 10;
    loop.instructionId = 300;
    loop.timestamp = 3000000000;
    loop.duration = 700000.0;
    loop.hasDataSize = true;
    loop.dataSize = 4096;
    data.push_back(loop);
    // SetCKB mission: with CKB Info + UDF
    CCUV62TimelineData mission;
    mission.timeType = CCU_TIME_TYPE_SETCKB;
    mission.deviceId = 0;
    mission.taskId = 20;
    mission.instructionId = 400;
    mission.timestamp = 4000000000;
    mission.duration = 200000.0;
    mission.setckbCkbinfo = 0xABCD;
    mission.hasUdf = true;
    mission.udfProfilingData = "hash_abc";
    mission.lpFlg = 0;
    data.push_back(mission);
    // SetCKB loop
    CCUV62TimelineData loopSetckb;
    loopSetckb.timeType = CCU_TIME_TYPE_SETCKB;
    loopSetckb.deviceId = 0;
    loopSetckb.taskId = 30;
    loopSetckb.instructionId = 500;
    loopSetckb.timestamp = 5000000000;
    loopSetckb.duration = 900000.0;
    loopSetckb.lpFlg = 1;
    loopSetckb.earliestLoopId = 2;
    loopSetckb.latestLoopId = 5;
    loopSetckb.longestLoopId = 3;
    loopSetckb.longestDuration = 450000.0;
    loopSetckb.hasMask = true;
    loopSetckb.mask = 128;
    loopSetckb.hasDelayChannel = true;
    loopSetckb.maxDelayChannel = 21;
    loopSetckb.maxChannelDelay = 35;
    data.push_back(loopSetckb);
    return data;
}
} // anonymous namespace

class CCUAssemblerUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(OUTPUT_PATH));
        MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(TEST_PID));
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
        if (File::Check(BASE_PATH)) {
            EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        }
    }

    void InjectV61Data()
    {
        auto v61 = MakeV61Data();
        std::shared_ptr<std::vector<CCUMissionTimelineData>> sp;
        MAKE_SHARED_NO_OPERATION(sp, std::vector<CCUMissionTimelineData>, v61);
        dataInventory_.Inject(sp);
    }

    void InjectV62Data()
    {
        auto v62 = MakeV62Data();
        std::shared_ptr<std::vector<CCUV62TimelineData>> sp;
        MAKE_SHARED_NO_OPERATION(sp, std::vector<CCUV62TimelineData>, v62);
        dataInventory_.Inject(sp);
    }

    std::string ReadOutput()
    {
        auto files = File::GetOriginData(OUTPUT_PATH, {MSPROF_JSON_FILE}, {});
        if (files.empty()) {
            return "";
        }
        FileReader reader(files.back());
        std::vector<std::string> res;
        if (reader.ReadText(res) != 0 || res.empty()) {
            return "";
        }
        // Return the last (and usually only) line
        return res.back();
    }

protected:
    DataInventory dataInventory_;
};

TEST_F(CCUAssemblerUTest, ShouldAssembleV61LoopGroupAndWaitEvents)
{
    InjectV61Data();
    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto output = ReadOutput();
    EXPECT_FALSE(output.empty());
    // v6_1: Physic Stream Id present
    EXPECT_TRUE(output.find("\"Physic Stream Id\":42") != std::string::npos);
    EXPECT_TRUE(output.find("\"Physic Stream Id\":43") != std::string::npos);
    // LoopGroup event
    EXPECT_TRUE(output.find("\"name\":\"LoopGroup\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"Task Id\":1") != std::string::npos);
    EXPECT_TRUE(output.find("\"Instruction ID\":100") != std::string::npos);
    EXPECT_TRUE(output.find("\"Die Id\":3") != std::string::npos);
    EXPECT_TRUE(output.find("\"Data Size\":8192") != std::string::npos);
    EXPECT_TRUE(output.find("\"Bandwidth (MB/s)\":16.384") != std::string::npos);
    // Wait event
    EXPECT_TRUE(output.find("\"name\":\"Wait\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"Task Id\":2") != std::string::npos);
    EXPECT_TRUE(output.find("\"Notify Instruction ID\":200") != std::string::npos);
    EXPECT_TRUE(output.find("\"Notify Rank ID\":15") != std::string::npos);
    EXPECT_TRUE(output.find("\"Mask\":255") != std::string::npos);
    EXPECT_TRUE(output.find("\"Maximum Delay Channel\":10") != std::string::npos);
    // Metadata
    EXPECT_TRUE(output.find("\"name\":\"process_name\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"name\":\"thread_name\"") != std::string::npos);
}

TEST_F(CCUAssemblerUTest, ShouldAssembleV62LoopGroupAndSetckbEvents)
{
    InjectV62Data();
    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto output = ReadOutput();
    EXPECT_FALSE(output.empty());
    // v6_2: no Physic Stream Id
    EXPECT_TRUE(output.find("\"Physic Stream Id\"") == std::string::npos);
    // LoopGroup v6_2
    EXPECT_TRUE(output.find("\"name\":\"LoopGroup\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"Task Id\":10") != std::string::npos);
    EXPECT_TRUE(output.find("\"Instruction ID\":300") != std::string::npos);
    EXPECT_TRUE(output.find("\"Data Size\":4096") != std::string::npos);
    // SetCKB mission
    EXPECT_TRUE(output.find("\"name\":\"SetCKB Mission\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"Task Id\":20") != std::string::npos);
    EXPECT_TRUE(output.find("\"Instruction ID\":400") != std::string::npos);
    EXPECT_TRUE(output.find("\"Notify Instruction ID\":400") != std::string::npos);
    EXPECT_TRUE(output.find("\"Notify Rank ID\"") == std::string::npos);
    const auto ckbInfo = output.find("\"CKB Info\":\"1010101111001101\""); // 0xABCD
    ASSERT_NE(ckbInfo, std::string::npos);
    EXPECT_EQ(output.find("\"CKB Info\""), ckbInfo);
    EXPECT_EQ(output.find("\"CKB Info\"", ckbInfo + 1), std::string::npos);
    EXPECT_TRUE(output.find("\"UDF Profiling Data\":\"hash_abc\"") != std::string::npos);
    // SetCKB loop
    EXPECT_TRUE(output.find("\"name\":\"SetCKB Loop\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"Task Id\":30") != std::string::npos);
    EXPECT_TRUE(output.find("\"Instruction ID\":500") != std::string::npos);
    EXPECT_TRUE(output.find("\"Earliest Loop Id\":2") != std::string::npos);
    EXPECT_TRUE(output.find("\"Latest Loop Id\":5") != std::string::npos);
    EXPECT_TRUE(output.find("\"Longest Duration Loop Id\":3") != std::string::npos);
    EXPECT_TRUE(output.find("\"Mask\":128") != std::string::npos);
    EXPECT_TRUE(output.find("\"Maximum Delay Channel\":21") != std::string::npos);
    EXPECT_TRUE(output.find("\"Maximum Channel Delay\":35") != std::string::npos);
}

TEST_F(CCUAssemblerUTest, ShouldAssembleBothV61AndV62Events)
{
    InjectV61Data();
    InjectV62Data();
    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto output = ReadOutput();
    EXPECT_FALSE(output.empty());
    // Both v6_1 and v6_2 features present
    EXPECT_TRUE(output.find("\"Physic Stream Id\":42") != std::string::npos);
    EXPECT_TRUE(output.find("\"CKB Info\":\"1010101111001101\"") != std::string::npos);
    // All event types present
    EXPECT_TRUE(output.find("\"name\":\"LoopGroup\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"name\":\"Wait\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"name\":\"SetCKB Mission\"") != std::string::npos);
    EXPECT_TRUE(output.find("\"name\":\"SetCKB Loop\"") != std::string::npos);
}

TEST_F(CCUAssemblerUTest, ShouldReturnDataNotExistWhenNoData)
{
    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto output = ReadOutput();
    EXPECT_TRUE(output.empty());
}

TEST_F(CCUAssemblerUTest, ShouldUseZeroPidWhenDevicePidIsUnavailable)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(uint32_t{0}));
    InjectV61Data();

    CCUAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    EXPECT_FALSE(ReadOutput().empty());
}
