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

#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H

#include <thread>
#include <unordered_map>

#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

namespace Analysis
{
namespace Domain
{
using EventData = std::vector<
    std::tuple<std::string, std::string, std::string, uint32_t, std::string, uint64_t, uint64_t, uint32_t, uint64_t>>;

class ApiEventDBDumper final : public BaseDumper<ApiEventDBDumper>
{
    using ApiData = std::vector<std::shared_ptr<Domain::Event>>;

   public:
    explicit ApiEventDBDumper(const std::string &hostFilePath);
    EventData GenerateData(const ApiData &apiTraces);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H
