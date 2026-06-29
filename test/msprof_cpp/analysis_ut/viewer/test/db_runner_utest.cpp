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
#include <memory>
#include <string>
#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

using DATA_FORMAT = std::vector<std::tuple<int32_t, uint32_t, int64_t, uint16_t, double, double, std::string>>;
using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Infra;

class DBRunnerUtest : public testing::Test {
protected:
    static void TearDownTestCase()
    {
        if (File::Exist("./a.db")) {
            File::DeleteFile("./a.db");
        }
    }
};

std::vector<TableColumn> cols = {
    TableColumn("col1", "INTEGER"),
    TableColumn("col2", "INTEGER"),
    TableColumn("col3", "INTEGER"),
    TableColumn("col4", "INTEGER"),
    TableColumn("col5", "NUMERIC"),
    TableColumn("col6", "REAL"),
    TableColumn("col7", "TEXT")
};

TEST_F(DBRunnerUtest, DBRunner)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    EXPECT_EQ(path, dbRunner->path_);
}

TEST_F(DBRunnerUtest, CheckTableExistsShouldReturnTrueWhenCreateTableSuccess)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    const std::string tableName = "tb3";
    auto rc = dbRunner->CreateTable(tableName, cols);
    EXPECT_EQ(rc, true);
    rc = dbRunner->CheckTableExists(tableName);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, CheckTableExistsShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    const std::string tableName = "tb3";
    auto rc = dbRunner->CreateTable(tableName, cols);
    EXPECT_EQ(rc, true);
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    rc = dbRunner->CheckTableExists(tableName);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, CreateTable)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, CreateTableShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, CreateIndexWhenNoThisColThenReturnFalse)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, true);
    rc = dbRunner->CreateIndex("tb3", "a", {"a"});
    EXPECT_EQ(rc, false);
}

TEST_F(DBRunnerUtest, CreateIndexShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, true);

    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    rc = dbRunner->CreateIndex("tb3", "a", {"a"});
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, CreateIndexWhenCreateSuccessThenReturnTrue)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, true);
    rc = dbRunner->CreateIndex("tb3", "a", {"age"});
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, DropTableShouldReturnTrueWhenDropTableSuccess)
{
    std::string path = "./a.db";
    const std::string tableName = "tb4";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable(tableName, cols);
    auto rc = dbRunner->DropTable(tableName);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, DropTableShouldReturnFalseWhenDropTableExecFailed)
{
    std::string path = "./a.db";
    const std::string tableName = "tb4";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable(tableName, cols);
    // tableName为空字符串
    auto rc = dbRunner->DropTable("");
    EXPECT_EQ(rc, false);

    // connection初始化异常
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    rc = dbRunner->DropTable(tableName);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();

    // dropTable执行失败
    MOCKER_CPP(&Connection::ExecuteDropTable).stubs().will(returnValue(false));
    rc = dbRunner->DropTable(tableName);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::ExecuteDropTable).reset();
}

TEST_F(DBRunnerUtest, InsertData)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 100, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    auto rc = dbRunner->InsertData("tb4", data);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, InsertDataShouldReturnFasleWhenDropTableExecFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 100, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto rc = dbRunner->InsertData("tb4", data);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, DeleteData)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);
    auto rc = dbRunner->DeleteData("DELETE FROM tb3 WHERE age>20;");
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, DeleteDataShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);

    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto rc = dbRunner->DeleteData("DELETE FROM tb3 WHERE age>20;");
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, QueryData)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    dbRunner->DeleteData("DELETE FROM tb4;");
    DATA_FORMAT data = {
        std::make_tuple(7, 8, 9, 10, 1320.4, 56.5, "hcom_allGather__516_1")
    };
    dbRunner->InsertData("tb4", data);
    std::string sql = "SELECT * FROM tb4;";
    DATA_FORMAT result;
    auto rc = dbRunner->QueryData(sql, result);
    EXPECT_EQ(rc, true);
    int32_t num1 = 7;
    uint32_t num2 = 8;
    int64_t num3 = 9;
    uint16_t num4 = 10;
    for (const auto& row : result) {
        int32_t c1 = std::get<0>(row);
        uint32_t c2 = std::get<1>(row);
        int64_t c3 = std::get<2>(row);
        uint16_t c4 = std::get<3>(row);
        std::string c6 = std::get<6>(row);
        EXPECT_EQ(num1, c1);
        EXPECT_EQ(num2, c2);
        EXPECT_EQ(num3, c3);
        EXPECT_EQ(num4, c4);
        EXPECT_EQ("hcom_allGather__516_1", c6);
    }
}

