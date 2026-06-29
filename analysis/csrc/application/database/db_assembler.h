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

#ifndef ANALYSIS_APPLICATION_DB_ASSEMBLER_H
#define ANALYSIS_APPLICATION_DB_ASSEMBLER_H

#include "analysis/csrc/application/database/msprof_db.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/db/include/db_info.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Infra;

class DBAssembler
{
   public:
    DBAssembler() = default;
    DBAssembler(const std::string &profPath, const std::string &outputPath);
    virtual ~DBAssembler() = default;
    bool Run(DataInventory &dataInventory);
    const static std::set<std::string> &GetProcessList();

   private:
    std::string profPath_;
    std::string outputPath_;
    DBInfo msprofDB_;
};

template <typename... Args>
bool SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName, DBInfo &msprofDB)
{
    INFO("DBAssembler Save % Data.", tableName);
    if (data.empty())
    {
        ERROR("% is empty.", tableName);
        return false;
    }
    if (msprofDB.database == nullptr)
    {
        ERROR("Msprof db database is nullptr.");
        return false;
    }
    if (msprofDB.dbRunner == nullptr)
    {
        ERROR("Msprof db runner is nullptr.");
        return false;
    }
    if (!msprofDB.dbRunner->CreateTable(tableName, msprofDB.database->GetTableCols(tableName)))
    {
        ERROR("Create table: % failed", tableName);
        return false;
    }
    if (!msprofDB.dbRunner->InsertData(tableName, data))
    {
        ERROR("Insert data into % failed", tableName);
        return false;
    }
    return true;
}

}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_DB_ASSEMBLER_H
