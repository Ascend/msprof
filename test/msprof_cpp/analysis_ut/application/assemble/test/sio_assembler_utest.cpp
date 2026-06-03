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
#include "analysis/csrc/application/timeline/sio_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"
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
const std::string BASE_PATH = "./sio_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class SioAssemblerUTest : public testing::Test {
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
DataInventory SioAssemblerUTest::dataInventory_;

static std::vector<SioData> GenerateSioData()
{
    std::vector<SioData> res;
    SioData data1;
    data1.deviceId = 0; // 0
    data1.dieId = 0; // 0
    data1.timestamp = 1724405892226599429; // 1724405892226599429
    data1.reqRxBandwidth = 20.3; // 20.3
    data1.rspRxBandwidth = 19.6; // 19.6
    data1.snpRxBandwidth = 18.6; // 18.6
    data1.datRxBandwidth = 25.9; // 25.9
    data1.reqTxBandwidth = 20.4; // 20.4
    data1.rspTxBandwidth = 19.1; // 19.1
    data1.snpTxBandwidth = 18.7; // 18.7
    data1.datTxBandwidth = 25.2; // 25.2
    res.push_back(data1);
    data1.dieId = 1; // 1
    data1.timestamp = 1724405892226599429; // 1724405892226599429
    data1.reqRxBandwidth = 15.3; // 15.3
    data1.rspRxBandwidth = 29.6; // 29.6
    data1.snpRxBandwidth = 28.6; // 28.6
    data1.datRxBandwidth = 35.9; // 35.9
    data1.reqTxBandwidth = 30.4; // 30.4
    data1.rspTxBandwidth = 28.1; // 28.1
    data1.snpTxBandwidth = 34.7; // 34.7
    data1.datTxBandwidth = 25.2; // 25.2
    res.push_back(data1);
    data1.dieId = 0; // 0
    data1.timestamp = 1724405892226699529; // 1724405892226699529
    data1.reqRxBandwidth = 21.3; // 21.3
    data1.rspRxBandwidth = 18.6; // 18.6
    data1.snpRxBandwidth = 16.6; // 16.6
    data1.datRxBandwidth = 24.9; // 24.9
    data1.reqTxBandwidth = 23.4; // 23.4
    data1.rspTxBandwidth = 20.1; // 20.1
    data1.snpTxBandwidth = 15.7; // 15.7
    data1.datTxBandwidth = 23.2; // 23.2
    res.push_back(data1);
    data1.dieId = 1; // 1
    data1.timestamp = 1724405892226699529; // 1724405892226699529
    data1.reqRxBandwidth = 24.3; // 24.3
    data1.rspRxBandwidth = 12.6; // 12.6
    data1.snpRxBandwidth = 17.6; // 17.6
    data1.datRxBandwidth = 23.9; // 23.9
    data1.reqTxBandwidth = 24.4; // 24.4
    data1.rspTxBandwidth = 21.1; // 21.1
    data1.snpTxBandwidth = 19.7; // 19.7
    data1.datTxBandwidth = 22.2; // 22.2
    res.push_back(data1);
    return res;
}

TEST_F(SioAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    SioAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(SioAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    SioAssembler assembler;
    std::shared_ptr<std::vector<SioData>> dataS;
    auto data = GenerateSioData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<SioData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(static_cast<unsigned short>(0)));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960960,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"name\":\"SIO\"}},{\"name\":\"process_labels\",\"pid\":2383960960,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\","
                            "\"pid\":2383960960,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":28}},{\"name\":"
                            "\"snp_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226699.529\",\"ph\":\"C\","
                            "\"args\":{\"\":19.7}},{\"name\":\"rsp_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226699.529\",\"ph\":\"C\",\"args\":{\"\":21.1}},{\"name\":\"snp_rx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":"
                            "{\"\":28.6}},{\"name\":\"req_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"\":30.4}},{\"name\":\"req_tx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226699.529\",\"ph\":\"C\","
                            "\"args\":{\"\":24.4}},{\"name\":\"req_rx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"\":15.3}},{\"name\":\"dat_rx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":"
                            "{\"\":35.9}},{\"name\":\"rsp_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"\":28.1}},{\"name\":\"snp_rx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226699.529\",\"ph\":\"C\",\"args\":"
                            "{\"\":17.6}},{\"name\":\"rsp_rx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"\":29.6}},{\"name\":\"dat_rx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226699.529\",\"ph\":\"C\",\"args\":"
                            "{\"\":23.9}},{\"name\":\"dat_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226699.529\",\"ph\":\"C\",\"args\":{\"\":22.2}},{\"name\":\"snp_tx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":"
                            "{\"\":34.7}},{\"name\":\"dat_tx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"\":25.2}},{\"name\":\"req_rx\","
                            "\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226699.529\",\"ph\":\"C\",\"args\":"
                            "{\"\":24.3}},{\"name\":\"rsp_rx\",\"pid\":2383960960,\"tid\":0,\"ts\":"
                            "\"1724405892226699.529\",\"ph\":\"C\",\"args\":{\"\":12.6}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(SioAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    SioAssembler assembler;
    std::shared_ptr<std::vector<SioData>> dataS;
    auto data = GenerateSioData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<SioData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
