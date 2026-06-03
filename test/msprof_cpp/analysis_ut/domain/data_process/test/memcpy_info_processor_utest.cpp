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
#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string MEMCPY_INFO_PATH = "./memcpy_info";
const std::string PROF_PATH = File::PathJoin({MEMCPY_INFO_PATH, "PROF_0"});
const std::string DB_SUFFIX = "runtime.db";
const std::string TABLE_NAME = "MemcpyInfo";
MemcpyInfoFormat DATA{{30, 0, 3, 4294967295, 0, 67108864, 0},
                      {30, 0, 4, 4294967295, 0, 16777216, 0},
                      {30, 0, 10, 4294967295, 0, 22282240, 0}};
}

class MemcpyInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(MEMCPY_INFO_PATH)) {
            File::RemoveDir(MEMCPY_INFO_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(MEMCPY_INFO_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST, SQLITE})));
        InitData(File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX}), DATA);
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::RemoveDir(MEMCPY_INFO_PATH, DEPTH));
    }

    static void InitData(const std::string &path, MemcpyInfoFormat &data)
    {
        std::shared_ptr<RuntimeDB> database;
        MAKE_SHARED0_RETURN_VOID(database, RuntimeDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, path);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(MemcpyInfoProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    auto res = dataInventory.GetPtr<std::vector<MemcpyInfoData>>();
    EXPECT_EQ(3ul, res->size());
}

// 不开--runtime-api开关的时候，没有这个表
TEST_F(MemcpyInfoProcessorUTest, ShouldReturnTrueWhenMemcpyInfoTableNotExists)
{
    auto dbPath = File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<MemcpyInfoData>>());
}

TEST_F(MemcpyInfoProcessorUTest, GetDataShouldReturnFalseWhenDataReserveFail)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    Analysis::Test::StubReserveFailureForVector<std::vector<MemcpyInfoData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    Analysis::Test::ResetReserveFailureForVector<std::vector<MemcpyInfoData>>();
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<MemcpyInfoData>>());
}

TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFail)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathAndTableFail)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnFalseWhenLoadDataFail)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    MemcpyInfoFormat oriData;
    MOCKER_CPP(&MemcpyInfoProcessor::LoadData).stubs().will(returnValue(oriData));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    MOCKER_CPP(&MemcpyInfoProcessor::LoadData).reset();
}

TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFail)
{
    DataInventory dataInventory;
    auto processor = MemcpyInfoProcessor(PROF_PATH);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MemcpyInfoData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEMCPY_INFO));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<MemcpyInfoData>).reset();
}
