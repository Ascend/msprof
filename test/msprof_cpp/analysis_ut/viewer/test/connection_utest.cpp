/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include <memory>
#include <string>
#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/db/include/connection.h"

using DATA_FORMAT = std::vector<std::tuple<int32_t, uint32_t, int64_t, double, double, std::string>>;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

class ConnectionUtest : public testing::Test {
protected:
    static void TearDownTestCase()
    {
        if (File::Exist("./a.db")) {
            File::DeleteFile("./a.db");
        }
    }
};

TEST_F(ConnectionUtest, Connection)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    EXPECT_NE(nullptr, dbConn->db_);
}

TEST_F(ConnectionUtest, ExecuteCreateIndexWhenIndexExistReturnFalse)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    std::string sql = "CREATE TABLE IF NOT EXISTS tb1 (id INT PRIMARY KEY,name VARCHAR(255),age INT);";
    auto rc = dbConn->ExecuteCreateTable(sql);
    EXPECT_EQ(rc, true);

    sql = "CREATE INDEX IF NOT EXISTS a ON tb1 ( age );";
    rc = dbConn->ExecuteCreateIndex(sql);
    EXPECT_EQ(rc, true);

    sql = "CREATE INDEX IF NOT EXISTS a ON tb2 ( age );";
    rc = dbConn->ExecuteCreateIndex(sql);
    EXPECT_EQ(rc, false);
}

TEST_F(ConnectionUtest, ExecuteCreateTable)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    std::string sql = "CREATE TABLE IF NOT EXISTS tb1 (id INT PRIMARY KEY,name VARCHAR(255),age INT);";
    auto rc = dbConn->ExecuteCreateTable(sql);
    EXPECT_EQ(rc, true);
}

TEST_F(ConnectionUtest, ExecuteDropTable)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    std::string sql = "DROP TABLE tb1;";
    auto rc = dbConn->ExecuteDropTable(sql);
    EXPECT_EQ(rc, true);
}

TEST_F(ConnectionUtest, ExecuteInsert)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    dbConn->ExecuteDropTable("DROP TABLE tb2;");
    std::string sql = "CREATE TABLE IF NOT EXISTS tb2 (c1 INTEGER PRIMARY KEY,c2 INTEGER, c3 INTEGER,"
                      "c4 NUMERIC, c5 REAL, C6 TEXT);";
    dbConn->ExecuteCreateTable(sql);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    auto rc = dbConn->ExecuteInsert("tb2", data);
    EXPECT_EQ(rc, true);
}

TEST_F(ConnectionUtest, ExecuteDelete)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    std::string sql = "CREATE TABLE IF NOT EXISTS tb1 (id INT PRIMARY KEY,name VARCHAR(255),age INT);";
    dbConn->ExecuteCreateTable(sql);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbConn->ExecuteInsert("tb1", data);
    auto rc = dbConn->ExecuteDelete("DELETE FROM tb1 WHERE age>20;");
    EXPECT_EQ(rc, true);
}

TEST_F(ConnectionUtest, ExecuteUpdate)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    std::string sql = "CREATE TABLE IF NOT EXISTS tb1 (id INT PRIMARY KEY,name VARCHAR(255),age INT);";
    dbConn->ExecuteCreateTable(sql);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(3, "Bob", 26),
        std::make_tuple(4, "Tom", 18)
    };
    dbConn->ExecuteInsert("tb1", data);
    auto rc = dbConn->ExecuteUpdate("UPDATE tb1 SET age = 30 WHERE name = 'Tom';");
    EXPECT_EQ(rc, true);
}

TEST_F(ConnectionUtest, ExecuteQuery)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    dbConn->ExecuteDropTable("DROP TABLE tb2;");
    std::string sql = "CREATE TABLE IF NOT EXISTS tb2 (c1 INTEGER PRIMARY KEY,c2 INTEGER, c3 INTEGER,"
                      "c4 NUMERIC, c5 REAL, C6 TEXT);";
    dbConn->ExecuteCreateTable(sql);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    dbConn->ExecuteInsert("tb2", data);
    sql = "SELECT * FROM tb2;";
    DATA_FORMAT result;
    auto rc = dbConn->ExecuteQuery(sql, result);
    EXPECT_EQ(rc, true);
    int32_t num1 = 2147483647;
    uint32_t num2 = 4294967295;
    int64_t num3 = 9223372036854775807;
    for (const auto& tuple : result) {
        int32_t c1 = std::get<0>(tuple);
        uint32_t c2 = std::get<1>(tuple);
        int64_t c3 = std::get<2>(tuple);
        double c4 = std::get<3>(tuple);
        double c5 = std::get<4>(tuple);
        std::string c6 = std::get<5>(tuple);
        EXPECT_EQ(num1, c1);
        EXPECT_EQ(num2, c2);
        EXPECT_EQ(num3, c3);
        EXPECT_EQ("hcom_allGather__516_2", c6);
    }
}

TEST_F(ConnectionUtest, InsertCmd)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    auto rc = dbConn->InsertCmd("tb1", 3);
    EXPECT_EQ(rc, true);
    EXPECT_NE(nullptr, dbConn->stmt_);
}

TEST_F(ConnectionUtest, QueryCmd)
{
    std::string path = "./a.db";
    auto dbConn = std::make_shared<Connection>(path);
    auto rc = dbConn->QueryCmd("SELECT * FROM tb1;");
    EXPECT_EQ(rc, true);
    EXPECT_NE(nullptr, dbConn->stmt_);
}

TEST_F(ConnectionUtest, ConnectionErrorReturn)
{
    // 虚假文件路径，确保构造函数失败, 进入异常分支
    std::string path = "./fake/fake.db";
    auto dbConn = std::make_shared<Connection>(path);
}