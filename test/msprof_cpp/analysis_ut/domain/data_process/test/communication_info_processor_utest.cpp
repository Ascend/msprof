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
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Analysis::Application::Credential;
using namespace Analysis::Utils;
using namespace Domain::Environment;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t OP_NAME_NUM = 3;
const uint16_t CONNECTION_ID_NUM = 2;
const uint16_t OP_ID_NUM = 3;
const uint16_t OP_DATATYPE_NUM = 3;
const uint16_t OP_TYPE_NUM = 2;
const std::string COMMUNICATION_TASK_PATH = "./task_path";
const std::string DB_PATH = File::PathJoin({COMMUNICATION_TASK_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hccl_single_device.db";
const std::string PROF_PATH_A = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                   "./PROF_000001"});
const std::string PROF_PATH_B = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                   "./PROF_000002"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TASK_TABLE_NAME = "HCCLTaskSingleDevice";
const std::string OP_TABLE_NAME = "HCCLOpSingleDevice";
const std::string KFC_TASK_TABLE_NAME = "KfcTask";
const std::string KFC_OP_TABLE_NAME = "KfcOP";

using HcclTaskSingleDeviceFormat = std::vector<std::tuple<uint32_t, int32_t, std::string, uint32_t, std::string,
    std::string, double, int32_t, double, double, double,
    std::string, std::string, uint64_t, int32_t, uint64_t, uint32_t,
    double, uint32_t, uint32_t, std::string, uint64_t, std::string,
    std::string, double, uint32_t, uint64_t, uint32_t, std::string, uint32_t>>;

using HcclOpSingleDeviceFormat = std::vector<std::tuple<uint32_t, std::string, std::string, std::string,
    double, int32_t, int32_t, std::string, std::string,
    uint64_t, std::string, uint32_t, uint32_t>>;

using KfcTaskFormat = std::vector<std::tuple<uint32_t, int32_t, std::string, uint64_t, uint32_t, std::string,
    std::string, uint32_t, uint64_t, double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, std::string,
    uint32_t, std::string, std::string, double, uint32_t, std::string, uint32_t, std::string, uint32_t, uint32_t>>;

using KfcOpFormat = std::vector<std::tuple<uint32_t, uint32_t, std::string, double, double,
                                           std::string, uint32_t, std::string, uint32_t, uint32_t,
                                           std::string, std::string, uint64_t, uint32_t, uint32_t>>;

const HcclTaskSingleDeviceFormat DATA_A{
    {4294967295, -1, "hcom_allReduce__360_0_1", 0, "Memcpy",   "10652832407468360",
        78180470736653, 0, 781687236999151, 2994.875, 1, "HCCL", "hcom_allReduce_", 125,
        1, 11, 1, 14.1825906735751, 0, 0, "SDMA", 262144, "INVALID_TYPE",
        "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE", 4},
    {4294967295, -1, "hcom_allReduce__360_0_1", 0, "Reduce23", "10652832407468360",
        78180470736653, 0, 781687236999152, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
        1, 11, 2, 14.1825906735751, 0, 0, "SDMA", 262144, "FP16",
        "HCCS",    87.530865228098, 4294967295, 8,          1, "INVALID_TYPE", 4},
    // 用于测试无主流通信算子的场景
    {4294967295, -1, "hcom_allReduce__360_888_1", 0, "Reduce23", "10652832407468360",
        2000026362976, 0, 2000026362976, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
        0, 11, 2, 14.1825906735751, 0, 0, "SDMA", 262144, "FP16",
        "HCCS",    87.530865228098, 4294967295, 8,          1, "INVALID_TYPE", 4}
};
const HcclOpSingleDeviceFormat DATA_OP_A{
    {4294967295, "hcom_allReduce_", "HCCL", "hcom_allReduce_",
        821026362976, 0, 1, "INT16", "HD-NB", 3021, "10652832407468360", 125, 4},
};
const HcclTaskSingleDeviceFormat DATA_B{
    {4294967295, -1, "hcom_allReduce__233_0_2", 0, "Memcpy23", "10653832407468233",
        78180470736653, 0, 781687236999153, 2994.875, 3, "HCCL", "hcom_allReduce_", 125,
        1, 11, 3, 14.1825906735751, 1, 4, "SDMA", 262144, "INVALID_TYPE",
        "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE", 4},
    {4294967295, -1, "hcom_allReduce__832_0_1", 0, "Memcpy",   "10652853832407468832",
        78180470736653, 0, 781687236999154, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
        1, 21, 4, 14.1825906735751, 4, 2, "SDMA", 262144, "FP32",
        "HCCS",    87.530865228098, 4294967295, 8,          2, "INVALID_TYPE", 4}
};
const HcclOpSingleDeviceFormat DATA_OP_B{
    {4294967295, "hcom_allReduce_", "HCCL", "hcom_allReduce_",
        821026362976, 1, 1, "INT32", "HD-NHR", 4921, "10652853832407468832", 126, 4}
};

