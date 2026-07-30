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

#include "analysis/csrc/infrastructure/db/include/db_runner.h"

#include <algorithm>
#include <string>

namespace Analysis
{
namespace Infra
{
using namespace Analysis::Utils;
using namespace Analysis;

static std::string GetColumnsString(const std::vector<TableColumn> &cols)
{
    std::vector<std::string> result(cols.size());
    std::transform(cols.begin(), cols.end(), result.begin(), [](const TableColumn &col) { return col.ToString(); });
    return Join(result, ", ");
}

bool DBRunner::CheckTableExists(const std::string &tableName)
{
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    return conn->CheckTableExists(tableName);
}

bool DBRunner::CreateTable(const std::string &tableName, const std::vector<TableColumn> &cols) const
{
    if (tableName.empty())
    {
        ERROR("The tableName is empty string");
        return false;
    }
    std::string valuesStr = GetColumnsString(cols);
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (" + valuesStr + ");";
    INFO("Start create %, sql is %.", tableName, sql);
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    if (!conn->ExecuteCreateTable(sql))
    {
        ERROR("Create % failed", tableName);
        return false;
    }
    INFO("create % success", tableName);
    return true;
}

bool DBRunner::CreateIndex(const std::string &tableName, const std::string &indexName,
                           const std::vector<std::string> &colNames) const
{
    if (tableName.empty())
    {
        ERROR("The tableName is empty string");
        return false;
    }
    std::string valuesStr = Join(colNames, ",");
    std::string sql = "CREATE INDEX IF NOT EXISTS " + indexName + " ON " + tableName + " (" + valuesStr + ");";
    INFO("Start create % index， sql is %.", tableName, sql);
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    if (!conn->ExecuteCreateIndex(sql))
    {
        ERROR("Create % index failed", tableName);
        return false;
    }
    INFO("create % index success", tableName);
    return true;
}

bool DBRunner::DropTable(const std::string &tableName) const
{
    if (tableName.empty())
    {
        ERROR("The tableName is empty string");
        return false;
    }
    INFO("Start drop %", tableName);
    std::string sql = "DROP TABLE " + tableName + ";";
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    if (!conn->ExecuteDropTable(sql))
    {
        ERROR("Drop % failed", tableName);
        return false;
    }
    INFO("Drop % success", tableName);
    return true;
}

bool DBRunner::DeleteData(const std::string &sql) const
{
    INFO("Start delete data");
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    if (!conn->ExecuteDelete(sql))
    {
        ERROR("Delete data failed: %", sql);
        return false;
    }
    INFO("Delete data success");
    return true;
}

bool DBRunner::UpdateData(const std::string &sql) const
{
    INFO("Start update data");
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, path_);
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, path is %d", path_);
        return false;
    }
    if (!conn->ExecuteUpdate(sql))
    {
        ERROR("Update data failed: %", sql);
        return false;
    }
    INFO("Update data success");
    return true;
}

std::vector<TableColumn> DBRunner::GetTableColumns(const std::string &tableName)
{
    INFO("Start get % headers", tableName);
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, {}, path_);
    std::vector<TableColumn> cols;
    if (!conn->IsDBOpened())
    {
        ERROR("Create Connection failed, tableName is %d", tableName);
        return cols;
    }
    cols = conn->ExecuteGetTableColumns(tableName);
    if (cols.empty())
    {
        ERROR("Get % columns failed", tableName);
        return cols;
    }
    INFO("Get % columns success", tableName);
    return cols;
}
}  // namespace Infra
}  // namespace Analysis
