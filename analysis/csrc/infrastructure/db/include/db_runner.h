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

#ifndef ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H
#define ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/db/include/connection.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Infra
{
using namespace Analysis;
// 该类用于定义数据库模块的对外接口
// 主要包括以下特性：
// 1. 提供统一的增删改查接口
// 2. 屏蔽db选型
class DBRunner
{
   public:
    explicit DBRunner(const std::string &dbPath) : path_(dbPath) {};
    bool CheckTableExists(const std::string &tableName);
    bool CreateTable(const std::string &tableName, const std::vector<TableColumn> &cols) const;
    bool CreateTableWithConstraints(const std::string &tableName, const std::vector<TableColumn> &cols,
                                    const std::vector<std::string> &constraints) const;
    bool CreateIndex(const std::string &tableName, const std::string &indexName,
                     const std::vector<std::string> &colNames) const;
    bool DropTable(const std::string &tableName) const;
    // 数据插入接口，支持不同类型数据的插入
    template <typename... Args>
    bool InsertData(const std::string &tableName, const std::vector<std::tuple<Args...>> &data) const;
    bool DeleteData(const std::string &sql) const;
    template <typename... Args>
    bool QueryData(const std::string &sql, std::vector<std::tuple<Args...>> &result) const;
    bool UpdateData(const std::string &sql) const;
    std::vector<TableColumn> GetTableColumns(const std::string &tableName);

   private:
    std::string path_;
};

template <typename... Args>
bool DBRunner::InsertData(const std::string &tableName, const std::vector<std::tuple<Args...>> &data) const
{
    if (tableName.empty())
    {
        ERROR("The tableName is empty string");
        return false;
    }
    INFO("Start insert data to %", tableName);
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create connection failed, path is %", path_);
        return false;
    }
    if (!conn->ExecuteInsert(tableName, data))
    {
        ERROR("Insert data to % failed", tableName);
        return false;
    }
    INFO("insert data to % success", tableName);
    return true;
}

template <typename... Args>
bool DBRunner::QueryData(const std::string &sql, std::vector<std::tuple<Args...>> &result) const
{
    INFO("Start query data");
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create connection failed, path is %", path_);
        return false;
    }
    if (!conn->ExecuteQuery(sql, result))
    {
        ERROR("Query data failed: %", sql);
        return false;
    }
    INFO("Query data success: %", sql);
    return true;
}
}  // namespace Infra
}  // namespace Analysis

#endif  // ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H
