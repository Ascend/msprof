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

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Application;
using namespace Analysis::Domain;
using HashData = std::unordered_map<uint64_t, std::string>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t HASH_TABLE_SIZE = 4;
class HashDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }
};

TEST_F(HashDBDumperUtest, TestDumpHashDataWithCompletedHashDataShouldInsertSuccessAndCanBeQueried)
{
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    EXPECT_TRUE(res);
    HashDB hashDb;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hashDb.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string>> data;
    runtimeDBRunner.QueryData("select * from GeHashInfo", data);
    EXPECT_EQ(data.size(), HASH_TABLE_SIZE);
}


TEST_F(HashDBDumperUtest, TestHashDBDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    ASSERT_FALSE(res);
}
TEST_F(HashDBDumperUtest, TestHashDBDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    ASSERT_FALSE(res);
}