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

#include "analysis/csrc/application/timeline/fusion_task_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_task_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ft_assembler_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});

std::vector<FusionTaskTimelineData> GenerateFusionTaskData()
{
    std::vector<FusionTaskTimelineData> res;
    FusionTaskTimelineData aicoreData;
    aicoreData.deviceId = 0;
    aicoreData.streamId = 0;
    aicoreData.taskId = 100;
    aicoreData.accId = 3;
    aicoreData.taskType = "AI_CORE";
    aicoreData.fusionTaskType = "AICORE";
    aicoreData.startTime = 1000000000;  // ns
    aicoreData.endTime = 2000000000;
    aicoreData.duration = 1000000.0;    // ns
    aicoreData.missionId = 0;
    aicoreData.isCcu = false;
    aicoreData.hasCcuDie = false;
    res.push_back(aicoreData);

    FusionTaskTimelineData ccuData;
    ccuData.deviceId = 0;
    ccuData.streamId = 0;
    ccuData.taskId = 200;
    ccuData.accId = 7;
    ccuData.taskType = "CCU_SQE";
    ccuData.fusionTaskType = "CCU";
    ccuData.startTime = 3000000000;
    ccuData.endTime = 4000000000;
    ccuData.duration = 1000000.0;
    ccuData.missionId = 3;
    ccuData.ccuDieId = 0;
    ccuData.isCcu = true;
    ccuData.hasCcuDie = true;
    res.push_back(ccuData);
    return res;
}
}

class FusionTaskAssemblerUTest : public testing::Test {
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
        GlobalMockObject::verify();
    }

protected:
    DataInventory dataInventory_;
};

TEST_F(FusionTaskAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    FusionTaskAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(FusionTaskAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    FusionTaskAssembler assembler;
    std::shared_ptr<std::vector<FusionTaskTimelineData>> dataS;
    auto data = GenerateFusionTaskData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<FusionTaskTimelineData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10008));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
}

TEST_F(FusionTaskAssemblerUTest, ShouldGenerateCcuArgsWithDieId)
{
    FusionTaskAssembler assembler;
    std::shared_ptr<std::vector<FusionTaskTimelineData>> dataS;
    std::vector<FusionTaskTimelineData> ccuOnly;

    FusionTaskTimelineData ccu;
    ccu.deviceId = 0;
    ccu.streamId = 0;
    ccu.taskId = 300;
    ccu.accId = 5;
    ccu.taskType = "CCU_SQE";
    ccu.fusionTaskType = "CCU";
    ccu.startTime = 1000000000;
    ccu.endTime = 2000000000;
    ccu.duration = 1000000.0;
    ccu.missionId = 7;
    ccu.ccuDieId = 1;
    ccu.isCcu = true;
    ccu.hasCcuDie = true;
    ccuOnly.push_back(ccu);

    MAKE_SHARED_NO_OPERATION(dataS, std::vector<FusionTaskTimelineData>, ccuOnly);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10008));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
}