const KfcTaskFormat DATA_KFC_A{
    {4294967295, -1, "allreduceAicpuKernel_360_1_1", 41683029923680, 0, "Notify_Wait", "10652832407468360",
     1, 41683029923680, 20, 0, 69, 0, 0, 5, 6, "SDMA", 4, "INT8", "HCCS", 3.12, 4294967295, "102", 0,
     "INVALID_TYPE", 405, 0}
};
const KfcOpFormat DATA_KFC_OP_A{
    {4294967295, -1, "hcom_allReduce__360_0_1", 781687236999151, 35092402526.203125, "10652832407468360", 125,
     "AicpuKernel", 0, 1, "INT16", "HD-NB", 3021, 2, 0}
};
}

class CommunicationInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Exist(COMMUNICATION_TASK_PATH)) {
            File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMMUNICATION_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE})));
        CreateHcclTaskSingleDevice(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateHcclTaskSingleDevice(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        CreateHcclOpSingleDevice(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_OP_A);
        CreateHcclOpSingleDevice(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_OP_B);
        CreateKfcTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_KFC_A);
        CreateKfcOP(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_KFC_OP_A);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).reset();
    }
    static void CreateHcclTaskSingleDevice(const std::string& dbPath, HcclTaskSingleDeviceFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TASK_TABLE_NAME);
        dbRunner->CreateTable(TASK_TABLE_NAME, cols);
        dbRunner->InsertData(TASK_TABLE_NAME, data);
    }
    static void CreateHcclOpSingleDevice(const std::string& dbPath, HcclOpSingleDeviceFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(OP_TABLE_NAME);
        dbRunner->CreateTable(OP_TABLE_NAME, cols);
        dbRunner->InsertData(OP_TABLE_NAME, data);
    }
    static void CreateKfcTask(const std::string& dbPath, KfcTaskFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(KFC_TASK_TABLE_NAME);
        dbRunner->CreateTable(KFC_TASK_TABLE_NAME, cols);
        dbRunner->InsertData(KFC_TASK_TABLE_NAME, data);
    }
    static void CreateKfcOP(const std::string& dbPath, KfcOpFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(KFC_OP_TABLE_NAME);
        dbRunner->CreateTable(KFC_OP_TABLE_NAME, cols);
        dbRunner->InsertData(KFC_OP_TABLE_NAME, data);
    }
};

