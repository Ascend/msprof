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
#include "analysis/csrc/domain//data_process/ai_task/api_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;

const std::string API_DIR = "./api_data";
const std::string PROF0 = File::PathJoin({API_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({API_DIR, "PROF_1"});
const std::string PROF2 = File::PathJoin({API_DIR, "PROF_2"});
const std::string PROF3 = File::PathJoin({API_DIR, "PROF_3"});
const std::string TABLE_NAME = "ApiData";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1, PROF2, PROF3};

const OriApiDataFormat API_DATA = {
    {"StreamSyncTaskFinish", "0", "runtime", 116, "0", 65177262396323, 65177262396323, 1},
    {"ACL_RTS", "aclrtSynchronizeStreamWithTimeout", "acl", 116, "0", 65177262395395, 65177262397115, 2},
    {"ACL_RTS", "aclrtMemcpy", "test", 116, "0", 65177262397274, 65177262404692, 4},
    {"launch", "0", "node", 116, "Index4", 65177262436161, 65177262437816, 24},
    {"ACL_OP", "aclCreateTensorDesc", "acl", 116, "0", 65177262448526, 65177262448545, 35},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264896891, 65177264928192, 224},
    {"HOST_HCCL", "hcom_allReduce_", "acl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 245},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 247},
    {"ACL_OTHERS", "262144", "acl", 116, "GraphOperation::Setup", 65177265026179, 65177265048078, 299},
    {"master", "hccl", "communication", 116, "ModelLoad", 65177262397274, 65177262404692, 7},
};
const uint16_t LEVEL_INDEX = 2;
const uint16_t TID_INDEX = 3;
const uint16_t CONNECTION_ID_INDEX = 7;

class ApiProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(API_DIR)) {
            File::RemoveDir(API_DIR, 0);
        }
        EXPECT_TRUE(File::CreateDir(API_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF0, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF1, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF1, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF2));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF2, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF2, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF3));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF3, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF3, HOST, SQLITE}), API_DATA));
    }

    static bool CreateAPIDB(const std::string& sqlitePath, OriApiDataFormat data)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<ApiEventDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, ApiEventDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, data));
        return true;
    }

    static void TearDownTestCase()
    {
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(API_DIR, 0));
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
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }
};

void CheckApiDataValid(const std::vector<ApiData> &checkData)
{
    size_t index = 0;
    for (const auto &data : checkData) {
        // 比对前后的level是否一致
        auto tempTypeIt = API_LEVEL_TABLE.find(std::get<LEVEL_INDEX>(API_DATA[index]));
        EXPECT_EQ(data.level, (tempTypeIt == API_LEVEL_TABLE.end()) ? UINT16_MAX : tempTypeIt->second);
        uint32_t oriTid = std::get<TID_INDEX>(API_DATA[index]);
        uint32_t oriConId = std::get<CONNECTION_ID_INDEX>(API_DATA[index]);
        // 分别校验获取到的tid和connectionId的低32位是否与原先保持一致。
        EXPECT_EQ((data.threadId & 0xffffffff), oriTid);
        EXPECT_EQ((data.connectionId & 0xffffffff), oriConId);
        ++index;
    }
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<DataInventory> res(PROF_PATHS.size());
    size_t i = 0;
    for (const auto& profPath : PROF_PATHS) {
        auto processor = ApiProcessor(profPath);
        EXPECT_TRUE(processor.Run(res[i], PROCESSOR_NAME_API));
        ++i;
    }
    for (auto& node : res) {
        auto checkData = node.GetPtr<std::vector<ApiData>>();
        EXPECT_EQ(API_DATA.size(), checkData->size());
        CheckApiDataValid(*checkData);
        node.RemoveRestData({});
    }
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = ApiProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&DBInfo::ConstructDBRunner)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<ApiData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<ApiData>).reset();
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = ApiProcessor(PROF0);
    DataInventory dataInventory;
    StubReserveFailureForVector<std::vector<ApiData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    ResetReserveFailureForVector<std::vector<ApiData>>();
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnFalseWhenFileOverMaxSize)
{
    auto processor = ApiProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&FileReader::Check).reset();
}

TEST_F(ApiProcessorUTest, TestRunShouldFalseWhenApiDataIsEmpty)
{
    auto processor = ApiProcessor(PROF0);
    DataInventory dataInventory;
    OriApiDataFormat emptyApiData;

    MOCKER_CPP(&ApiProcessor::LoadData)
    .stubs()
    .will(returnValue(emptyApiData));

    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&ApiProcessor::LoadData)
    .reset();

    DBInfo apiDB("api_event.db", "ApiData");
    std::string dbPath = Utils::File::PathJoin({PROF0, HOST, SQLITE, apiDB.dbName});
    apiDB.dbRunner == nullptr;
    OriApiDataFormat oriData;
    ASSERT_EQ(processor.LoadData(apiDB, dbPath).size(), oriData.size());
}
