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
#include "analysis/csrc/domain/data_process/system/host_usage_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./host_usage_data";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string CPU_DB_NAME = "host_cpu_usage.db";
const std::string CPU_TABLE_NAME = "CpuUsage";
const std::string MEM_DB_NAME = "host_mem_usage.db";
const std::string MEM_TABLE_NAME = "MemUsage";
const std::string DISK_DB_NAME = "host_disk_usage.db";
const std::string DISK_TABLE_NAME = "DiskUsage";
const std::string NETWORK_DB_NAME = "host_network_usage.db";
const std::string NETWORK_TABLE_NAME = "NetworkUsage";
const std::string RUNTIME_API_DB_NAME = "host_runtime_api.db";
const std::string RUNTIME_API_TABLE_NAME = "Syscall";
}
using CpuUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, std::string, double>>;
CpuUsageDataType cpuUsage{{3758215093862910, 3758215114581640, "50", 0},
                          {3758215093862910, 3758215114581640, "Avg", 0},
                          {3758215114581640, 3758215135288460, "54", 50}};
using MemUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double>>;
MemUsageDataType memUsage{{3758215115714500, 3758215406847370, 0.144066},
                          {3758215406847370, 3758215712894170, 0.144523}};
using DiskUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double, double, std::string, double>>;
DiskUsageDataType diskUsage{{35675969243521, 35675989243521, 0, 0, "0.00", 0},
                            {35676649243521, 35676669243521, 1000.23, 10.56, "0.00", 0.98}};
using NetWorkUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double, double>>;
NetWorkUsageDataType networkUsage{{35675490307527, 35675511080960, 0.002465, 3.00865051962984},
                                  {35675531284823, 35675552244520, 0, 0},
                                  {35675697968700, 35675718727140, 0.002466, 3.01082354863434}};
using RuntimeApiDataType = std::vector<std::tuple<std::string, uint64_t, uint64_t, std::string,
    uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
RuntimeApiDataType runtimeApiData{
    {"MSVP_ProfTimer", 206226, 206315, "openat", 35675490307000, 49000, 35675490356000,
        35675490488000, 35675490492900},
    {"MSVP_ProfTimer", 206226, 206315, "read", 35675499907000, 20400000, 35675520307000,
        35675534307000, 35675554707000},
    {"MSVP_UploaderD", 206226, 206315, "clock_nanosleep", 35675500307000, 50000000, 35675550307000,
        35675770307000, 35675820307000},
};

class HostUsageProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST, SQLITE})));
        std::string sqlitePath = File::PathJoin({PROF_PATH_A, HOST, SQLITE});
        EXPECT_TRUE(CreateCpuUsageData(File::PathJoin({sqlitePath, CPU_DB_NAME}), cpuUsage));
        EXPECT_TRUE(CreateMemUsageData(File::PathJoin({sqlitePath, MEM_DB_NAME}), memUsage));
        EXPECT_TRUE(CreateDiskUsageData(File::PathJoin({sqlitePath, DISK_DB_NAME}), diskUsage));
        EXPECT_TRUE(CreateNetWorkUsageData(File::PathJoin({sqlitePath, NETWORK_DB_NAME}), networkUsage));
        EXPECT_TRUE(CreateRuntimeApiData(File::PathJoin({sqlitePath, RUNTIME_API_DB_NAME}), runtimeApiData));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "30001129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static bool CreateCpuUsageData(const std::string &dbPath, CpuUsageDataType &data)
    {
        std::shared_ptr<HostCpuUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostCpuUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(CPU_TABLE_NAME);
        dbRunner->CreateTable(CPU_TABLE_NAME, cols);
        dbRunner->InsertData(CPU_TABLE_NAME, data);
        return true;
    }
    static bool CreateMemUsageData(const std::string &dbPath, MemUsageDataType &data)
    {
        std::shared_ptr<HostMemUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostMemUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(MEM_TABLE_NAME);
        dbRunner->CreateTable(MEM_TABLE_NAME, cols);
        dbRunner->InsertData(MEM_TABLE_NAME, data);
        return true;
    }
    static bool CreateDiskUsageData(const std::string &dbPath, DiskUsageDataType &data)
    {
        std::shared_ptr<HostDiskUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostDiskUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(DISK_TABLE_NAME);
        dbRunner->CreateTable(DISK_TABLE_NAME, cols);
        dbRunner->InsertData(DISK_TABLE_NAME, data);
        return true;
    }
    static bool CreateNetWorkUsageData(const std::string &dbPath, NetWorkUsageDataType &data)
    {
        std::shared_ptr<HostNetworkUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostNetworkUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(NETWORK_TABLE_NAME);
        dbRunner->CreateTable(NETWORK_TABLE_NAME, cols);
        dbRunner->InsertData(NETWORK_TABLE_NAME, data);
        return true;
    }
    static bool CreateRuntimeApiData(const std::string &dbPath, RuntimeApiDataType &data)
    {
        std::shared_ptr<HostRuntimeApi> database;
        MAKE_SHARED_RETURN_VALUE(database, HostRuntimeApi, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(RUNTIME_API_TABLE_NAME);
        dbRunner->CreateTable(RUNTIME_API_TABLE_NAME, cols);
        dbRunner->InsertData(RUNTIME_API_TABLE_NAME, data);
        return true;
    }
};

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenCpuUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    auto res = dataInventory.GetPtr<std::vector<CpuUsageData>>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HostUsageProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);

    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(HostUsageProcessorUTest, TestHostCpuUsageProcessorRunShouldReturnFalseWhenProcessDataFailed)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CpuUsageData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CpuUsageData>).reset();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenCpuUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<CpuUsageData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    ResetReserveFailureForVector<std::vector<CpuUsageData>>();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenCpuUsageProcessCheckFailed)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
}