static void CheckStringId(std::vector<CommunicationTaskData> data)
{
    const std::set<std::string> nameSet = {"hcom_allReduce__360_0_1", "hcom_allReduce__233_0_2",
        "hcom_allReduce__832_0_1", "allreduceAicpuKernel_360_1_1"};
    const std::set<std::string> taskTypeSet = {"Memcpy", "Memcpy23", "Reduce23", "Notify_Wait"};
    const uint64_t rdmaTypeHashId = HCCL_RDMA_TYPE_TABLE.find("INVALID_TYPE")->second;
    const std::set<uint64_t> srcRankSet = {0, 1, 4, 5};
    const std::set<uint64_t> dstRankSet = {0, 2, 4, 6};
    const uint64_t transportTypeHashId = HCCL_TRANSPORT_TYPE_TABLE.find("SDMA")->second;
    const std::set<uint64_t> dataTypeSet = {HCCL_DATA_TYPE_TABLE.find("FP16")->second,
        HCCL_DATA_TYPE_TABLE.find("FP32")->second,
        HCCL_DATA_TYPE_TABLE.find("INVALID_TYPE")->second, HCCL_DATA_TYPE_TABLE.find("INT8")->second};
    const std::set<uint64_t> linkTypeSet = {HCCL_LINK_TYPE_TABLE.find("HCCS")->second,
        HCCL_LINK_TYPE_TABLE.find("ON_CHIP")->second};
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        EXPECT_NE(nameSet.find(item.opName), nameSet.end());
        EXPECT_NE(taskTypeSet.find(item.taskType), taskTypeSet.end());
        EXPECT_EQ(item.rdmaType, rdmaTypeHashId);
        EXPECT_NE(srcRankSet.find(item.srcRank), srcRankSet.end());
        EXPECT_NE(dstRankSet.find(item.dstRank), dstRankSet.end());
        EXPECT_EQ(item.transportType, transportTypeHashId);
        EXPECT_NE(dataTypeSet.find(item.dataType), dataTypeSet.end());
        EXPECT_NE(linkTypeSet.find(item.linkType), linkTypeSet.end());
    }
}
static void CheckOpInfo(std::vector<CommunicationOpData> data)
{
    std::set<std::string> opNameSet;
    std::set<uint64_t> connectionIdSet;
    std::set<std::string> opKeySet;
    std::set<uint64_t> opDataTypeSet;
    std::set<std::string> opTypeSet;
    for (auto item : data) {
        opNameSet.insert(item.opName);
        connectionIdSet.insert(item.connectionId);
        opKeySet.insert(item.opKey);
        opDataTypeSet.insert(item.dataType);
        opTypeSet.insert(item.opType);
        if (item.opName == "hcom_allReduce__360_0_1") {
            EXPECT_EQ(item.retry, 1);  // 重执行
        }
    }
    EXPECT_EQ(opNameSet.size(), OP_NAME_NUM);
    EXPECT_EQ(connectionIdSet.size(), CONNECTION_ID_NUM);
    EXPECT_EQ(opKeySet.size(), OP_ID_NUM);
    EXPECT_EQ(opDataTypeSet.size(), OP_DATATYPE_NUM);
    EXPECT_EQ(opTypeSet.size(), OP_TYPE_NUM);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<CommunicationTaskData> taskResult;
    std::vector<CommunicationOpData> opResult;
    std::string processorName = "COMMUNICATION_TASK_INFO";
    std::vector<CommunicationTaskData> taskRes;
    std::vector<CommunicationOpData> opRes;
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
        taskResult = *dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
        taskRes.insert(taskRes.end(), taskResult.begin(), taskResult.end());
        opResult = *dataInventory.GetPtr<std::vector<CommunicationOpData>>();
        opRes.insert(opRes.end(), opResult.begin(), opResult.end());
    }
    CheckOpInfo(opRes);
    CheckStringId(taskRes);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TASK_TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TASK_TABLE_NAME);
    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
    }
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    std::string processorName = "COMMUNICATION_TASK_INFO";
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, processorName));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    MOCKER_CPP(&Analysis::Domain::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    std::string processorName = "COMMUNICATION_TASK_INFO";
    for (auto path: PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, processorName));
    }
    MOCKER_CPP(&Analysis::Domain::Database::GetTableCols).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    StubReserveFailureForVector<std::vector<CommunicationTaskData>>();
    std::string processorName = "COMMUNICATION_TASK_INFO";
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, processorName));
    }
    ResetReserveFailureForVector<std::vector<CommunicationTaskData>>();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({COMMUNICATION_TASK_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    auto processor = CommunicationInfoProcessor({File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    auto dataInventory = DataInventory();
    dataInventory.Inject(geHashMapPtr);
    std::string processorName = "COMMUNICATION_TASK_INFO";
    EXPECT_TRUE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestFormatKfcDataShouldReturnFalseWhenReserveFailed)
{
    std::vector<CommunicationTaskData> taskData;
    std::vector<CommunicationOpData> opData;
    CommunicationInfoProcessor::CommunicationData communicationData;
    auto processor = CommunicationInfoProcessor({File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    MOCKER_CPP(&Utils::Reserve<CommunicationTaskData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.FormatKfcData(taskData, opData, communicationData));
    MOCKER_CPP(&Utils::Reserve<CommunicationTaskData>).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestShouldReturnFalseWhenHashMapIsNullptr)
{
    std::vector<std::string> deviceList = {File::PathJoin({COMMUNICATION_TASK_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    std::shared_ptr<GeHashMap> geHashMapPtr;
    auto processor = CommunicationInfoProcessor({File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    auto dataInventory = DataInventory();
    dataInventory.Inject(geHashMapPtr);
    std::string processorName = "COMMUNICATION_TASK_INFO";
    EXPECT_FALSE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestShouldReturnFalseWhenReserveTaskFormatDataFailed)
{
    std::vector<CommunicationTaskData> taskData;
    CommunicationInfoProcessor::CommunicationData communicationData;
    std::vector<CommunicationOpData> communicationOpData;
    auto processor = CommunicationInfoProcessor({File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    MOCKER_CPP(&Utils::Reserve<CommunicationTaskData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.FormatData(taskData, communicationOpData, communicationData));
    MOCKER_CPP(&Utils::Reserve<CommunicationTaskData>).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenSaveCommunicationTaskDataFailed)
{
    std::vector<CommunicationTaskData> taskResult;
    std::vector<CommunicationOpData> opResult;
    std::string processorName = "COMMUNICATION_TASK_INFO";
    std::vector<CommunicationTaskData> taskRes;
    std::vector<CommunicationOpData> opRes;
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    auto processor = CommunicationInfoProcessor(PROF_PATH_A);
    auto dataInventory = DataInventory();
    dataInventory.Inject(geHashMapPtr);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CommunicationTaskData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CommunicationTaskData>).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenSaveCommunicationOpDataFailed)
{
    std::vector<CommunicationTaskData> taskResult;
    std::vector<CommunicationOpData> opResult;
    std::string processorName = "COMMUNICATION_TASK_INFO";
    std::vector<CommunicationTaskData> taskRes;
    std::vector<CommunicationOpData> opRes;
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    auto processor = CommunicationInfoProcessor(PROF_PATH_A);
    auto dataInventory = DataInventory();
    dataInventory.Inject(geHashMapPtr);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CommunicationOpData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<CommunicationOpData>).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    auto processor = CommunicationInfoProcessor(PROF_PATH_A);
    std::string processorName = "COMMUNICATION_TASK_INFO";
    auto dataInventory = DataInventory();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, processorName));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}
