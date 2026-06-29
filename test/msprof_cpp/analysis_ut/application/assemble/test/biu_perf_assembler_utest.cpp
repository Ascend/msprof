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
#include "analysis/csrc/application/timeline/biu_perf_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/biu_perf_data.h"
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
const std::string BASE_PATH = "./biu_perf_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class BiuPerfAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
protected:
    static DataInventory dataInventory_;
};
DataInventory BiuPerfAssemblerUTest::dataInventory_;

static std::vector<BiuPerfData> GenerateBiuPerfData()
{
    std::vector<BiuPerfData> res;
    BiuPerfData data;
    data.groupId = 0;
    data.coreType = "AI_CORE";
    data.blockId = 1;
    data.instruction = "instruction_a";
    data.timestamp = 1717575960208020758;
    data.duration = 450780;
    res.push_back(data);

    data.coreType = "VECTOR";
    data.blockId = 2;
    data.instruction = "instruction_b";
    data.duration = 250260;
    res.push_back(data);
    return res;
}

TEST_F(BiuPerfAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    BiuPerfAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(BiuPerfAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    BiuPerfAssembler assembler;
    std::shared_ptr<std::vector<BiuPerfData>> biuPerfS;
    auto biuPerfData = GenerateBiuPerfData();
    MAKE_SHARED_NO_OPERATION(biuPerfS, std::vector<BiuPerfData>, biuPerfData);
    dataInventory_.Inject(biuPerfS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383961088,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Biu Perf\"}},{\"name\":\"process_labels\",\"pid\":2383961088,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383961088,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":32}},{\"name\":\"thread_name\",\"pid"
                            "\":2383961088,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Group0-AI_CORE\"}},{\"name\":"
                            "\"instruction_a\",\"pid\":2383961088,\"tid\":0,\"ts\":\"1717575960208020.758\",\"dur"
                            "\":450.0,\"ph\":\"X\",\"args\":{\"Core Type\":\"AI_CORE\",\"Block Id\":1}},{\"name\":"
                            "\"thread_name\",\"pid\":2383961088,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Group0-VECTOR\"}},{\"name\":\"instruction_b\",\"pid\":2383961088,\"tid\":1,\"ts\":"
                            "\"1717575960208020.758\",\"dur\":250.0,\"ph\":\"X\",\"args\":{\"Core Type\":\"VECTOR\","
                            "\"Block Id\":2}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(BiuPerfAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    BiuPerfAssembler assembler;
    std::shared_ptr<std::vector<BiuPerfData>> biuPerfS;
    auto biuPerfData = GenerateBiuPerfData();
    MAKE_SHARED_NO_OPERATION(biuPerfS, std::vector<BiuPerfData>, biuPerfData);
    dataInventory_.Inject(biuPerfS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(BiuPerfAssemblerUTest, ShouldReturnTrueWhenPidMapIsEmpty)
{
    BiuPerfAssembler assembler;
    std::shared_ptr<std::vector<BiuPerfData>> biuPerfS;
    auto biuPerfData = GenerateBiuPerfData();
    MAKE_SHARED_NO_OPERATION(biuPerfS, std::vector<BiuPerfData>, biuPerfData);
    std::vector<std::string> deviceList = {};
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    dataInventory_.Inject(biuPerfS);
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
}