TEST_F(DBRunnerUtest, QueryDataShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    dbRunner->DeleteData("DELETE FROM tb4;");
    DATA_FORMAT data = {
        std::make_tuple(7, 8, 9, 10, 1320.4, 56.5, "hcom_allGather__516_1")
    };
    dbRunner->InsertData("tb4", data);
    std::string sql = "SELECT * FROM tb4;";
    DATA_FORMAT result;
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto rc = dbRunner->QueryData(sql, result);
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, UpdateData)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);
    auto rc = dbRunner->UpdateData("UPDATE tb3 SET age = 30 WHERE name = 'Tom';");
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, UpdateDataShouldReturnFalseWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto rc = dbRunner->UpdateData("UPDATE tb3 SET age = 30 WHERE name = 'Tom';");
    EXPECT_EQ(rc, false);
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

bool isEqual(const std::vector<TableColumn>& vec1, const std::vector<TableColumn>& vec2)
{
    if (vec1.size() != vec2.size()) {
        return false;
    }

    for (size_t i = 0; i < vec1.size(); i++) {
        if (vec1[i].name != vec2[i].name || vec1[i].type != vec2[i].type) {
            return false;
        }
    }
    return true;
}

TEST_F(DBRunnerUtest, GetTableColumns)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 100, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    dbRunner->InsertData("tb4", data);
    std::vector<TableColumn> expectedTableColumns{
        {"col1", "INTEGER"},
        {"col2", "INTEGER"},
        {"col3", "INTEGER"},
        {"col4", "INTEGER"},
        {"col5", "NUMERIC"},
        {"col6", "REAL"},
        {"col7", "TEXT"},
    };
    auto tableColumns = dbRunner->GetTableColumns("tb4");
    EXPECT_TRUE(isEqual(tableColumns, expectedTableColumns));
}

TEST_F(DBRunnerUtest, GetTableColumnsShouldReturnEmptyVectorWhenCreateConnectionFailed)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 100, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    dbRunner->InsertData("tb4", data);
    std::vector<TableColumn> expectedTableColumns{};
    MOCKER_CPP(&Connection::IsDBOpened).stubs().will(returnValue(false));
    auto tableColumns = dbRunner->GetTableColumns("tb4");
    EXPECT_TRUE(isEqual(tableColumns, expectedTableColumns));
    MOCKER_CPP(&Connection::IsDBOpened).reset();
}

TEST_F(DBRunnerUtest, MultithreadingInsertQuery)
{
    std::string path = "./a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->DropTable("tb4");
    dbRunner->CreateTable("tb4", cols);
    dbRunner->CreateTable("tb5", cols);
    const int threadsNum = 5;
    const int rowNum = 1000;
    DATA_FORMAT data;
    auto row = std::make_tuple(2147483647, 4294967295, 9223372036854775807, 100, 118972596256332.95,
                               1340.046875, "hcom_allGather__516_2");
    for (int i = 0; i < rowNum; i++) {
        data.emplace_back(row);
    }
    ThreadPool pool(threadsNum);
    pool.Start();
    auto task1 = [dbRunner, &data]() {
        dbRunner->InsertData("tb4", data);
    };
    auto task2 = [dbRunner, &data]() {
        dbRunner->InsertData("tb5", data);
    };
    pool.AddTask(task1);
    pool.AddTask(task2);
    pool.WaitAllTasks();
    pool.Stop();
    std::string sql1 = "SELECT * FROM tb4;";
    std::string sql2 = "SELECT * FROM tb5;";
    DATA_FORMAT resultTb4;
    DATA_FORMAT resultTb5;
    ThreadPool pool_query(threadsNum);
    pool_query.Start();
    auto task3 = [dbRunner, &resultTb4, sql1]() {
        dbRunner->QueryData(sql1, resultTb4);
    };
    auto task4 = [dbRunner, &resultTb5, sql2]() {
        dbRunner->QueryData(sql2, resultTb5);
    };
    pool_query.AddTask(task3);
    pool_query.AddTask(task4);
    pool_query.WaitAllTasks();
    pool_query.Stop();
    EXPECT_EQ(rowNum, resultTb4.size());
    EXPECT_EQ(rowNum, resultTb5.size());
}