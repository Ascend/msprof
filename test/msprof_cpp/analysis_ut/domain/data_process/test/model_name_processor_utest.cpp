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
#include "analysis/csrc/domain/data_process/ai_task/model_name_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/data_process/data_processor.h";
#include "analysis/csrc/infrastructure/db/include/db_runner.h";
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Test;
using namespace Analysis::Viewer::Database;

const std::string API_DIR = "./model_name";
const std::string PROF0 = File::PathJoin({API_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({API_DIR, "PROF_1"});
const std::string TABLE_NAME = "ModelName";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1};

const OriModelNameDataFormat MODEL_NAME_DATA = {
    {1, "ge_default_20240924192820_1"},
    {1, "tf_resnet50"},
    {5, "graph_1_0"},
};

class ModelNameProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(API_DIR)) {
            File::RemoveDir(API_DIR, 0);
        }
        EXPECT_TRUE(File::CreateDir(API_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF0, HOST, SQLITE}), MODEL_NAME_DATA));
        EXPECT_TRUE(File::CreateDir(PROF1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF1, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF1, HOST, SQLITE}), MODEL_NAME_DATA));
    }

    static bool CreateAPIDB(const std::string& sqlitePath, OriModelNameDataFormat data)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<GeModelInfoDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, GeModelInfoDB, false);
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

TEST_F(ModelNameProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<DataInventory> res(PROF_PATHS.size());
    size_t i = 0;
    for (const auto& profPath : PROF_PATHS) {
        auto processor = ModelNameProcessor(profPath);
        EXPECT_TRUE(processor.Run(res[i], PROCESSOR_NAME_MODEL_NAME));
        ++i;
    }
    for (auto& node : res) {
        auto checkData = node.GetPtr<std::vector<ModelName>>();
        EXPECT_EQ(MODEL_NAME_DATA.size(), checkData->size());
        node.RemoveRestData({});
    }
}

TEST_F(ModelNameProcessorUTest, TestProcessShouldReturnFalseWhenEachStepFailed)
{
    auto processor = ModelNameProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MODEL_NAME));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();

    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
    .stubs()
    .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MODEL_NAME));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    OriModelNameDataFormat oriData;
    MOCKER_CPP(&ModelNameProcessor::LoadData)
    .stubs()
    .will(returnValue(oriData));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MODEL_NAME));
    MOCKER_CPP(&ModelNameProcessor::LoadData).reset();

    std::vector<ModelName> formatData;
    MOCKER_CPP(&ModelNameProcessor::FormatData)
    .stubs()
    .will(returnValue(formatData));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MODEL_NAME));
    MOCKER_CPP(&ModelNameProcessor::FormatData).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<ModelName>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<ModelName>).reset();
}

TEST_F(ModelNameProcessorUTest, TestFormatDataShouldReturnEmpty)
{
    auto processor = ModelNameProcessor(PROF0);
    DataInventory dataInventory;
    OriModelNameDataFormat oriData;
    std::vector<ModelName> formatData;
    StubReserveFailureForVector<std::vector<ModelName>>();
    ASSERT_EQ(processor.FormatData(oriData).size(), formatData.size());
    ResetReserveFailureForVector<std::vector<ModelName>>();
}

TEST_F(ModelNameProcessorUTest, TestRunShouldReturnFalseWhenFileOverMaxSize)
{
    auto processor = ModelNameProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MODEL_NAME));
    MOCKER_CPP(&FileReader::Check).reset();
}

TEST_F(ModelNameProcessorUTest, TestLoadDataShouldReturnEmptyWhenDbRunnerFailed)
{
    auto processor = ModelNameProcessor(PROF0);
    DBInfo modelNameDB("ge_model_info.db", "ModelName");
    std::string dbPath = Utils::File::PathJoin({PROF0, HOST, SQLITE, modelNameDB.dbName});
    OriModelNameDataFormat oriData;

    modelNameDB.dbRunner = nullptr;
    ASSERT_EQ(processor.LoadData(modelNameDB, dbPath).size(), oriData.size());
}
