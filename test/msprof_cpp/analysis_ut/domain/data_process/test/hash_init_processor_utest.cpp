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
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string HASH_PATH = "./hash";
const std::string PROF_PATH = File::PathJoin({HASH_PATH, "PROF_0"});
const std::string DB_SUFFIX = "ge_hash.db";
const std::string LOGIC_DB_SUFFIX = "ge_logic_stream_info.db";
const std::string TABLE_NAME = "GeHashInfo";
const std::string LOGIC_TABLE_NAME = "GeLogicStreamInfo";
const std::string PROCESS_HASH = "Hash";

using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
using LogicStream = std::vector<std::tuple<uint32_t, uint32_t>>;

GeHashFormat DATA{{"7383439776149831", "Cast"},
                  {"247669290252505", "Add"},
                  {"8477521346829072275", "aclnnMul_MulAiCore_Mul"}};
LogicStream DATA_STREAM{{3, 2}, {4, 2}};
}

class HashInitProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(HASH_PATH)) {
            File::RemoveDir(HASH_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(HASH_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST, SQLITE})));
        CreateHashData(File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX}), DATA);
        CreateLogicStream(File::PathJoin({PROF_PATH, HOST, SQLITE, LOGIC_DB_SUFFIX}), DATA_STREAM);
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::RemoveDir(HASH_PATH, DEPTH));
    }

    static void CreateHashData(const std::string& dbPath, GeHashFormat& data)
    {
        std::shared_ptr<HashDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HashDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }

    static void CreateLogicStream(const std::string& dbPath, LogicStream& data)
    {
        std::shared_ptr<GeLogicStreamDB> database;
        MAKE_SHARED0_RETURN_VOID(database, GeLogicStreamDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(LOGIC_TABLE_NAME);
        dbRunner->CreateTable(LOGIC_TABLE_NAME, cols);
        dbRunner->InsertData(LOGIC_TABLE_NAME, data);
    }
};

TEST_F(HashInitProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(3ul, res->size());
    EXPECT_EQ(2ul, dataInventory.GetPtr<StreamMap>()->size());
}

TEST_F(HashInitProcessorUTest, ShouldReturnFalseWhenSourceTableNotExists)
{
    auto dbPath = File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto logicPath = File::PathJoin({PROF_PATH, HOST, SQLITE, LOGIC_DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, logicPath);
    dbRunner->DropTable(LOGIC_TABLE_NAME);
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(0ul, res->size());
    EXPECT_FALSE(dataInventory.GetPtr<StreamMap>());
}

TEST_F(HashInitProcessorUTest, ShouldReturnFalseWhenFileOverMaxSize)
{
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    MOCKER_CPP(&Analysis::Utils::FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(0ul, res->size());
    EXPECT_FALSE(dataInventory.GetPtr<StreamMap>());
}

TEST_F(HashInitProcessorUTest, ShouldReturnFalseWhenConstructDBRunnerFailed)
{
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.ProcessLogicStream(dataInventory));
    EXPECT_FALSE(processor.ProcessHashMap(dataInventory));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}