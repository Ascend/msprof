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
#include "analysis/csrc/application/database/db_constant.h"
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
const size_t TASK_NUM = 5;
const size_t TASK_NAME_NUM = 4;
const size_t OP_NUM = 3;
const size_t CONNECTION_ID_NUM = 3;
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

// 与 device_hccl_persistence.cpp 中 SaveHcclTaskData 写入顺序一致：
// modelId, indexId, hcclName, groupName, planeId, timestamp, duration,
// opId, isMaster, streamId, taskId, contextId, batchId, size, bandwidth,
// localRank, remoteRank, rankSize, transportType, dataType, linkType, rdmaType, notifyId, iterId
using HcclTaskSingleDeviceFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t,
    double, double, int64_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, double, double, int64_t, int64_t,
    int64_t, std::string, std::string, std::string, std::string, std::string, uint32_t>>;

// 与 device_hccl_persistence.cpp 中 SaveHcclOpData 写入顺序一致：
// modelId, indexId, opName, taskType, opType, start, end, relay, retry, dataType, algType, count, groupName,
// connectionId, rankSize, iterId
using HcclOpSingleDeviceFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, std::string,
    uint64_t, uint64_t, int32_t, int32_t, std::string, std::string, int32_t, std::string, int64_t, int64_t, uint32_t>>;

// KfcTask 与 HCCLTaskSingleDevice 结构一致，末尾多一列 source
using KfcTaskFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t,
    double, double, int64_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, double, double, int64_t, int64_t,
    int64_t, std::string, std::string, std::string, std::string, std::string, uint32_t, uint32_t>>;

// KfcOP 列序与 HCCLOpSingleDevice 不同：无 task_type，group_name 靠前，末尾有 source
using KfcOpFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, uint64_t, uint64_t, std::string,
    int64_t, std::string, int32_t, int32_t, std::string, std::string, int32_t, int64_t, uint32_t, uint32_t>>;

const HcclTaskSingleDeviceFormat DATA_A{
    {4294967295, -1, "hcom_allReduce_360", "group_1", 0, 781687236999151.0, 2994.875, 100, 1, 11, 1, 0, 1,
        262144.0, 87.53, 0, 1, 8, "SDMA", "FP16", "HCCS", "INVALID_TYPE", "4294967692", 1},
    {4294967295, -1, "hcom_allReduce_832", "group_1", 0, 781687236999152.0, 2994.875, 100, 0, 11, 2, 0, 1,
        262144.0, 87.53, 1, 0, 8, "SDMA", "FP32", "HCCS", "INVALID_TYPE", "4294967692", 2},
};
const HcclOpSingleDeviceFormat DATA_OP_A{
    {4294967295, -1, "hcom_allReduce_", "HCCL", "hcom_allReduce_", 781687236999151, 781687237000000, 0, 1,
        "FP16", "HD-NB", 3021, "group_1", 100, 8, 1},
};
const HcclTaskSingleDeviceFormat DATA_B{
    {4294967295, -1, "hcom_allReduce_233", "group_1", 0, 781687236999153.0, 2994.875, 300, 1, 11, 3, 0, 1,
        262144.0, 87.53, 2, 3, 8, "SDMA", "FP16", "HCCS", "INVALID_TYPE", "4294967693", 3},
    {4294967295, -1, "hcom_allReduce_832", "group_1", 0, 781687236999154.0, 2994.875, 300, 0, 11, 4, 0, 1,
        262144.0, 87.53, 3, 2, 8, "SDMA", "FP32", "HCCS", "INVALID_TYPE", "4294967693", 4},
};
const HcclOpSingleDeviceFormat DATA_OP_B{
    {4294967295, -1, "hcom_allReduce_", "HCCL", "hcom_allReduce_", 781687236999155, 781687237000000, 1, 1,
        "FP32", "HD-NHR", 4921, "group_1", 300, 8, 2},
};