TEST_F(HostUsageProcessorUTest, ShouldReturnTrueWhenCpuUsageProcessPathNotExists)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::File::Exist).stubs().will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<CpuUsageData>>());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenMemUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostMemUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_MEM_USAGE));
    auto res = dataInventory.GetPtr<std::vector<MemUsageData>>();
    EXPECT_EQ(2ul, res->size());
}


TEST_F(HostUsageProcessorUTest, TestHostMemUsageProcessorRunShouldReturnFalseWhenProcessDataFailed)
{
    DataInventory dataInventory;
    auto processor = HostMemUsageProcessor(PROF_PATH_A);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MemUsageData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEM_USAGE));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MemUsageData>).reset();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenMemUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostMemUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<MemUsageData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEM_USAGE));
    ResetReserveFailureForVector<std::vector<MemUsageData>>();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenDiskUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostDiskUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_DISK_USAGE));
    auto res = dataInventory.GetPtr<std::vector<DiskUsageData>>();
    std::cout << "res size is " << res->size() << std::endl;
    EXPECT_EQ(2ul, res->size());
}

TEST_F(HostUsageProcessorUTest, TestHostDiskUsageProcessorRunShouldReturnFalseWhenProcessDataFailed)
{
    DataInventory dataInventory;
    auto processor = HostDiskUsageProcessor(PROF_PATH_A);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DiskUsageData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DISK_USAGE));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DiskUsageData>).reset();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenDiskUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostDiskUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<DiskUsageData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DISK_USAGE));
    ResetReserveFailureForVector<std::vector<DiskUsageData>>();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenNetworkUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostNetworkUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NETWORK_USAGE));
    auto res = dataInventory.GetPtr<std::vector<NetWorkUsageData>>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HostUsageProcessorUTest, TestHostNetworkUsageProcessorRunShouldReturnFalseWhenProcessDataFailed)
{
    DataInventory dataInventory;
    auto processor = HostNetworkUsageProcessor(PROF_PATH_A);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NetWorkUsageData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETWORK_USAGE));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<NetWorkUsageData>).reset();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenNetworkUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostNetworkUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<NetWorkUsageData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETWORK_USAGE));
    ResetReserveFailureForVector<std::vector<NetWorkUsageData>>();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenRuntimeApiProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = OSRuntimeApiProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_OSRT_API));
    auto res = dataInventory.GetPtr<std::vector<OSRuntimeApiData>>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HostUsageProcessorUTest, TestOSRuntimeApiProcessorRunShouldReturnFalseWhenProcessDataFailed)
{
    DataInventory dataInventory;
    auto processor = OSRuntimeApiProcessor(PROF_PATH_A);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<OSRuntimeApiData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_OSRT_API));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<OSRuntimeApiData>).reset();
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenRuntimeApiProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = OSRuntimeApiProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    StubReserveFailureForVector<std::vector<OSRuntimeApiData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_OSRT_API));
    ResetReserveFailureForVector<std::vector<OSRuntimeApiData>>();
}
