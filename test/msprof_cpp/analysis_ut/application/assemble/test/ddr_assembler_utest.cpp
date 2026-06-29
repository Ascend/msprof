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
#include "analysis/csrc/application/timeline/ddr_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
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
const std::string BASE_PATH = "./ddr_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class DDRAssemblerUTest : public testing::Test {
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
DataInventory DDRAssemblerUTest::dataInventory_;

static std::vector<DDRData> GenerateDDRData()
{
    std::vector<DDRData> res;
    DDRData data;
    data.deviceId = 0; // device 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    data.fluxRead = 38.56; // 读带宽 38.56
    data.fluxWrite = 69.1654; // 写带宽 69.1654
    res.push_back(data);
    data.timestamp = 1724405892226699429; // 本地时间 1724405892226699429
    data.fluxRead = 72.5; // 读带宽 72.5
    data.fluxWrite = 64.35; // 写带宽 64.35
    res.push_back(data);
    data.timestamp = 1724405892226799429; // 本地时间 1724405892226799429
    data.fluxRead = 56.78; // 读带宽 56.78
    data.fluxWrite = 17.56; // 写带宽 17.56
    res.push_back(data);
    return res;
}

TEST_F(DDRAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    DDRAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(DDRAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    DDRAssembler assembler;
    std::shared_ptr<std::vector<DDRData>> dataS;
    auto data = GenerateDDRData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960640,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"DDR\"}},{\"name\":\"process_labels\",\"pid\":2383960640,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383960640"
                            ",\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":18}},{\"name\":\"DDR/Read\",\"pid\":"
                            "2383960640,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Read("
                            "MB/s)\":38.56}},{\"name\":\"DDR/Write\",\"pid\":2383960640,\"tid\":0,\"ts\":\""
                            "1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Write(MB/s)\":69.1654}},{\"name"
                            "\":\"DDR/Read\",\"pid\":2383960640,\"tid\":0,\"ts\":\"1724405892226699.429\",\"ph\""
                            ":\"C\",\"args\":{\"Read(MB/s)\":72.5}},{\"name\":\"DDR/Write\",\"pid\":2383960640,\"tid"
                            "\":0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\"Write(MB/s)\":64.35}},"
                            "{\"name\":\"DDR/Read\",\"pid\":2383960640,\"tid\":0,\"ts\":\"1724405892226799.429\","
                            "\"ph\":\"C\",\"args\":{\"Read(MB/s)\":56.78}},{\"name\":\"DDR/Write\",\"pid\":2383960640,"
                            "\"tid\":0,\"ts\":\"1724405892226799.429\",\"ph\":\"C\",\"args\":{\"Write(MB/s)\":"
                            "17.56}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DDRAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    DDRAssembler assembler;
    std::shared_ptr<std::vector<DDRData>> dataS;
    auto data = GenerateDDRData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