const KfcTaskFormat DATA_KFC_A{
    {4294967295, -1, "allreduceAicpuKernel_360", "group_1", 0, 781687236999156.0, 20.0, 200, 1, 69, 0, 0, 0,
        1024.0, 3.12, 5, 6, 8, "SDMA", "INT8", "HCCS", "INVALID_TYPE", "102", 0, 1},
};
const KfcOpFormat DATA_KFC_OP_A{
    {4294967295, -1, "hcom_allReduce_360", 781687236999151, 35092402526, "group_1",
        200, "AicpuKernel", 0, 1, "INT8", "HD-NB", 3021, 8, 0, 1},
};
// KfcTask/KfcOP 的 source 列同时存在 HCCL(0) 与 MC2(1) 的行，末尾元素即 source
const KfcTaskFormat DATA_KFC_MIXED_SOURCE{
    {4294967295, -1, "allreduceAicpuKernel_100", "group_1", 0, 781687236999156.0, 20.0, 200, 1, 69, 0, 0, 0,
        1024.0, 3.12, 5, 6, 8, "SDMA", "INT8", "HCCS", "INVALID_TYPE", "102", 0, 0},  // source=0 => HCCL
    {4294967295, -1, "allreduceAicpuKernel_200", "group_1", 0, 781687236999157.0, 20.0, 200, 1, 69, 0, 0, 0,
        1024.0, 3.12, 5, 6, 8, "SDMA", "INT8", "HCCS", "INVALID_TYPE", "102", 0, 1},  // source=1 => MC2
};
const KfcOpFormat DATA_KFC_OP_MIXED_SOURCE{
    {4294967295, -1, "hcom_allReduce_100", 781687236999151, 35092402526, "group_1",
        200, "AicpuKernel", 0, 1, "INT8", "HD-NB", 3021, 8, 0, 0},  // source=0 => HCCL
    {4294967295, -1, "hcom_allReduce_200", 781687236999152, 35092402526, "group_1",
        200, "AicpuKernel", 0, 1, "INT8", "HD-NB", 3021, 8, 0, 1},  // source=1 => MC2
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
    // 建一张 0 行的空表（表存在但无数据），用于验证"无数据即跳过"而非"报失败"
    static void CreateEmptyTable(const std::string& dbPath, const std::string& tableName)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
    }
    // 删除指定表，模拟"表缺失"，用于验证四表独立处理、谁有谁处理
    static void DropTableFromDb(const std::string& dbPath, const std::string& tableName)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
        dbRunner->DropTable(tableName);
    }
};

static void CheckTaskInfo(std::vector<CommunicationTaskData> data)
{
    // 通信 task 处理后：taskType 与 hcclName 相同；transportType/dataType/linkType/rdmaType 均为字符串
    EXPECT_EQ(data.size(), TASK_NUM);
    std::set<std::string> hcclNameSet;
    std::set<std::string> dataTypeSet;
    for (auto item : data) {
        hcclNameSet.insert(item.hcclName);
        dataTypeSet.insert(item.dataType);
        EXPECT_EQ(item.taskType, item.hcclName);
        EXPECT_EQ(item.transportType, "SDMA");
        EXPECT_EQ(item.linkType, "HCCS");
        EXPECT_EQ(item.rdmaType, "INVALID_TYPE");
    }
    EXPECT_EQ(hcclNameSet.size(), TASK_NAME_NUM);
    EXPECT_EQ(dataTypeSet.size(), static_cast<size_t>(3));  // FP16 / FP32 / INT8
}

