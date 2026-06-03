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
#include "analysis/csrc/domain/data_process/ai_task/mc2_comm_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string MC2_PATH = "./mc2";
const std::string PROF_PATH = File::PathJoin({MC2_PATH, "PROF_0"});
const std::string DB_SUFFIX = "mc2_comm_info.db";
const std::string TABLE_NAME = "Mc2CommInfo";

using Mc2Format = std::vector<std::tuple<std::string, uint64_t, uint16_t, uint16_t, uint16_t, std::string>>;

Mc2Format DATA{{"11810164099128512732", 16, 0, 0, 27, "50,51,52,53,54,55,56,57"},
               {"1715742220471189745", 8, 0, 0, 29, "70,71,72,73,74,75,76,77"}};
}

class Mc2CommInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(MC2_PATH)) {
            File::RemoveDir(MC2_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(MC2_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST, SQLITE})));
        InitData(File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX}), DATA);
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::RemoveDir(MC2_PATH, DEPTH));
    }

    static void InitData(const std::string &path, Mc2Format &data)
    {
        std::shared_ptr<Mc2CommInfoDB> database;
        MAKE_SHARED0_RETURN_VOID(database, Mc2CommInfoDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, path);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(Mc2CommInfoProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = Mc2CommInfoProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_MC2_COMM_INFO));
    auto res = dataInventory.GetPtr<std::vector<MC2CommInfoData>>();
    EXPECT_EQ(2ul, res->size());
}

TEST_F(Mc2CommInfoProcessorUTest, ShouldReturnFalseWhenSourceTableNotExists)
{
    auto dbPath = File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    DataInventory dataInventory;
    auto processor = Mc2CommInfoProcessor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_MC2_COMM_INFO));
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<MC2CommInfoData>>());
}

TEST_F(Mc2CommInfoProcessorUTest, GetDataShouldReturnFalseWhenDataReserveFail)
{
    DataInventory dataInventory;
    auto processor = Mc2CommInfoProcessor(PROF_PATH);
    Analysis::Test::StubReserveFailureForVector<std::vector<MC2CommInfoData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_MC2_COMM_INFO));
    Analysis::Test::ResetReserveFailureForVector<std::vector<MC2CommInfoData>>();
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<MC2CommInfoData>>());
}

TEST_F(Mc2CommInfoProcessorUTest, TestrUNShouldReturnFalseWhenConstructDBRunnerFail)
{
    DataInventory dataInventory;
    auto processor = Mc2CommInfoProcessor(PROF_PATH);
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_MC2_COMM_INFO));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(Mc2CommInfoProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFail)
{
    DataInventory dataInventory;
    auto processor = Mc2CommInfoProcessor(PROF_PATH);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MC2CommInfoData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_MC2_COMM_INFO));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MC2CommInfoData>).reset();
}
