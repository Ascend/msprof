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
#include "analysis/csrc/application/timeline/host_usage_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"
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
const std::string BASE_PATH = "./host_usage_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class HostUsageAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
    }
    static void TearDownTestCase()
    {
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
        GlobalMockObject::verify();
    }
    virtual void TearDown()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
protected:
    static DataInventory dataInventory_;
};
DataInventory HostUsageAssemblerUTest::dataInventory_;

static void GenerateHostUsageData(DataInventory &dataInventory)
{
    std::shared_ptr<std::vector<CpuUsageData>> cpuResS;
    std::vector<CpuUsageData> cpuRes;
    CpuUsageData cpuData;
    cpuData.timestamp = 1719621074669030430; // start 1719621074669030430
    cpuData.usage = 30; // usage 30
    cpuData.cpuNo = "1"; // cpuNo 1
    cpuRes.push_back(cpuData);
    MAKE_SHARED_NO_OPERATION(cpuResS, std::vector<CpuUsageData>, cpuRes);
    dataInventory.Inject(cpuResS);

    std::shared_ptr<std::vector<MemUsageData>> memResS;
    std::vector<MemUsageData> memRes;
    MemUsageData memData;
    memData.timestamp = 1719621074669030430; // start 1719621074669030430
    memData.usage = 26; // usage 26
    memRes.push_back(memData);
    MAKE_SHARED_NO_OPERATION(memResS, std::vector<MemUsageData>, memRes);
    dataInventory.Inject(memResS);

    std::shared_ptr<std::vector<DiskUsageData>> diskResS;
    std::vector<DiskUsageData> diskRes;
    DiskUsageData diskData;
    diskData.timestamp = 1719621074669030430; // start 1719621074669030430
    diskData.usage = 25; // usage 25
    diskRes.push_back(diskData);
    MAKE_SHARED_NO_OPERATION(diskResS, std::vector<DiskUsageData>, diskRes);
    dataInventory.Inject(diskResS);

    std::shared_ptr<std::vector<NetWorkUsageData>> networkResS;
    std::vector<NetWorkUsageData> networkRes;
    NetWorkUsageData networkData;
    networkData.timestamp = 1719621074669030430; // start 1719621074669030430
    networkData.usage = 20; // usage 20
    networkRes.push_back(networkData);
    MAKE_SHARED_NO_OPERATION(networkResS, std::vector<NetWorkUsageData>, networkRes);
    dataInventory.Inject(networkResS);

    std::shared_ptr<std::vector<OSRuntimeApiData>> runtimeApiResS;
    std::vector<OSRuntimeApiData> runtimeApiRes;
    OSRuntimeApiData runtimeApiData;
    runtimeApiData.name = "clock_nanosleep"; // clock_nanosleep
    runtimeApiData.pid = 20; // pid 20
    runtimeApiData.tid = 123456; // tid 123456
    runtimeApiData.timestamp = 1719621074669030430; // start 1719621074669030430
    runtimeApiData.endTime = 1719621074669040430; // end 1719621074669040430
    runtimeApiRes.push_back(runtimeApiData);
    MAKE_SHARED_NO_OPERATION(runtimeApiResS, std::vector<OSRuntimeApiData>, runtimeApiRes);
    dataInventory.Inject(runtimeApiResS);
}

static void GenerateEmptyCpuUsageData(DataInventory &dataInventory)
{
    std::shared_ptr<std::vector<CpuUsageData>> cpuResS;
    std::vector<CpuUsageData> cpuRes;
    MAKE_SHARED_NO_OPERATION(cpuResS, std::vector<CpuUsageData>, cpuRes);
    dataInventory.Inject(cpuResS);
}

