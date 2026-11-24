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

#include "analysis/csrc/domain/services/persistence/device/l2_cache_persistence.h"

#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/calculator/l2_cache/l2_cache_task_calculator.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/l2_cache/include/l2_cache_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
std::string JoinEventLists(const HalL2CacheRaw &row)
{
    std::vector<std::string> counts;
    counts.reserve(L2_CACHE_EVENT_SLOT_COUNT);
    for (uint32_t slot = 0; slot < L2_CACHE_EVENT_SLOT_COUNT; ++slot)
    {
        counts.emplace_back(std::to_string(row.events[slot]));
    }
    return Utils::Join(counts, ",");
}
}  // namespace

uint32_t L2CachePersistence::ProcessEntry(Infra::DataInventory &inventory, const Infra::Context &context)
{
    const auto &device = static_cast<const DeviceContext &>(context);
    const auto dbPath = Utils::File::PathJoin({device.GetDeviceFilePath(), SQLITE, "l2cache.db"});
    const auto parsed = inventory.GetPtr<std::vector<HalL2CacheRaw>>();
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    if ((!parsed || parsed->empty()) && (!summary || summary->empty()))
    {
        return ANALYSIS_OK;
    }
    std::shared_ptr<Infra::Database> database;
    MAKE_SHARED0_RETURN_VALUE(database, Infra::L2CacheDB, ANALYSIS_ERROR);
    std::shared_ptr<Infra::DBRunner> runner;
    MAKE_SHARED_RETURN_VALUE(runner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    DBInfo info;
    info.dbName = "l2cache.db";
    info.database = database;
    info.dbRunner = runner;
    std::string path = dbPath;
    if (parsed && !parsed->empty())
    {
        std::vector<L2CacheParseDbRow> rows;
        rows.reserve(parsed->size());
        for (const auto &row : *parsed)
        {
            rows.emplace_back(row.taskType, row.streamId, row.taskId, JoinEventLists(row));
        }
        info.tableName = "L2CacheParse";
        if (!SaveData(rows, info, path))
        {
            ERROR("Persist L2CacheParse data failed.");
            return ANALYSIS_ERROR;
        }
    }
    if (summary && !summary->empty())
    {
        std::vector<L2CacheSummaryDbRow> rows;
        rows.reserve(summary->size());
        for (const auto &row : *summary)
        {
            rows.emplace_back(row.taskType, row.streamId, row.taskId, row.hitRate, row.victimRate);
        }
        info.tableName = "L2CacheSummary";
        if (!SaveData(rows, info, path))
        {
            ERROR("Persist L2CacheSummary data failed.");
            return ANALYSIS_ERROR;
        }
    }
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(L2CachePersistence, true, L2CacheParser, L2CacheTaskCalculator);
// REGISTER_PROCESS_DEPENDENT_DATA(L2CachePersistence, std::vector<HalL2CacheRaw>, std::vector<L2CacheSummaryRow>);
// REGISTER_PROCESS_SUPPORT_CHIP(L2CachePersistence, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0, CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0, CHIP_V6_1_1);
}  // namespace Domain
}  // namespace Analysis
