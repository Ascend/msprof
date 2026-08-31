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
#ifndef ANALYSIS_VIEWER_DATABASE_CONNECTION_H
#define ANALYSIS_VIEWER_DATABASE_CONNECTION_H

#include <sqlite3.h>

#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis
{
namespace Infra
{
using namespace Analysis;
using CHAR_PTR = char *;
const int TIMEOUT = 2147483647;
struct TableColumn
{
    std::string name;
    std::string type;
    bool isPrimary = false;
    TableColumn(const std::string &name, const std::string &type, const bool isPrimary = false)
        : name(name), type(type), isPrimary(isPrimary)
    {
    }
    std::string ToString() const
    {
        std::string base = name + " " + type;
        auto str = isPrimary ? base + " PRIMARY KEY" : base;
        return str;
    }

    bool operator==(const TableColumn &other) const { return (name == other.name) && (type == other.type); }
};

template <size_t... S>
struct IndexSequence
{
};

template <size_t N, size_t... S>
struct IndexSequenceMaker : public IndexSequenceMaker<N - 1, N - 1, S...>
{
};

template <size_t... S>
struct IndexSequenceMaker<0, S...>
{
    using Type = IndexSequence<S...>;
};

template <size_t N>
using MakeIndexSequence = typename IndexSequenceMaker<N>::Type;

// 数据库连接对象，当前版本以sqlite3实现
class Connection
{
   public:
    explicit Connection(const std::string &path);
    ~Connection();
    bool IsDBOpened() const { return db_ != nullptr; }
    bool CheckTableExists(const std::string &tableName);
    bool ExecuteSql(const std::string &sql, const std::string &sqlType);
    bool ExecuteCreateTable(const std::string &sql);
    bool ExecuteCreateIndex(const std::string &sql);
    bool ExecuteDropTable(const std::string &sql);
    template <typename... Args>
    bool ExecuteInsert(const std::string &tableName, const std::vector<std::tuple<Args...>> &data);
    bool ExecuteDelete(const std::string &sql);
    template <typename... Args>
    bool ExecuteQuery(const std::string &sql, std::vector<std::tuple<Args...>> &result);
    bool ExecuteUpdate(const std::string &sql);
    std::vector<TableColumn> ExecuteGetTableColumns(const std::string &tableName);

   private:
    bool InsertCmd(const std::string &tableName, const uint32_t &colNum);
    void FinalizeStmt();
    void BindParameters(int64_t value);
    void BindParameters(uint64_t value);
    void BindParameters(int32_t value);
    void BindParameters(uint32_t value);
    void BindParameters(double value);
    void BindParameters(std::string value);
    void BindParameters(std::nullptr_t value);
    template <typename T, size_t... S>
    void ExecuteInsertHelper(T &row, IndexSequence<S...>);
    template <typename T>
    int ExecuteInsertHelperHelper(T t);
    template <typename T>
    void InsertRow(T &row);

    bool QueryCmd(const std::string &sql);
    void GetColumn(int64_t &value);
    void GetColumn(uint64_t &value);
    void GetColumn(int32_t &value);
    void GetColumn(uint32_t &value);
    void GetColumn(uint16_t &value);
    void GetColumn(double &value);
    void GetColumn(std::string &value);
    template <typename T, size_t... S>
    void ExecuteQueryHelper(T &row, IndexSequence<S...>);
    template <typename T>
    int ExecuteQueryHelperHelper(T &t);
    template <typename T>
    void GetRow(T &row);

   private:
    int index_ = 0;
    sqlite3 *db_ = nullptr;
    sqlite3_stmt *stmt_ = nullptr;
};

template <typename T>
int Connection::ExecuteInsertHelperHelper(T t)
{
    BindParameters(t);
    return 0;
}

template <typename T, size_t... S>
void Connection::ExecuteInsertHelper(T &row, IndexSequence<S...>)
{
    std::initializer_list<int>{(ExecuteInsertHelperHelper(std::get<S>(row)), 0)...};
}

template <typename T>
void Connection::InsertRow(T &row)
{
    using TupleType = typename std::decay<T>::type;
    ExecuteInsertHelper(row, MakeIndexSequence<std::tuple_size<TupleType>::value>{});
}

template <typename T>
int Connection::ExecuteQueryHelperHelper(T &t)
{
    GetColumn(t);
    return 0;
}

template <typename T, size_t... S>
void Connection::ExecuteQueryHelper(T &row, IndexSequence<S...>)
{
    std::initializer_list<int>{(ExecuteQueryHelperHelper(std::get<S>(row)), 0)...};
}

template <typename T>
void Connection::GetRow(T &row)
{
    using TupleType = typename std::decay<T>::type;
    ExecuteQueryHelper(row, MakeIndexSequence<std::tuple_size<TupleType>::value>{});
}

template <typename... Args>
bool Connection::ExecuteInsert(const std::string &tableName, const std::vector<std::tuple<Args...>> &data)
{
    uint32_t colNum = sizeof...(Args);
    // 开启事务
    sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);
    if (!InsertCmd(tableName, colNum))
    {
        return false;
    }
    // 绑定参数
    for (const auto &row : data)
    {
        index_ = 0;
        sqlite3_reset(stmt_);
        InsertRow(row);
        auto rc = sqlite3_step(stmt_);
        if (rc != SQLITE_DONE)
        {
            // 插入失败，回滚事务
            if (sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr))
            {
                ERROR("sqlite3_exec return %: rollback failed!", rc);
            }
            std::string errorMsg = "Failed to insert data: " + std::string(sqlite3_errmsg(db_));
            ERROR("sqlite3_exec return %, %", rc, errorMsg);
            return false;
        }
    }
    // 提交事务
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

template <typename... Args>
bool Connection::ExecuteQuery(const std::string &sql, std::vector<std::tuple<Args...>> &result)
{
    if (!QueryCmd(sql))
    {
        return false;
    }
    while (true)
    {
        auto rc = sqlite3_step(stmt_);
        if (rc != SQLITE_ROW)
        {
            if (rc != SQLITE_DONE)
            {
                std::string errorMsg = "Failed to query data: " + std::string(sqlite3_errmsg(db_));
                ERROR("sqlite3_step return %, %", rc, errorMsg);
                return false;
            }
            break;
        }
        std::tuple<Args...> row;
        index_ = -1;
        GetRow(row);
        result.emplace_back(row);
    }
    return true;
}
}  // namespace Infra
}  // namespace Analysis

#endif  // ANALYSIS_VIEWER_DATABASE_CONNECTION_H