static void GenerateMultipleCpuUsageData(DataInventory &dataInventory)
{
    std::shared_ptr<std::vector<CpuUsageData>> cpuResS;
    std::vector<CpuUsageData> cpuRes;
    CpuUsageData cpuData1;
    cpuData1.timestamp = 1719621074669030430;
    cpuData1.usage = 30;
    cpuData1.cpuNo = "1";
    cpuRes.push_back(cpuData1);

    CpuUsageData cpuData2;
    cpuData2.timestamp = 1719621074669031430;
    cpuData2.usage = 40;
    cpuData2.cpuNo = "2";
    cpuRes.push_back(cpuData2);

    MAKE_SHARED_NO_OPERATION(cpuResS, std::vector<CpuUsageData>, cpuRes);
    dataInventory.Inject(cpuResS);
}

static void GenerateRuntimeApiDataWithDuplicateTid(DataInventory &dataInventory)
{
    std::shared_ptr<std::vector<OSRuntimeApiData>> runtimeApiResS;
    std::vector<OSRuntimeApiData> runtimeApiRes;

    OSRuntimeApiData runtimeApiData1;
    runtimeApiData1.name = "clock_nanosleep";
    runtimeApiData1.pid = 20;
    runtimeApiData1.tid = 123456;
    runtimeApiData1.timestamp = 1719621074669030430;
    runtimeApiData1.endTime = 1719621074669040430;
    runtimeApiRes.push_back(runtimeApiData1);

    OSRuntimeApiData runtimeApiData2 = runtimeApiData1;
    runtimeApiData2.name = "read";
    runtimeApiData2.timestamp += 1000;
    runtimeApiData2.endTime += 1000;
    runtimeApiRes.push_back(runtimeApiData2);

    OSRuntimeApiData runtimeApiData3 = runtimeApiData1;
    runtimeApiData3.name = "write";
    runtimeApiData3.tid = 654321;
    runtimeApiData3.timestamp += 2000;
    runtimeApiData3.endTime += 2000;
    runtimeApiRes.push_back(runtimeApiData3);

    MAKE_SHARED_NO_OPERATION(runtimeApiResS, std::vector<OSRuntimeApiData>, runtimeApiRes);
    dataInventory.Inject(runtimeApiResS);
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    CpuUsageAssembler cpu;
    EXPECT_TRUE(cpu.Run(dataInventory_, PROF_PATH));

    MemUsageAssembler mem;
    EXPECT_TRUE(mem.Run(dataInventory_, PROF_PATH));

    DiskUsageAssembler disk;
    EXPECT_TRUE(disk.Run(dataInventory_, PROF_PATH));

    NetworkUsageAssembler network;
    EXPECT_TRUE(network.Run(dataInventory_, PROF_PATH));

    OSRuntimeApiAssembler runtimeApi;
    EXPECT_TRUE(runtimeApi.Run(dataInventory_, PROF_PATH));
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenCpuSuccess)
{
    CpuUsageAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"CPU 1\",\"pid\":2383960383,\"tid\":0,\"ts\":\"1719621074669030.430\","
                            "\"ph\":\"C\",\"args\":{\"Usage(%)\":30.0}},{\"name\":\"process_name\",\"pid\":2383960383,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"CPU Usage\"}},{\"name\":\"process_labels\","
                            "\"pid\":2383960383,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"CPU\"}},{\"name\":"
                            "\"process_sort_index\",\"pid\":2383960383,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index"
                            "\":9}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenMemSuccess)
{
    MemUsageAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"Memory Usage\",\"pid\":2383960415,\"tid\":0,\"ts\":\"1719621074669030.430"
                            "\",\"ph\":\"C\",\"args\":{\"Usage(%)\":26.0}},{\"name\":\"process_name\","
                            "\"pid\":2383960415,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Memory Usage\"}},"
                            "{\"name\":\"process_labels\",\"pid\":2383960415,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960415,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":10}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenDiskSuccess)
{
    DiskUsageAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"Disk Usage\",\"pid\":2383960479,\"tid\":0,\"ts\":\"1719621074669030.430"
                            "\",\"ph\":\"C\",\"args\":{\"Usage(%)\":25.0}},{\"name\":\"process_name\","
                            "\"pid\":2383960479,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Disk Usage\"}},"
                            "{\"name\":\"process_labels\",\"pid\":2383960479,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960479,"
                            "\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":12}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenNetworkSuccess)
{
    NetworkUsageAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"Network Usage\",\"pid\":2383960447,\"tid\":0,\"ts\":\"1719621074669030.430"
                            "\",\"ph\":\"C\",\"args\":{\"Usage(%)\":20.0}},{\"name\":\"process_name\","
                            "\"pid\":2383960447,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Network Usage\"}},"
                            "{\"name\":\"process_labels\",\"pid\":2383960447,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960447,\"tid"
                            "\":0,\"ph\":\"M\",\"args\":{\"sort_index\":11}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnTrueWhenRuntimeApiSuccess)
{
    OSRuntimeApiAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"clock_nanosleep\",\"pid\":2383960511,\"tid\":123456,\"ts\":"
                            "\"1719621074669030.430\",\"dur\":10.0,\"ph\":\"X\",\"args\":{}},{\"name\":"
                            "\"thread_name\",\"pid\":2383960511,\"tid\":123456,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Thread 123456\"}},{\"name\":\"process_name\",\"pid\":2383960511,\"tid\":0,\"ph\":"
                            "\"M\",\"args\":{\"name\":\"OS Runtime API\"}},{\"name\":\"process_labels\","
                            "\"pid\":2383960511,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"CPU\"}},{\"name\":"
                            "\"process_sort_index\",\"pid\":2383960511,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"sort_index\":13}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    CpuUsageAssembler assembler;
    GenerateHostUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(HostUsageAssemblerUTest, ShouldReturnFalseWhenCpuDataVectorIsEmpty)
{
    CpuUsageAssembler assembler;
    GenerateEmptyCpuUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));

    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(HostUsageAssemblerUTest, ShouldGenerateAllCpuCountersWhenMultipleCpuRecordsExist)
{
    CpuUsageAssembler assembler;
    GenerateMultipleCpuUsageData(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"CPU 1\",\"pid\":2383960383,\"tid\":0,\"ts\":\"1719621074669030.430\","
                            "\"ph\":\"C\",\"args\":{\"Usage(%)\":30.0}},{\"name\":\"CPU 2\",\"pid\":2383960383,"
                            "\"tid\":0,\"ts\":\"1719621074669031.430\",\"ph\":\"C\",\"args\":{\"Usage(%)\":40.0}},"
                            "{\"name\":\"process_name\",\"pid\":2383960383,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"name\":\"CPU Usage\"}},{\"name\":\"process_labels\",\"pid\":2383960383,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\","
                            "\"pid\":2383960383,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":9}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HostUsageAssemblerUTest, ShouldGenerateThreadMetadataOnlyForUniqueRuntimeApiTid)
{
    OSRuntimeApiAssembler assembler;
    GenerateRuntimeApiDataWithDuplicateTid(dataInventory_);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086));

    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));

    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"clock_nanosleep\",\"pid\":2383960511,\"tid\":123456,\"ts\":"
                            "\"1719621074669030.430\",\"dur\":10.0,\"ph\":\"X\",\"args\":{}},{\"name\":\"read\","
                            "\"pid\":2383960511,\"tid\":123456,\"ts\":\"1719621074669031.430\",\"dur\":10.0,"
                            "\"ph\":\"X\",\"args\":{}},{\"name\":\"write\",\"pid\":2383960511,\"tid\":654321,"
                            "\"ts\":\"1719621074669032.430\",\"dur\":10.0,\"ph\":\"X\",\"args\":{}},{\"name\":"
                            "\"thread_name\",\"pid\":2383960511,\"tid\":123456,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Thread 123456\"}},{\"name\":\"thread_name\",\"pid\":2383960511,\"tid\":654321,"
                            "\"ph\":\"M\",\"args\":{\"name\":\"Thread 654321\"}},{\"name\":\"process_name\","
                            "\"pid\":2383960511,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"OS Runtime API\"}},"
                            "{\"name\":\"process_labels\",\"pid\":2383960511,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960511,\"tid\":0,"
                            "\"ph\":\"M\",\"args\":{\"sort_index\":13}},";
    EXPECT_EQ(expectStr, res.back());
}
