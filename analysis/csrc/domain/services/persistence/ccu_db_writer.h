/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_CCU_DB_WRITER_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_CCU_DB_WRITER_H

#include "analysis/csrc/infrastructure/db/include/db_runner.h"

namespace Analysis
{
namespace Domain
{
// Failed imports require the existing full-clean workflow. Verify committed rows because
// the shared insertion API does not report COMMIT errors.
template <typename... Args>
bool InsertCcuRowsOnce(const Infra::DBRunner& runner, const std::string& table,
                       const std::vector<std::tuple<Args...>>& rows)
{
    std::vector<std::tuple<uint64_t>> count;
    const std::string query = "SELECT COUNT(*) FROM " + table;
    if (!runner.QueryData(query, count) || count.size() != 1 || std::get<0>(count[0]) != 0)
    {
        ERROR("CCU table % is not empty or unreadable; clear parsed output and reimport the full capture", table);
        return false;
    }
    if (rows.empty())
    {
        return true;
    }
    if (!runner.InsertData(table, rows))
    {
        return false;
    }
    count.clear();
    return runner.QueryData(query, count) && count.size() == 1 && std::get<0>(count[0]) == rows.size();
}
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_CCU_DB_WRITER_H
