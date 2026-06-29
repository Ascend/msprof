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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Utils;
struct DBInfo
{
    std::string dbName;
    std::string tableName;
    std::shared_ptr<Database> database;
    std::shared_ptr<DBRunner> dbRunner;
    DBInfo() = default;
    DBInfo(std::string dbName, std::string tableName) : dbName(std::move(dbName)), tableName(std::move(tableName)) {};
    virtual ~DBInfo() = default;
};

template <typename... Args>
bool SaveData(const std::vector<std::tuple<Args...>>& data, DBInfo& dbInfo, std::string& dbPath)
{
    INFO("Processor Save % Data.", dbInfo.tableName);
    if (data.empty())
    {
        ERROR("% is empty.", dbInfo.tableName);
        return false;
    }
    if (dbInfo.database == nullptr)
    {
        ERROR("Msprof db database is nullptr.");
        return false;
    }
    if (dbInfo.dbRunner == nullptr)
    {
        ERROR("Msprof db runner is nullptr.");
        return false;
    }
    if (!dbInfo.dbRunner->CreateTable(dbInfo.tableName, dbInfo.database->GetTableCols(dbInfo.tableName)))
    {
        ERROR("Create table: % failed", dbInfo.tableName);
        return false;
    }
    if (!dbInfo.dbRunner->InsertData(dbInfo.tableName, data))
    {
        ERROR("Insert data into % failed", dbPath);
        return false;
    }
    return true;
}

SyscntConversionParams GenerateSyscntConversionParams(const DeviceContext& context);
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H
