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

#include "analysis/csrc/application/timeline/overlap_analysis_assembler.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/application/database/db_constant.h"

using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Application;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./overlap_analysis_assembler";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});

class OverlapAnalysisAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }

    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }

    void TearDown() override
    {
        if (File::Exist(RESULT_PATH)) {
            EXPECT_TRUE(File::RemoveDir(RESULT_PATH, DEPTH));
        }
        GlobalMockObject::verify();
    }
};

OverlapAnalysisData MakeOverlapData(uint16_t deviceId, uint64_t timestamp, uint64_t duration, OverlapAnalysisType type)
{
    OverlapAnalysisData data;
    data.deviceId = deviceId;
    data.timestamp = timestamp;
    data.duration = duration;
    data.type = type;
    return data;
}

std::vector<OverlapAnalysisData> GenerateOverlapData()
{
    std::vector<OverlapAnalysisData> data;
    data.push_back(MakeOverlapData(0, 1, 2, OverlapAnalysisType::COMPUTE));
    data.push_back(MakeOverlapData(0, 4, 3, OverlapAnalysisType::COMMUNICATION));
    data.push_back(MakeOverlapData(0, 7, 1, OverlapAnalysisType::COMM_NOT_OVERLAP_COMP));
    data.push_back(MakeOverlapData(0, 8, 2, OverlapAnalysisType::FREE));
    data.push_back(MakeOverlapData(1, 12, 4, OverlapAnalysisType::COMMUNICATION));
    return data;
}

std::string ReadOnlyJsonFile()
{
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    if (files.empty()) {
        return "";
    }
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    return res.empty() ? "" : res.back();
}
}  // namespace

TEST_F(OverlapAnalysisAssemblerUTest, RunShouldDumpOverlapAnalysisDataFromDataInventory)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<OverlapAnalysisData>> overlapData;
    MAKE_SHARED0_NO_OPERATION(overlapData, std::vector<OverlapAnalysisData>, GenerateOverlapData());
    ASSERT_TRUE(dataInventory.Inject<std::vector<OverlapAnalysisData>>(overlapData));

    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // formatted pid: device0 10330080

    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));

    const auto json = ReadOnlyJsonFile();
    const std::vector<std::string> expectedFragments = {
        "\"name\":\"Overlap Analysis\"",
        "\"labels\":\"NPU 0\"",
        "\"labels\":\"NPU 1\"",
        "\"name\":\"Computing\",\"pid\":10330080,\"tid\":0",
        "\"name\":\"Communication\",\"pid\":10330080,\"tid\":1",
        "\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":2",
        "\"name\":\"Free\",\"pid\":10330080,\"tid\":3",
        "\"name\":\"Communication\",\"pid\":10330081,\"tid\":1",
        "\"ts\":\"0.001\"",
        "\"dur\":0.002"};
    for (size_t i = 0; i < expectedFragments.size(); ++i) {
        EXPECT_NE(std::string::npos, json.find(expectedFragments[i])) << expectedFragments[i];
    }
}

TEST_F(OverlapAnalysisAssemblerUTest, RunShouldNotDumpFileWhenOverlapDataIsEmpty)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<OverlapAnalysisData>> overlapData;
    MAKE_SHARED0_NO_OPERATION(overlapData, std::vector<OverlapAnalysisData>);
    ASSERT_TRUE(dataInventory.Inject<std::vector<OverlapAnalysisData>>(overlapData));

    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_TRUE(files.empty());
}
