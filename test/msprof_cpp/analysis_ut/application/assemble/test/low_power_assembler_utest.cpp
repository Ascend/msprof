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
#include "analysis/csrc/application/timeline/low_power_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./low_power_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class LowPowerAssemblerUTest : public testing::Test {
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
DataInventory LowPowerAssemblerUTest::dataInventory_;

static std::vector<LowPowerData> GenerateFreqData()
{
    std::vector<LowPowerData> res;
    LowPowerData data;
    data.deviceId = 0; // device 0
    data.dieId = 0;
    data.timestamp = 1719621074669030430; // timestamp 1719621074669030430
    data.aicFreq = 800; // freq 800 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.dieId = 0;
    data.timestamp = 1719621074688865380; // timestamp 1719621074688865380
    data.aicFreq = 1850; // freq 1850 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.dieId = 0;
    data.timestamp = 1719621074688867780; // timestamp 1719621074688867780
    data.aicFreq = 1800; // freq 1800 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.dieId = 0;
    data.timestamp = 1719621074688868780; // timestamp 1719621074688868780
    data.aicFreq = 1800; // freq 1800 MHz
    res.push_back(data);
    return res;
}

TEST_F(LowPowerAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    LowPowerAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(LowPowerAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    LowPowerAssembler assembler;
    std::shared_ptr<std::vector<LowPowerData>> dataS;
    auto data = GenerateFreqData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<LowPowerData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"AI Core Freq Die 0\",\"pid\":2383960544,\"tid\":0,\"ts\":\"1719621074669030.430\",\"ph\":\"C\",\""
                            "args\":{\"MHz\":800}},{\"name\":\"AI Core Freq Die 0\",\"pid\":2383960544,\"tid\":0,\"ts\":\""
                            "1719621074688865.380\",\"ph\":\"C\",\"args\":{\"MHz\":1850}},{\"name\":\"AI Core Freq Die 0\",\"pid\":2383960544,"
                            "\"tid\":0,\"ts\":\"1719621074688867.780\",\"ph\":\"C\",\"args\":{\"MHz\":1800}},{\"name\":\"AI Core Freq Die 0\","
                            "\"pid\":2383960544,\"tid\":0,\"ts\":\"1719621074688868.780\",\"ph\":\"C\",\"args\":{\"MHz\":1800}},{\""
                            "name\":\"process_name\",\"pid\":2383960544,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"AI Core Freq\"}},"
                            "{\"name\":\"process_labels\",\"pid\":2383960544,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
                            "{\"name\":\"process_sort_index\",\"pid\":2383960544,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":15}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(LowPowerAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    LowPowerAssembler assembler;
    std::shared_ptr<std::vector<LowPowerData>> dataS;
    auto data = GenerateFreqData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<LowPowerData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
