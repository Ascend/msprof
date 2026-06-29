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
#include "analysis/csrc/application/timeline/sys_io_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
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
const std::string BASE_PATH = "./sys_io_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class SysIOAssemblerUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }

protected:
    DataInventory dataInventory_;
};

static std::string ReadTraceContent()
{
    auto files = File::GetOriginData(RESULT_PATH, {MSPROF_JSON_FILE}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_FALSE(res.empty());
    return res.empty() ? "" : res.back();
}

static void InjectNicData(DataInventory &dataInventory, const std::vector<SysIOReceiveSendData> &traceData)
{
    NicReceiveSendData nicData;
    nicData.sysIOReceiveSendData = traceData;
    std::vector<NicReceiveSendData> nicDataVec {nicData};
    std::shared_ptr<std::vector<NicReceiveSendData>> dataS;
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NicReceiveSendData>, nicDataVec);
    dataInventory.Inject(dataS);
}

static void InjectRoceData(DataInventory &dataInventory, const std::vector<SysIOReceiveSendData> &traceData)
{
    RoceReceiveSendData roceData;
    roceData.sysIOReceiveSendData = traceData;
    std::vector<RoceReceiveSendData> roceDataVec {roceData};
    std::shared_ptr<std::vector<RoceReceiveSendData>> dataS;
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<RoceReceiveSendData>, roceDataVec);
    dataInventory.Inject(dataS);
}

static std::vector<NicReceiveSendData> GenerateNicReceiveSendData()
{
    std::vector<NicReceiveSendData> res;
    NicReceiveSendData nicReceiveSendData;
    SysIOReceiveSendData data;
    data.deviceId = 0; // device 0
    data.funcId = 0; // port 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    data.rxDroppedRate = 5.6; // 丢包率 5.6
    data.rxErrorRate = 1.2; // 错误率 1.2
    data.rxPacketRate = 60; // 收包速率 60
    data.rxBandwidthEfficiency = 0.5; // 带宽利用率 0.5
    data.txDroppedRate = 3.2; // 丢包率 3.2
    data.txErrorRate = 2.2; // 错误率 2.2
    data.txPacketRate = 50; // 发包速率 50
    data.txBandwidthEfficiency = 0.8; // 带宽利用率 0.8
    nicReceiveSendData.sysIOReceiveSendData.push_back({data});
    data.timestamp = 1724405892226699429; // 本地时间 1724405892226699429
    data.rxDroppedRate = 8.6; // 丢包率 8.6
    data.rxErrorRate = 3.2; // 错误率 3.2
    data.rxPacketRate = 40; // 收包速率 40
    data.rxBandwidthEfficiency = 0.9; // 带宽利用率 0.9
    data.txDroppedRate = 3.8; // 丢包率 3.8
    data.txErrorRate = 2.3; // 错误率 2.3
    data.txPacketRate = 60; // 发包速率 60
    data.txBandwidthEfficiency = 2.2; // 带宽利用率 2.2
    nicReceiveSendData.sysIOReceiveSendData.push_back(data);
    res.push_back(nicReceiveSendData);
    return res;
}

static std::vector<RoceReceiveSendData> GenerateRoceReceiveSendData()
{
    std::vector<RoceReceiveSendData> res;
    RoceReceiveSendData roceReceiveSendData;
    SysIOReceiveSendData data;
    data.deviceId = 0; // device 0
    data.funcId = 0; // port 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    data.rxDroppedRate = 5.6; // 丢包率 5.6
    data.rxErrorRate = 1.2; // 错误率 1.2
    data.rxPacketRate = 60; // 收包速率 60
    data.rxBandwidthEfficiency = 0.5; // 带宽利用率 0.5
    data.txDroppedRate = 3.2; // 丢包率 3.2
    data.txErrorRate = 2.2; // 错误率 2.2
    data.txPacketRate = 50; // 发包速率 50
    data.txBandwidthEfficiency = 0.8; // 带宽利用率 0.8
    roceReceiveSendData.sysIOReceiveSendData.push_back({data});
    data.timestamp = 1724405892226699429; // 本地时间 1724405892226699429
    data.rxDroppedRate = 8.6; // 丢包率 8.6
    data.rxErrorRate = 3.2; // 错误率 3.2
    data.rxPacketRate = 40; // 收包速率 40
    data.rxBandwidthEfficiency = 0.9; // 带宽利用率 0.9
    data.txDroppedRate = 3.8; // 丢包率 3.8
    data.txErrorRate = 2.3; // 错误率 2.3
    data.txPacketRate = 60; // 发包速率 60
    data.txBandwidthEfficiency = 2.2; // 带宽利用率 2.2
    roceReceiveSendData.sysIOReceiveSendData.push_back(data);
    res.push_back(roceReceiveSendData);
    return res;
}

