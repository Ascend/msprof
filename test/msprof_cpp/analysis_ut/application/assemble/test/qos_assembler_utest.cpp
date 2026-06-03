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
#include "analysis/csrc/application/timeline/qos_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
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
const std::string BASE_PATH = "./qos_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class QosAssemblerUTest : public testing::Test {
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

static void InjectQosData(DataInventory &dataInventory, const std::vector<QosData> &qosData)
{
    std::shared_ptr<std::vector<QosData>> dataS;
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<QosData>, qosData);
    dataInventory.Inject(dataS);
}

static std::vector<QosData> GenerateQosData()
{
    std::vector<QosData> res;
    QosData data;
    data.deviceId = 0; // 0
    data.timestamp = 1724405892226599429; // 1724405892226599429
    data.bw1 = 10; // 10
    data.bw2 = 10; // 10
    data.bw3 = 10; // 10
    data.bw4 = 10; // 10
    data.bw5 = 10; // 10
    data.bw6 = 10; // 10
    data.bw7 = 10; // 10
    data.bw8 = 10; // 10
    data.bw9 = 10; // 10
    data.bw10 = 10; // 10
    res.push_back(data);
    data.timestamp = 1724405892227599429; // 1724405892227599429
    data.bw1 = 20; // 20
    data.bw2 = 20; // 20
    data.bw3 = 20; // 20
    data.bw4 = 20; // 20
    data.bw5 = 20; // 20
    data.bw6 = 20; // 20
    data.bw7 = 20; // 20
    data.bw8 = 20; // 20
    data.bw9 = 20; // 20
    data.bw10 = 20; // 20
    res.push_back(data);
    data.timestamp = 1724405892228599429; // 1724405892228599429
    data.bw1 = 30; // 30
    data.bw2 = 30; // 30
    data.bw3 = 30; // 30
    data.bw4 = 30; // 30
    data.bw5 = 30; // 30
    data.bw6 = 30; // 30
    data.bw7 = 30; // 30
    data.bw8 = 30; // 30
    data.bw9 = 30; // 30
    data.bw10 = 30; // 30
    res.push_back(data);
    return res;
}

static std::vector<QosData> GenerateQosV6Data()
{
    std::vector<QosData> res;
    QosData dieZero;
    dieZero.deviceId = 0;
    dieZero.dieId = 0;
    dieZero.timestamp = 1724405892226599429;
    dieZero.bw3 = 11;
    dieZero.bw4 = 12;
    res.push_back(dieZero);

    QosData dieOne = dieZero;
    dieOne.dieId = 1;
    dieOne.bw3 = 21;
    dieOne.bw4 = 22;
    res.push_back(dieOne);
    return res;
}

TEST_F(QosAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    QosAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(QosAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    QosAssembler assembler;
    auto data = GenerateQosData();
    InjectQosData(dataInventory_, data);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    std::vector<std::string> qosEvents {"OTHERS", "DVPP"};
    MOCKER_CPP(&Context::GetQosEvents).stubs().will(returnValue(qosEvents));
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(static_cast<unsigned short>(0)));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"name\":\"QoS\"}},{\"name\":\"process_labels\",\"pid\":2383960992,\"tid\""
                            ":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\""
                            "process_sort_index\",\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\""
                            "args\":{\"sort_index\":29}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(QosAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    QosAssembler assembler;
    auto data = GenerateQosData();
    InjectQosData(dataInventory_, data);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(QosAssemblerUTest, ShouldGenerateMergedDieSeriesForV6Platform)
{
    QosAssembler assembler;
    InjectQosData(dataInventory_, GenerateQosV6Data());
    std::vector<std::string> qosEvents {"OTHERS", "DVPP"};
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    MOCKER_CPP(&Context::GetQosEvents).stubs().will(returnValue(qosEvents));
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(static_cast<unsigned short>(15)));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV4).stubs().will(returnValue(false));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent();
    std::string expectMeta = "{\"name\":\"process_name\",\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\"args\":"
                             "{\"name\":\"QoS\"}},{\"name\":\"process_labels\",\"pid\":2383960992,\"tid\":0,"
                             "\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\","
                             "\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":29}},";
    std::string expectOthers1 = "{\"name\":\"QoS OTHERS\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892226599.429\","
                                "\"ph\":\"C\",\"args\":{\"die 0\":11,\"die 1\":21}}";
    std::string expectOthers2 = "{\"name\":\"QoS OTHERS\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892226599.429\","
                                "\"ph\":\"C\",\"args\":{\"die 1\":21,\"die 0\":11}}";
    std::string expectDvpp1 = "{\"name\":\"QoS DVPP\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892226599.429\","
                              "\"ph\":\"C\",\"args\":{\"die 0\":12,\"die 1\":22}}";
    std::string expectDvpp2 = "{\"name\":\"QoS DVPP\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892226599.429\","
                              "\"ph\":\"C\",\"args\":{\"die 1\":22,\"die 0\":12}}";
    EXPECT_EQ(0ul, content.find(expectMeta));
    EXPECT_TRUE(content.find(expectOthers1) != std::string::npos || content.find(expectOthers2) != std::string::npos);
    EXPECT_TRUE(content.find(expectDvpp1) != std::string::npos || content.find(expectDvpp2) != std::string::npos);
}

TEST_F(QosAssemblerUTest, ShouldGenerateCounterSeriesForV4Platform)
{
    QosAssembler assembler;
    auto data = GenerateQosData();
    InjectQosData(dataInventory_, data);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));
    std::vector<std::string> qosEvents {"OTHERS", "DVPP"};
    MOCKER_CPP(&Context::GetQosEvents).stubs().will(returnValue(qosEvents));
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(static_cast<unsigned short>(15)));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    MOCKER_CPP(&Context::IsChipV4).stubs().will(returnValue(true));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    std::string content = ReadTraceContent();
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"name\":\"QoS\"}},{\"name\":\"process_labels\",\"pid\":2383960992,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},{\"name\":\"process_sort_index\","
                            "\"pid\":2383960992,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":29}},{\"name\":"
                            "\"QoS OTHERS\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892226599.429\","
                            "\"ph\":\"C\",\"args\":{\"value\":10}},{\"name\":\"QoS DVPP\",\"pid\":2383960992,"
                            "\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"value\":10}},"
                            "{\"name\":\"QoS OTHERS\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892227599.429\","
                            "\"ph\":\"C\",\"args\":{\"value\":20}},{\"name\":\"QoS DVPP\",\"pid\":2383960992,"
                            "\"tid\":0,\"ts\":\"1724405892227599.429\",\"ph\":\"C\",\"args\":{\"value\":20}},"
                            "{\"name\":\"QoS OTHERS\",\"pid\":2383960992,\"tid\":0,\"ts\":\"1724405892228599.429\","
                            "\"ph\":\"C\",\"args\":{\"value\":30}},{\"name\":\"QoS DVPP\",\"pid\":2383960992,"
                            "\"tid\":0,\"ts\":\"1724405892228599.429\",\"ph\":\"C\",\"args\":{\"value\":30}},";
    EXPECT_EQ(expectStr, content);
}
