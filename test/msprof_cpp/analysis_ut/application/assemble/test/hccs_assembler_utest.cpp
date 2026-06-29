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
#include "analysis/csrc/application/timeline/hccs_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
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
const std::string BASE_PATH = "./hccs_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class HCCSAssemblerUTest : public testing::Test {
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
DataInventory HCCSAssemblerUTest::dataInventory_;

static std::vector<HccsData> GenerateHccsData()
{
    std::vector<HccsData> res;
    HccsData data;
    data.deviceId = 0; // device 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    data.rxThroughput = 240125.966; // 接收带宽 240125.966
    data.txThroughput = 3.197; // 发送带宽 3.197
    res.push_back(data);
    return res;
}

TEST_F(HCCSAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    HCCSAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(HCCSAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    HCCSAssembler assembler;
    std::shared_ptr<std::vector<HccsData>> dataS;
    auto data = GenerateHccsData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960704,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"HCCS\"}},{\"name\":\"process_labels\",\"pid\":2383960704,\"tid\":0,\"ph\":\"M\",\"args"
                            "\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383960704,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":20}},{\"name\":\"Rx\",\"pid\":2383960704,\"tid\":0,"
                            "\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Rx(MB/s)\":240125.966}},"
                            "{\"name\":\"Tx\",\"pid\":2383960704,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":"
                            "\"C\",\"args\":{\"Tx(MB/s)\":3.197}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HCCSAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    HCCSAssembler assembler;
    std::shared_ptr<std::vector<HccsData>> dataS;
    auto data = GenerateHccsData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