static void CheckOpInfo(std::vector<CommunicationOpData> data)
{
    EXPECT_EQ(data.size(), OP_NUM);
    std::set<int64_t> connectionIdSet;
    std::set<std::string> opNameSet;
    for (auto item : data) {
        connectionIdSet.insert(item.connectionId);
        opNameSet.insert(item.opName);
    }
    EXPECT_EQ(connectionIdSet.size(), CONNECTION_ID_NUM);
    EXPECT_EQ(opNameSet.size(), static_cast<size_t>(2));  // hcom_allReduce_ / hcom_allReduce_360
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
    CheckTaskInfo(taskRes);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReadKfcSourceFromData)
{
    // KfcTask/KfcOP 的 source 应从数据中来：数据里既有 source=hccl 也有 source=mc2 的行，
    // 处理结果必须各自保持，不能一律硬标成 mc2（回归：曾对所有 Kfc 行默认标 MC2）
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPath, KFC_TASK_TABLE_NAME);
    DropTableFromDb(dbPath, KFC_OP_TABLE_NAME);
    CreateKfcTask(dbPath, DATA_KFC_MIXED_SOURCE);
    CreateKfcOP(dbPath, DATA_KFC_OP_MIXED_SOURCE);

    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    auto processor = CommunicationInfoProcessor(PROF_PATH_A);
    auto dataInventory = DataInventory();
    dataInventory.Inject(geHashMapPtr);
    EXPECT_TRUE(processor.Run(dataInventory, processorName));

    auto taskResPtr = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
    ASSERT_TRUE(taskResPtr != nullptr);
    bool foundHcclTask = false;
    bool foundMc2Task = false;
    for (const auto& item : *taskResPtr) {
        if (item.hcclName == "allreduceAicpuKernel_100") {
            EXPECT_EQ(item.source, HcclType::HCCL);
            foundHcclTask = true;
        } else if (item.hcclName == "allreduceAicpuKernel_200") {
            EXPECT_EQ(item.source, HcclType::MC2);
            foundMc2Task = true;
        }
    }
    EXPECT_TRUE(foundHcclTask);
    EXPECT_TRUE(foundMc2Task);

    auto opResPtr = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    ASSERT_TRUE(opResPtr != nullptr);
    bool foundHcclOp = false;
    bool foundMc2Op = false;
    for (const auto& item : *opResPtr) {
        if (item.opName == "hcom_allReduce_100") {
            EXPECT_EQ(item.source, HcclType::HCCL);
            foundHcclOp = true;
        } else if (item.opName == "hcom_allReduce_200") {
            EXPECT_EQ(item.source, HcclType::MC2);
            foundMc2Op = true;
        }
    }
    EXPECT_TRUE(foundHcclOp);
    EXPECT_TRUE(foundMc2Op);
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

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenTableEmpty)
{
    // 表存在但 0 行：HCCL/KFC 均应"跳过"而非"报失败"，Run 返回 true 且不产生任何数据
    // HCCL 表在 SetUp 中 A/B 均已创建，统一清空；KFC 表仅在 A 创建，B 走 NOT_EXIST 跳过
    auto dbPathA = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunnerA;
    MAKE_SHARED0_NO_OPERATION(dbRunnerA, DBRunner, dbPathA);
    dbRunnerA->DropTable(TASK_TABLE_NAME);
    dbRunnerA->DropTable(OP_TABLE_NAME);
    dbRunnerA->DropTable(KFC_TASK_TABLE_NAME);
    dbRunnerA->DropTable(KFC_OP_TABLE_NAME);
    CreateEmptyTable(dbPathA, TASK_TABLE_NAME);
    CreateEmptyTable(dbPathA, OP_TABLE_NAME);
    CreateEmptyTable(dbPathA, KFC_TASK_TABLE_NAME);
    CreateEmptyTable(dbPathA, KFC_OP_TABLE_NAME);

    auto dbPathB = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunnerB;
    MAKE_SHARED0_NO_OPERATION(dbRunnerB, DBRunner, dbPathB);
    dbRunnerB->DropTable(TASK_TABLE_NAME);
    dbRunnerB->DropTable(OP_TABLE_NAME);
    CreateEmptyTable(dbPathB, TASK_TABLE_NAME);
    CreateEmptyTable(dbPathB, OP_TABLE_NAME);

    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
        EXPECT_TRUE(dataInventory.GetPtr<std::vector<CommunicationTaskData>>() == nullptr);
        EXPECT_TRUE(dataInventory.GetPtr<std::vector<CommunicationOpData>>() == nullptr);
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

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldExportTaskDataWhenOpTableNotExist)
{
    // 所有 op 表缺失时，task 表数据仍应正常导出（四表独立，谁有谁处理）
    auto dbPathA = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPathA, OP_TABLE_NAME);
    DropTableFromDb(dbPathA, KFC_OP_TABLE_NAME);
    auto dbPathB = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPathB, OP_TABLE_NAME);

    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    std::vector<CommunicationTaskData> taskRes;
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
        auto opResPtr = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
        EXPECT_TRUE(opResPtr == nullptr);  // op 表全缺失，不应注入 op 数据
        auto taskResPtr = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
        if (taskResPtr != nullptr) {
            taskRes.insert(taskRes.end(), taskResPtr->begin(), taskResPtr->end());
        }
    }
    // HCCL task 4 条（A、B 各 2）+ KfcTask 1 条（仅 A）= 5 条
    EXPECT_EQ(taskRes.size(), TASK_NUM);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldExportOpDataWhenTaskTableNotExist)
{
    // 所有 task 表缺失时，op 表数据仍应正常导出（四表独立，谁有谁处理）
    auto dbPathA = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPathA, TASK_TABLE_NAME);
    DropTableFromDb(dbPathA, KFC_TASK_TABLE_NAME);
    auto dbPathB = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPathB, TASK_TABLE_NAME);

    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    std::vector<CommunicationOpData> opRes;
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
        auto taskResPtr = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
        EXPECT_TRUE(taskResPtr == nullptr);  // task 表全缺失，不应注入 task 数据
        auto opResPtr = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
        if (opResPtr != nullptr) {
            opRes.insert(opRes.end(), opResPtr->begin(), opResPtr->end());
        }
    }
    // HCCL op 2 条（A、B 各 1）+ KfcOP 1 条（仅 A）= 3 条
    CheckOpInfo(opRes);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldProcessHcclWhenKfcTableMissing)
{
    // Kfc 表全缺失时，HCCL 数据仍正常导出（HCCL 与 KFC 独立处理）
    auto dbPathA = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    DropTableFromDb(dbPathA, KFC_TASK_TABLE_NAME);
    DropTableFromDb(dbPathA, KFC_OP_TABLE_NAME);

    std::string processorName = "COMMUNICATION_TASK_INFO";
    GeHashMap geHashMap = {{"key1", "value1"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    std::vector<CommunicationTaskData> taskRes;
    std::vector<CommunicationOpData> opRes;
    for (auto path : PROF_PATHS) {
        auto processor = CommunicationInfoProcessor(path);
        auto dataInventory = DataInventory();
        dataInventory.Inject(geHashMapPtr);
        EXPECT_TRUE(processor.Run(dataInventory, processorName));
        auto taskResPtr = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
        if (taskResPtr != nullptr) {
            taskRes.insert(taskRes.end(), taskResPtr->begin(), taskResPtr->end());
        }
        auto opResPtr = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
        if (opResPtr != nullptr) {
            opRes.insert(opRes.end(), opResPtr->begin(), opResPtr->end());
        }
    }
    // HCCL task 4 条（A、B 各 2），Kfc 数据已被 drop
    EXPECT_EQ(taskRes.size(), static_cast<size_t>(4));
    // HCCL op 2 条（A、B 各 1）
    EXPECT_EQ(opRes.size(), static_cast<size_t>(2));
}
