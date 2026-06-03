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
#include "analysis/csrc/domain/data_process/ai_task/fusion_op_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"
#include "analysis/csrc/domain/data_process/data_processor.h";
#include "analysis/csrc/infrastructure/db/include/db_runner.h";

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;

const std::string API_DIR = "./fusion_op";
const std::string PROF0 = File::PathJoin({API_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({API_DIR, "PROF_1"});
const std::string TABLE_NAME = "GeFusionOpInfo";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1};

const OriFusionOpDataFormat FUSION_OP_DATA = {
    {1, "FusedBatchNormV3_BNInferenceD", 1, "FusedBatchNormV3", "16386.09375", "16384.03125", "0", "0", "32770.125"},
    {1, "SigmoidMul_1SubMul_2Add", 5, "Sigmoid;Mul_1;Sub;Mul_2;Add", "49152.15625", \
        "16384.03125", "0", "0", "65536.1875"},
    {1, "fp32_vars/addfp32_vars/Relu_3", 2, "fp32_vars/add;fp32_vars/Relu_3", \
        "3136.0625", "1568.03125", "0", "0", "4704.09375"},
};

class FusionOpProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(API_DIR)) {
            File::RemoveDir(API_DIR, 0);
        }
        EXPECT_TRUE(File::CreateDir(API_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF0, HOST, SQLITE}), FUSION_OP_DATA));
        EXPECT_TRUE(File::CreateDir(PROF1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF1, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF1, HOST, SQLITE}), FUSION_OP_DATA));
    }

    static bool CreateAPIDB(const std::string& sqlitePath, OriFusionOpDataFormat data)
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

TEST_F(FusionOpProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<DataInventory> res(PROF_PATHS.size());
    size_t i = 0;
    for (const auto& profPath : PROF_PATHS) {
        auto processor = FusionOpProcessor(profPath);
        EXPECT_TRUE(processor.Run(res[i], PROCESSOR_NAME_FUSION_OP));
        ++i;
    }
    for (auto& node : res) {
        auto checkData = node.GetPtr<std::vector<FusionOpInfo>>();
        EXPECT_EQ(FUSION_OP_DATA.size(), checkData->size());
        node.RemoveRestData({});
    }
}

TEST_F(FusionOpProcessorUTest, TestProcessShouldReturnFalseWhenEachStepFailed)
{
    auto processor = FusionOpProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();

    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
    .stubs()
    .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    OriFusionOpDataFormat emptyFusionOpData;
    MOCKER_CPP(&FusionOpProcessor::LoadData)
    .stubs()
    .will(returnValue(emptyFusionOpData));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&FusionOpProcessor::LoadData).reset();

    std::vector<FusionOpInfo> emptyData;
    MOCKER_CPP(&FusionOpProcessor::FormatData)
    .stubs()
    .will(returnValue(emptyData));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&FusionOpProcessor::FormatData).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<FusionOpInfo>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Process(dataInventory));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<FusionOpInfo>).reset();
}

TEST_F(FusionOpProcessorUTest, TestFormatDataShouldReturnEmpty)
{
    auto processor = FusionOpProcessor(PROF0);
    OriFusionOpDataFormat oriData;
    std::vector<FusionOpInfo> formatData;
    StubReserveFailureForVector<std::vector<FusionOpInfo>>();
    ASSERT_EQ(processor.FormatData(oriData).size(), formatData.size());
    ResetReserveFailureForVector<std::vector<FusionOpInfo>>();
}

TEST_F(FusionOpProcessorUTest, TestRunShouldReturnFalseWhenFileOverMaxSize)
{
    auto processor = FusionOpProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_FUSION_OP));
    MOCKER_CPP(&FileReader::Check).reset();
}

TEST_F(FusionOpProcessorUTest, TestLoadDataShouldReturnEmptyWhenDBRunnerIsNULL)
{
    auto processor = FusionOpProcessor(PROF0);
    DBInfo fusionOpDB("ge_model_info.db", "GeFusionOpInfo");
    std::string dbPath = Utils::File::PathJoin({PROF0, HOST, SQLITE, fusionOpDB.dbName});
    OriFusionOpDataFormat oriData;

    fusionOpDB.dbRunner = nullptr;
    ASSERT_EQ(processor.LoadData(fusionOpDB, dbPath).size(), oriData.size());
}
