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
#include "analysis/csrc/application/timeline/device_tx_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;

namespace {
    const int DEPTH = 0;
    const std::string BASE_PATH = "./device_tx_test";
    const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
    const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
}

class DeviceTxAssemblerUTest : public testing::Test {
protected:
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
protected:
    DataInventory dataInventory_;
};

static std::vector<MsprofTxHostData> GenerateTxAndExData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData exData;
    exData.eventType = 0; // eventType 0
    exData.pid = 19988776;  // pid 19988776
    exData.tid = 1024; // tid 1024
    exData.timestamp = 1717575960208126869; // start 1717575960208126869
    exData.end = 1717575960208226869; // start 1717575960208226869
    exData.connectionId = 4000000001;  // connId 4000000001
    exData.message = "ex";
    res.push_back(exData);
    return res;
}

static std::vector<MsprofTxDeviceData> GenerateDeviceTxData()
{
    std::vector<MsprofTxDeviceData> res;
    MsprofTxDeviceData data;
    data.deviceId = 0; // device id 0
    data.indexId = 2147483647; // indexId 2147483647
    data.streamId = 1; // streamId 1
    data.taskId = 13; // taskId 13
    data.connectionId = 7890; // connectionId 7890
    res.push_back(data);
    return res;
}

static std::vector<MsprofTxDeviceData> GenerateMultipleDeviceTxData()
{
    auto res = GenerateDeviceTxData();
    MsprofTxDeviceData data;
    data.deviceId = 0;
    data.indexId = 2147483647;
    data.streamId = 2;
    data.taskId = 14;
    data.connectionId = 7891;
    data.timestamp = 1000;
    data.duration = 500;
    res.push_back(data);
    return res;
}

static std::vector<MsprofTxHostData> GenerateMatchingTxAndExData()
{
    auto res = GenerateTxAndExData();
    res.front().connectionId = 7890;
    res.front().message = "matched_message";
    return res;
}

TEST_F(DeviceTxAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    DeviceTxAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(DeviceTxAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    DeviceTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxHostData>> txDataS;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    auto txData = GenerateTxAndExData();
    auto tx = GenerateDeviceTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    MAKE_SHARED_NO_OPERATION(txDataS, std::vector<MsprofTxHostData>, txData);
    dataInventory_.Inject(txDataS);
    dataInventory_.Inject(txS);
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(2ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"N/A\",\"pid\":448,\"tid\":1,\"ts\":\"18446744073709551.615\",\"dur\":0.0,"
                            "\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":13}},{\"name\":\"MsTx_7890\",\""
                            "pid\":448,\"tid\":1,\"ph\":\"f\",\"cat\":\"MsTx\",\"id\":\"7890\",\"ts\":\""
                            "18446744073709551.615\",\"bp\":\"e\"},{\"name\":\"process_name\",\"pid\":448,\"tid\":0"
                            ",\"ph\":\"M\",\"args\":{\"name\":\"Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid"
                            "\":448,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\""
                            "process_sort_index\",\"pid\":448,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":14}},{\""
                            "name\":\"thread_name\",\"pid\":448,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\""
                            "}},{\"name\":\"thread_sort_index\",\"pid\":448,\"tid\":1,\"ph\":\"M\",\"args\":{\""
                            "sort_index\":1}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DeviceTxAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    DeviceTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    auto tx = GenerateDeviceTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    dataInventory_.Inject(txS);
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(DeviceTxAssemblerUTest, ShouldUseHostMessageWhenConnectionIdMatchesHostTx)
{
    DeviceTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxHostData>> txDataS;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    auto txData = GenerateMatchingTxAndExData();
    auto tx = GenerateDeviceTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    MAKE_SHARED_NO_OPERATION(txDataS, std::vector<MsprofTxHostData>, txData);
    dataInventory_.Inject(txDataS);
    dataInventory_.Inject(txS);

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof_tx"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    ASSERT_FALSE(res.empty());
    std::string expectStr = "{\"name\":\"matched_message\",\"pid\":448,\"tid\":1,\"ts\":\"18446744073709551.615\","
                            "\"dur\":0.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":13}},{\"name\":"
                            "\"MsTx_7890\",\"pid\":448,\"tid\":1,\"ph\":\"f\",\"cat\":\"MsTx\",\"id\":\"7890\","
                            "\"ts\":\"18446744073709551.615\",\"bp\":\"e\"},{\"name\":\"process_name\",\"pid\":448,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Ascend Hardware\"}},{\"name\":"
                            "\"process_labels\",\"pid\":448,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
                            "{\"name\":\"process_sort_index\",\"pid\":448,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":14}},{\"name\":\"thread_name\",\"pid\":448,\"tid\":1,\"ph\":\"M\","
                            "\"args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\",\"pid\":448,\"tid\":1,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DeviceTxAssemblerUTest, ShouldGenerateThreadMetadataForEachStream)
{
    DeviceTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    auto tx = GenerateMultipleDeviceTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    dataInventory_.Inject(txS);

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof_tx"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    ASSERT_FALSE(res.empty());
    std::string expectStr = "{\"name\":\"N/A\",\"pid\":448,\"tid\":1,\"ts\":\"18446744073709551.615\",\"dur\":0.0,"
                            "\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":13}},{\"name\":\"MsTx_7890\","
                            "\"pid\":448,\"tid\":1,\"ph\":\"f\",\"cat\":\"MsTx\",\"id\":\"7890\",\"ts\":"
                            "\"18446744073709551.615\",\"bp\":\"e\"},{\"name\":\"N/A\",\"pid\":448,\"tid\":2,"
                            "\"ts\":\"1.000\",\"dur\":0.5,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":2,\"Task Id\":14}}"
                            ",{\"name\":\"MsTx_7891\",\"pid\":448,\"tid\":2,\"ph\":\"f\",\"cat\":\"MsTx\",\"id\":"
                            "\"7891\",\"ts\":\"1.000\",\"bp\":\"e\"},{\"name\":\"process_name\",\"pid\":448,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"name\":\"Ascend Hardware\"}},{\"name\":\"process_labels\","
                            "\"pid\":448,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":"
                            "\"process_sort_index\",\"pid\":448,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":14}},"
                            "{\"name\":\"thread_name\",\"pid\":448,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}}"
                            ",{\"name\":\"thread_sort_index\",\"pid\":448,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}}"
                            ",{\"name\":\"thread_name\",\"pid\":448,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Stream 2\"}}"
                            ",{\"name\":\"thread_sort_index\",\"pid\":448,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(DeviceTxAssemblerUTest, ShouldReturnFalseWhenDeviceTxVectorIsEmpty)
{
    DeviceTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    std::vector<MsprofTxDeviceData> tx;
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    dataInventory_.Inject(txS);

    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}


