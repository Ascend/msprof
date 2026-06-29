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
#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H

#include <vector>

#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
template <typename DumperType>
class BaseDumper
{
   public:
    template <typename DataType>
    bool DumpData(const DataType &input)
    {
        if (!database_)
        {
            ERROR("Empty pointer: database_");
            return false;
        }
        if (input.empty())
        {
            WARN("Input data is empty, pass dump");
            return true;
        }
        MAKE_SHARED_RETURN_VALUE(dbRunner_, DBRunner, false, Utils::File::PathJoin({dbPath_, database_->GetDBName()}));
        if (!dbRunner_->CreateTable(tableName_, database_->GetTableCols(tableName_)))
        {
            ERROR("Create table: % failed", tableName_);
            return false;
        }
        auto data = static_cast<DumperType *>(this)->GenerateData(input);
        if (data.empty())
        {
            ERROR("Get Empty data");
            return false;
        }
        if (dbRunner_->template InsertData(tableName_, data))
        {
            INFO("Dump % data success", tableName_);
            return true;
        }
        ERROR("Dump table: % failed", tableName_);
        return false;
    };

    BaseDumper(const std::string &hostPath, std::string tableName) : tableName_(std::move(tableName))
    {
        dbPath_ = Utils::File::PathJoin({hostPath, Analysis::Common::SQLITE});
    };

   protected:
    std::shared_ptr<Database> database_;
    std::string dbPath_;
    std::shared_ptr<DBRunner> dbRunner_;
    std::string tableName_;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H
