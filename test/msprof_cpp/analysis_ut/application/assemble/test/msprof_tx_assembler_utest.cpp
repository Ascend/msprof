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
#include "analysis/csrc/application/timeline/msprof_tx_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./tx_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class MsprofTxAssemblerUTest : public testing::Test {
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

static void InjectTxData(DataInventory &dataInventory, const std::vector<MsprofTxHostData> &txData)
{
    std::shared_ptr<std::vector<MsprofTxHostData>> txDataS;
    MAKE_SHARED_NO_OPERATION(txDataS, std::vector<MsprofTxHostData>, txData);
    dataInventory.Inject(txDataS);
}

static std::string ReadTraceContent(const std::string &prefix)
{
    auto files = File::GetOriginData(RESULT_PATH, {prefix}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_FALSE(res.empty());
    return res.empty() ? "" : res.back();
}

static std::vector<MsprofTxHostData> GenerateTxAndExData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData txData;
    txData.eventType = 0; // eventType 0
    txData.pid = 19988776;  // pid 19988776
    txData.tid = 1024; // tid 1024
    txData.category = 23438457; // category 23438457
    txData.payloadType = 0; // payloadType 0
    txData.messageType = 1; // messageType 1
    txData.payloadValue = 283728437; // payloadValue 283728437
    txData.timestamp = 1717575960208020758; // start 1717575960208020758
    txData.end = 1717575960208026869; // start 1717575960208026869
    txData.message = "tx";
    res.push_back(txData);
    MsprofTxHostData exData;
    exData.eventType = 0; // eventType 0
    exData.pid = 19988776;  // pid 19988776
    exData.tid = 1024; // tid 1024
    exData.timestamp = 1717575960208126869; // start 1717575960208126869
    exData.end = 1717575960208226869; // start 1717575960208226869
    exData.connectionId = 4000000001;  // connId 4000000001
    exData.domain = "domain";
    exData.message = "ex";
    res.push_back(exData);
    return res;
}

static std::vector<MsprofTxHostData> GenerateDefaultConnectionTxData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData txData;
    txData.eventType = 0;
    txData.pid = 19988776;
    txData.tid = 1024;
    txData.category = 23438457;
    txData.payloadType = 3;
    txData.messageType = 9;
    txData.payloadValue = 283728437;
    txData.timestamp = 1717575960208020758;
    txData.end = 1717575960208026869;
    txData.message = "tx_default";
    res.push_back(txData);
    return res;
}

static std::vector<MsprofTxHostData> GenerateMultiThreadTxData()
{
    auto res = GenerateDefaultConnectionTxData();
    MsprofTxHostData otherTid = res.front();
    otherTid.tid = 2048;
    otherTid.message = "thread_two";
    otherTid.timestamp += 1000;
    otherTid.end += 1000;
    res.push_back(otherTid);
    MsprofTxHostData duplicatedTid = res.front();
    duplicatedTid.message = "thread_one_again";
    duplicatedTid.timestamp += 2000;
    duplicatedTid.end += 2000;
    res.push_back(duplicatedTid);
    return res;
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    MsprofTxAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    MsprofTxAssembler assembler;
    auto txData = GenerateTxAndExData();
    InjectTxData(dataInventory_, txData);
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(2ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"tx\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208020.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":0,"
                            "\"Payload_value\":283728437,\"Message_type\":1,\"event_type\":\"marker\"}},"
                            "{\"name\":\"ex\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208126.869\",\"dur\":100.0,"
                            "\"ph\":\"X\",\"args\":{\"event_type\":\"marker\",\"domain\":\"domain\"}},"
                            "{\"name\":\"MsTx_4000000001\",\"pid\":223,\"tid\":1024,\"ph\":\"s\",\"cat\":\"MsTx\","
                            "\"id\":\"4000000001\",\"ts\":\"1717575960208126.869\"},{\"name\":\"process_name\","
                            "\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"\"}},{\"name\":\"process_labels\","
                            "\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"CPU\"}},"
                            "{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,\"tid\":1024,\"ph\":\"M\","
                            "\"args\":{\"name\":\"Thread 1024\"}},{\"name\":\"thread_sort_index\",\"pid\":223,"
                            "\"tid\":1024,\"ph\":\"M\",\"args\":{\"sort_index\":1024}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    MsprofTxAssembler assembler;
    auto txData = GenerateTxAndExData();
    InjectTxData(dataInventory_, txData);
    std::string name = "main";
    MOCKER_CPP(&Context::GetPidNameFromInfoJson).stubs().will(returnValue(name));
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(MsprofTxAssemblerUTest, ShouldGenerateFlowStartWhenConnectionIdIsNotDefault)
{
    MsprofTxAssembler assembler;
    std::vector<MsprofTxHostData> txData;
    MsprofTxHostData exData;
    exData.eventType = 0;
    exData.tid = 2048;
    exData.timestamp = 1717575960208126869;
    exData.end = 1717575960208226869;
    exData.connectionId = 5000000001;
    exData.domain = "trace_domain";
    exData.message = "tx_ex_only";
    txData.push_back(exData);
    InjectTxData(dataInventory_, txData);

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent(MSPROF_TX_FILE);
    std::string expectStr = "{\"name\":\"tx_ex_only\",\"pid\":223,\"tid\":2048,\"ts\":\"1717575960208126.869\","
                            "\"dur\":100.0,\"ph\":\"X\",\"args\":{\"event_type\":\"marker\",\"domain\":"
                            "\"trace_domain\"}},{\"name\":\"MsTx_5000000001\",\"pid\":223,\"tid\":2048,\"ph\":\"s\","
                            "\"cat\":\"MsTx\",\"id\":\"5000000001\",\"ts\":\"1717575960208126.869\"},{\"name\":"
                            "\"process_name\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"\"}},"
                            "{\"name\":\"process_labels\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,"
                            "\"tid\":2048,\"ph\":\"M\",\"args\":{\"name\":\"Thread 2048\"}},{\"name\":"
                            "\"thread_sort_index\",\"pid\":223,\"tid\":2048,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":2048}},";
    EXPECT_EQ(expectStr, content);
}