TEST_F(SysIOAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    NicAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(SysIOAssemblerUTest, NicAssemblerShouldReturnTrueWhenDataAssembleSuccess)
{
    NicAssembler assembler;
    std::shared_ptr<std::vector<NicReceiveSendData>> dataS;
    auto data = GenerateNicReceiveSendData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NicReceiveSendData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960736,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"NIC\"}},{\"name\":\"process_labels\",\"pid\":2383960736,\"tid\":0,\"ph\":\"M\",\"args"
                            "\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383960736,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":21}},{\"name\":\"Port 0/Rx\",\"pid\":2383960736,"
                            "\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Rx Bandwidth "
                            "Efficiency\":0.5,\"Rx Packets\":60.0,\"Rx Error Rate\":1.2,\"Rx Dropped Rate\":5.6}},{"
                            "\"name\":\"Port 0/Tx\",\"pid\":2383960736,\"tid\":0,\"ts\":\"1724405892226599.429\","
                            "\"ph\":\"C\",\"args\":{\"Tx Bandwidth Efficiency\":0.8,\"Tx Packets\":50.0,\"Tx Error "
                            "Rate\":2.2,\"Tx Dropped Rate\":3.2}},{\"name\":\"Port 0/Rx\",\"pid\":2383960736,\"tid\":"
                            "0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\"Rx Bandwidth Efficiency\":"
                            "0.9,\"Rx Packets\":40.0,\"Rx Error Rate\":3.2,\"Rx Dropped Rate\":8.6}},{\"name\":\"Port "
                            "0/Tx\",\"pid\":2383960736,\"tid\":0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\""
                            "args\":{\"Tx Bandwidth Efficiency\":2.2,\"Tx Packets\":60.0,\"Tx Error Rate\":2.3,\"Tx "
                            "Dropped Rate\":3.8}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(SysIOAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    NicAssembler assembler;
    std::shared_ptr<std::vector<NicReceiveSendData>> dataS;
    auto data = GenerateNicReceiveSendData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NicReceiveSendData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(SysIOAssemblerUTest, RoCEAssemblerShouldReturnTrueWhenDataAssembleSuccess)
{
    RoCEAssembler assembler;
    std::shared_ptr<std::vector<RoceReceiveSendData>> dataS;
    auto data = GenerateRoceReceiveSendData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<RoceReceiveSendData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {MSPROF_JSON_FILE}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960768,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"RoCE\"}},{\"name\":\"process_labels\",\"pid\":2383960768,\"tid\":0,\"ph\":\"M\",\"args"
                            "\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383960768,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":22}},{\"name\":\"Port 0/Rx\",\"pid\":2383960768,"
                            "\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Rx Bandwidth "
                            "Efficiency\":0.5,\"Rx Packets\":60.0,\"Rx Error Rate\":1.2,\"Rx Dropped Rate\":5.6}},"
                            "{\"name\":\"Port 0/Tx\",\"pid\":2383960768,\"tid\":0,\"ts\":\"1724405892226599.429\""
                            ",\"ph\":\"C\",\"args\":{\"Tx Bandwidth Efficiency\":0.8,\"Tx Packets\":50.0,\"Tx Error "
                            "Rate\":2.2,\"Tx Dropped Rate\":3.2}},{\"name\":\"Port 0/Rx\",\"pid\":2383960768,\"tid\""
                            ":0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\"Rx Bandwidth Efficiency\""
                            ":0.9,\"Rx Packets\":40.0,\"Rx Error Rate\":3.2,\"Rx Dropped Rate\":8.6}},{\"name\":\"Port "
                            "0/Tx\",\"pid\":2383960768,\"tid\":0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\""
                            "args\":{\"Tx Bandwidth Efficiency\":2.2,\"Tx Packets\":60.0,\"Tx Error Rate\":2.3,\"Tx "
                            "Dropped Rate\":3.8}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(SysIOAssemblerUTest, RoCEAssemblerShouldReturnTrueWhenDataNotExists)
{
    RoCEAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(SysIOAssemblerUTest, NicAssemblerShouldGenerateMetadataOnlyWhenInnerTraceVectorEmpty)
{
    NicAssembler assembler;
    InjectNicData(dataInventory_, {});
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {MSPROF_JSON_FILE}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    ASSERT_FALSE(res.empty());
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960736,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"NIC\"}},{\"name\":\"process_labels\",\"pid\":2383960736,\"tid\":0,\"ph\":\"M\",\"args\""
                            ":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\",\"pid\":2383960736,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":21}},";
    EXPECT_EQ(expectStr, res.back());
}
