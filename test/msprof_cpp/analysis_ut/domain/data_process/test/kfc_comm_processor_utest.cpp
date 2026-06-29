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
#include "analysis/csrc/domain/data_process/ai_task/kfc_comm_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/application/database/db_constant.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Application;
using HashMap = std::unordered_map<std::string, std::string>;

// "model_id", "index_id", "op_name", "op_timestamp", "op_duration", "iteration", "hccl_name",
// "group_name", "plane_id", "timestamp", "duration", "stream_id", "task_id", "duration_estimated", "local_rank",
// "remote_rank", "transport_type",
// "size", "data_type", "link_type", "bandwidth", "context_id", "notify_id", "batch_id", "rdma_type",
using OriKfcTaskData = std::vector<std::tuple<uint32_t, uint32_t, std::string, uint64_t, uint64_t, uint32_t,
        std::string, std::string, int32_t, uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint32_t, uint32_t,
        std::string, uint32_t, std::string, std::string, double, uint32_t, std::string, uint32_t, std::string>>;

// "model_id", "index_id", "op_name", "timestamp", "duration", "group_name", "connection_id", "data_type", "alg_type",
// "count", "rank_size
using OriKfcOPData = std::vector<std::tuple<uint32_t, uint32_t, std::string, uint64_t, uint64_t, std::string,
uint64_t, std::string, std::string, uint64_t, uint64_t>>;

const std::string BASE_PATH = "./hccl_single_device";
const std::string DEVICE_SUFFIX = "device_0";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "PROF_1"});
const std::string DB_SUFFIX = "hccl_single_device.db";
const std::string TASK_TABLE_NAME = "KfcTask";
const std::string OP_TABLE_NAME = "KfcOP";
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};

const OriKfcOPData KFC_OP_DATA = {
    {4294967295, 1, "MatmulAllReduceAddRmsNormAicpu_448_0_1", 58983469236240, 76305740, "6727616873621928448",
     2, NA, NA, -1, 0},
    {4294967295, 1, "MatmulAllReduceAddRmsNormAicpu_448_1_1", 58983545542040, 24515440, "6727616873621928448",
     9,  NA, NA, -1, 0}
};

const OriKfcTaskData KFC_TASK_DATA = {
    {4294967295, 1, "MatmulAllReduceAddRmsNormAicpu_448_0_1", 58983469236240, 76305740, 0,
     "Inter_Processor_Sync", "6727616873621928448", 0, 58983473419360, 2269800, 50, 600, 500, 0, 0,
     "3", 0, "195", "INVALID_TYPE", 0, 4294967295, "0", 0, "RDMA_SEND_NOTIFY"},
    {4294967295, 1, "MatmulAllReduceAddRmsNormAicpu_448_0_1", 58983469236240, 76305740, 0,
     "Notify_Record", "6727616873621928448", 0, 58983475689180, 20, 50, 601, 500, 0, 0,
     "3", 0, "195", "INVALID_TYPE", 0, 4294967295, "249", 0, "RDMA_SEND_NOTIFY"}
};

static void InitHashMap(DataInventory& dataInventory)
{
    HashMap hashMap;
    hashMap.emplace("111", "TEST");
    std::shared_ptr<std::unordered_map<std::string, std::string>> res;
    MAKE_SHARED_RETURN_VOID(res, HashMap, std::move(hashMap));
    dataInventory.Inject(res);
}

class KfcCommProcessorUtest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, 0);
        }
        File::CreateDir(BASE_PATH);
        File::CreateDir(PROF_PATH_A);
        File::CreateDir(PROF_PATH_B);
        File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX}));
        CreateKfcTaskData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_TASK_DATA);
        CreateKfcTaskData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_TASK_DATA);

        CreateKfcOpData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_OP_DATA);
        CreateKfcOpData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_OP_DATA);
    }

    static void TearDownTestCase()
    {
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }

    static void CreateKfcOpData(std::string dbPath, OriKfcOPData data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(OP_TABLE_NAME);
        dbRunner->CreateTable(OP_TABLE_NAME, cols);
        dbRunner->InsertData(OP_TABLE_NAME, data);
    }

    static void CreateKfcTaskData(std::string dbPath, OriKfcTaskData data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TASK_TABLE_NAME);
        dbRunner->CreateTable(TASK_TABLE_NAME, cols);
        dbRunner->InsertData(TASK_TABLE_NAME, data);
    }
};

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnTrueWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(OP_TABLE_NAME);
    dbRunner->DropTable(TASK_TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(OP_TABLE_NAME);
    dbRunner->DropTable(TASK_TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenCheckPathNotExist)
{
    MOCKER_CPP(&Analysis::Utils::File::Exist).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Exist).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = KfcCommProcessor({File::PathJoin({BASE_PATH, "test"})});
    auto dataInventory = DataInventory();
    InitHashMap(dataInventory);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(KfcCommProcessorUtest, TestConvertTaskDataShouldReturnTrueWhenConvertSuccess)
{
    const uint16_t num = 10;
    std::vector<CommunicationTaskData> taskData (num, CommunicationTaskData());
    std::vector<KfcTaskData> resTask;
    EXPECT_TRUE(KfcCommProcessor::ConvertTaskData(resTask, taskData));
    EXPECT_EQ(num, resTask.size());
}

TEST_F(KfcCommProcessorUtest, TestConvertTaskDataShouldReturnFalseWhenReserveFailed)
{
    std::vector<CommunicationTaskData> taskData;
    std::vector<KfcTaskData> resTask;
    MOCKER_CPP(&Utils::Reserve<KfcTaskData>).stubs().will(returnValue(false));
    EXPECT_FALSE(KfcCommProcessor::ConvertTaskData(resTask, taskData));
    MOCKER_CPP(&Utils::Reserve<KfcTaskData>).reset();
}

TEST_F(KfcCommProcessorUtest, TestConvertOpDataShouldReturnTrueWhenConvertSuccess)
{
    const uint16_t num = 10;
    std::vector<CommunicationOpData> opData (num, CommunicationOpData());
    std::vector<KfcOpData> resOp;
    EXPECT_TRUE(KfcCommProcessor::ConvertOpData(resOp, opData));
    EXPECT_EQ(num, resOp.size());
}

TEST_F(KfcCommProcessorUtest, TestConvertOpDataShouldReturnFalseWhenReserveFailed)
{
    std::vector<CommunicationOpData> opData;
    std::vector<KfcOpData> resOp;
    MOCKER_CPP(&Utils::Reserve<KfcOpData>).stubs().will(returnValue(false));
    EXPECT_FALSE(KfcCommProcessor::ConvertOpData(resOp, opData));
    MOCKER_CPP(&Utils::Reserve<KfcOpData>).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenCheckPathAndTableFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTaskData>).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<KfcTaskData>).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenSaveDataFailed)
{
    MOCKER_CPP(&KfcCommProcessor::SaveData).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&KfcCommProcessor::SaveData).reset();
}

TEST_F(KfcCommProcessorUtest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcCommProcessor(path);
        auto dataInventory = DataInventory();
        InitHashMap(dataInventory);
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}