TEST_F(MsprofTxAssemblerUTest, ShouldUseContextPidNameAsProcessName)
{
    MsprofTxAssembler assembler;
    auto txData = GenerateDefaultConnectionTxData();
    InjectTxData(dataInventory_, txData);
    std::string processName = "python_main";
    MOCKER_CPP(&Context::GetPidNameFromInfoJson).stubs().will(returnValue(processName));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent(MSPROF_TX_FILE);
    std::string expectStr = "{\"name\":\"tx_default\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208020.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":3,"
                            "\"Payload_value\":283728437,\"Message_type\":9,\"event_type\":\"marker\"}},"
                            "{\"name\":\"process_name\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"python_main\"}},{\"name\":\"process_labels\",\"pid\":223,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,"
                            "\"tid\":1024,\"ph\":\"M\",\"args\":{\"name\":\"Thread 1024\"}},{\"name\":"
                            "\"thread_sort_index\",\"pid\":223,\"tid\":1024,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":1024}},";
    EXPECT_EQ(expectStr, content);
}

TEST_F(MsprofTxAssemblerUTest, ShouldGenerateThreadMetadataForDistinctTids)
{
    MsprofTxAssembler assembler;
    InjectTxData(dataInventory_, GenerateMultiThreadTxData());

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent(MSPROF_TX_FILE);
    std::string expectStr = "{\"name\":\"tx_default\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208020.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":3,"
                            "\"Payload_value\":283728437,\"Message_type\":9,\"event_type\":\"marker\"}},"
                            "{\"name\":\"thread_two\",\"pid\":223,\"tid\":2048,\"ts\":\"1717575960208021.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":3,"
                            "\"Payload_value\":283728437,\"Message_type\":9,\"event_type\":\"marker\"}},"
                            "{\"name\":\"thread_one_again\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208022.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":3,"
                            "\"Payload_value\":283728437,\"Message_type\":9,\"event_type\":\"marker\"}},"
                            "{\"name\":\"process_name\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"\"}},"
                            "{\"name\":\"process_labels\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,"
                            "\"tid\":1024,\"ph\":\"M\",\"args\":{\"name\":\"Thread 1024\"}},{\"name\":"
                            "\"thread_sort_index\",\"pid\":223,\"tid\":1024,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":1024}},{\"name\":\"thread_name\",\"pid\":223,\"tid\":2048,\"ph\":\"M\","
                            "\"args\":{\"name\":\"Thread 2048\"}},{\"name\":\"thread_sort_index\",\"pid\":223,"
                            "\"tid\":2048,\"ph\":\"M\",\"args\":{\"sort_index\":2048}},";
    EXPECT_EQ(expectStr, content);
}

TEST_F(MsprofTxAssemblerUTest, ShouldKeepUnknownEventTypeEmpty)
{
    MsprofTxAssembler assembler;
    auto txData = GenerateDefaultConnectionTxData();
    txData.front().eventType = 65530;
    txData.front().message = "unknown_type";
    InjectTxData(dataInventory_, txData);

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent(MSPROF_TX_FILE);
    std::string expectStr = "{\"name\":\"unknown_type\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208020.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":3,"
                            "\"Payload_value\":283728437,\"Message_type\":9,\"event_type\":\"\"}},{\"name\":"
                            "\"process_name\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"\"}},"
                            "{\"name\":\"process_labels\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,"
                            "\"tid\":1024,\"ph\":\"M\",\"args\":{\"name\":\"Thread 1024\"}},{\"name\":"
                            "\"thread_sort_index\",\"pid\":223,\"tid\":1024,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":1024}},";
    EXPECT_EQ(expectStr, content);
}
