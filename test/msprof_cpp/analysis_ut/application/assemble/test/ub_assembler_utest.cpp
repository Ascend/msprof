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
#include "analysis/csrc/application/timeline/ub_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
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
const std::string BASE_PATH = "./ub_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class UbAssemblerUTest : public testing::Test {
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
DataInventory UbAssemblerUTest::dataInventory_;

static std::vector<UbData> GenerateUbData()
{
    std::vector<UbData> res;
    UbData data;
    data.deviceId = 0;
    data.portId = 0;
    data.timestamp = 1717575960208020758;
    data.udmaRxBind = 100;
    data.udmaTxBind = 200;
    data.rxPortBandWidth = 1000;
    data.txPortBandWidth = 2000;
    res.push_back(data);

    data.portId = 1;
    data.udmaRxBind = 150;
    data.udmaTxBind = 250;
    res.push_back(data);
    return res;
}

TEST_F(UbAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    UbAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(UbAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    UbAssembler assembler;
    std::shared_ptr<std::vector<UbData>> ubDataS;
    auto ubData = GenerateUbData();
    MAKE_SHARED_NO_OPERATION(ubDataS, std::vector<UbData>, ubData);
    dataInventory_.Inject(ubDataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383961120,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Ub\"}},{\"name\":\"process_labels\",\"pid\":2383961120,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383961120,\"tid\":0,\"ph\":"
                            "\"M\",\"args\":{\"sort_index\":33}},{\"name\":\"UB Port000\",\"pid\":2383961120,\"tid\":0,"
                            "\"ts\":\"1717575960208020.758\",\"ph\":\"C\",\"args\":{\"bandwidth_tx(MB/s)\":200.0,"
                            "\"bandwidth_rx(MB/s)\":100.0}},{\"name\":\"UB Port001\",\"pid\":2383961120,\"tid\":0,\"ts\":"
                            "\"1717575960208020.758\",\"ph\":\"C\",\"args\":{\"bandwidth_tx(MB/s)\":250.0,"
                            "\"bandwidth_rx(MB/s)\":150.0}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(UbAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    UbAssembler assembler;
    std::shared_ptr<std::vector<UbData>> ubDataS;
    auto ubData = GenerateUbData();
    MAKE_SHARED_NO_OPERATION(ubDataS, std::vector<UbData>, ubData);
    dataInventory_.Inject(ubDataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
