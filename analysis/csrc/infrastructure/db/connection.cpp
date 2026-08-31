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

#include "analysis/csrc/infrastructure/db/include/connection.h"

#include <sqlite3.h>

#include <stdexcept>

#include "analysis/csrc/infrastructure/utils//utils.h"

namespace Analysis
{
namespace Infra
{
using namespace Analysis::Utils;
using namespace Analysis;

namespace
{
const std::string CREATE_TABLE = "create table";
const std::string CREATE_INDEX = "create index";
const std::string DROP_TABLE = "drop table";
const std::string DELETE = "delete";
const std::string UPDATE = "update";
const std::string CHECK = "check";
}  // namespace

Connection::Connection(const std::string &path)
{
    int rc = sqlite3_open(path.c_str(), &db_);  // 连接数据库
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    else
    {
        sqlite3_exec(db_, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);
    }
}

Connection::~Connection()
{
    // 先释放sqlite3_stmt,再释放sqlite3
    FinalizeStmt();
    if (db_)
    {
        int rc = sqlite3_close(db_);
        if (rc != SQLITE_OK)
        {
            ERROR("First close failed, err: % !", sqlite3_errmsg(db_));
            sqlite3_close_v2(db_);
        }
        db_ = nullptr;
    }
}

bool Connection::ExecuteSql(const std::string &sql, const std::string &sqlType)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = "Failed to " + sqlType + ": " + std::string(errMsg);
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::CheckTableExists(const std::string &tableName)
{
    std::string sql{"SELECT 1 FROM sqlite_master WHERE type='table' AND name='" + tableName + "' LIMIT 1"};
    std::vector<std::tuple<int32_t>> result;
    return ExecuteQuery(sql, result) && !result.empty();
}

bool Connection::ExecuteCreateTable(const std::string &sql) { return ExecuteSql(sql, CREATE_TABLE); }

bool Connection::ExecuteCreateIndex(const std::string &sql) { return ExecuteSql(sql, CREATE_INDEX); }

bool Connection::ExecuteDropTable(const std::string &sql) { return ExecuteSql(sql, DROP_TABLE); }

bool Connection::ExecuteDelete(const std::string &sql) { return ExecuteSql(sql, DELETE); }

bool Connection::ExecuteUpdate(const std::string &sql) { return ExecuteSql(sql, UPDATE); }

std::vector<TableColumn> Connection::ExecuteGetTableColumns(const std::string &tableName)
{
    std::vector<TableColumn> cols;
    std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_busy_timeout(db_, TIMEOUT);
    // prepare前需要保证stmt是nullptr
    FinalizeStmt();
    bool rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = std::string(sqlite3_errmsg(db_));
        ERROR("% failed", sql);
        return cols;
    }
    while (sqlite3_step(stmt_) == SQLITE_ROW)
    {
        std::string name;
        std::string type;
        GetColumn(name);
        GetColumn(type);
        index_ = 0;
        cols.emplace_back(TableColumn(name, type));
    }
    return cols;
}

bool Connection::InsertCmd(const std::string &tableName, const uint32_t &colNum)
{
    std::string valueStr;
    for (uint32_t i = 0; i < colNum - 1; ++i)
    {
        valueStr += "?,";
    }
    valueStr = "(" + valueStr + "?)";
    std::string sql = "INSERT INTO " + tableName + " VALUES" + valueStr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    // prepare前需要保证stmt是nullptr
    FinalizeStmt();
    bool rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return false;
    }
    return true;
}

bool Connection::QueryCmd(const std::string &sql)
{
    // prepare前需要保证stmt是nullptr
    FinalizeStmt();
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string errorMsg = "Failed to bind parameter: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return false;
    }
    return true;
}

void Connection::FinalizeStmt()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

void Connection::BindParameters(int64_t value) { sqlite3_bind_int64(stmt_, ++index_, value); }

void Connection::BindParameters(uint64_t value) { sqlite3_bind_int64(stmt_, ++index_, value); }

void Connection::BindParameters(int32_t value) { sqlite3_bind_int(stmt_, ++index_, value); }

void Connection::BindParameters(uint32_t value) { sqlite3_bind_int64(stmt_, ++index_, value); }

void Connection::BindParameters(double value) { sqlite3_bind_double(stmt_, ++index_, value); }

void Connection::BindParameters(std::nullptr_t) { sqlite3_bind_null(stmt_, ++index_); }

void Connection::BindParameters(std::string value)
{
    sqlite3_bind_text(stmt_, ++index_, value.c_str(), -1, SQLITE_TRANSIENT);
}

void Connection::GetColumn(int64_t &value) { value = sqlite3_column_int64(stmt_, ++index_); }

void Connection::GetColumn(uint64_t &value) { value = static_cast<uint64_t>(sqlite3_column_int64(stmt_, ++index_)); }

void Connection::GetColumn(int32_t &value) { value = sqlite3_column_int(stmt_, ++index_); }

void Connection::GetColumn(uint32_t &value) { value = static_cast<uint32_t>(sqlite3_column_int64(stmt_, ++index_)); }

void Connection::GetColumn(double &value) { value = sqlite3_column_double(stmt_, ++index_); }

void Connection::GetColumn(std::string &value)
{
    auto valueStr = const_cast<unsigned char *>(sqlite3_column_text(stmt_, ++index_));
    value = std::string(ReinterpretConvert<CHAR_PTR>(valueStr));
}

void Connection::GetColumn(uint16_t &value) { value = sqlite3_column_int(stmt_, ++index_); }
}  // namespace Infra
}  // namespace Analysis
