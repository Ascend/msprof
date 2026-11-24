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

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/calculator/l2_cache/l2_cache_task_calculator.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/l2_cache_persistence.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string ROOT_DIR = "./l2_cache_persistence_utest";
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});
const std::string DB_PATH = Utils::File::PathJoin({SQLITE_DIR, "l2cache.db"});

using SummaryTuple = std::tuple<uint32_t, uint32_t, uint32_t, double, double>;

HalL2CacheRaw MakeRaw(uint16_t taskType, uint16_t streamId, uint16_t taskId, const std::vector<uint64_t> &events)
{
    HalL2CacheRaw row{};
    row.taskType = taskType;
    row.streamId = streamId;
    row.taskId = taskId;
    for (size_t i = 0; i < events.size() && i < L2_CACHE_EVENT_SLOT_COUNT; ++i)
    {
        row.events[i] = events[i];
    }
    return row;
}

class L2CachePersistenceUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    DeviceContext CreateContext() const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        return context;
    }
};
}  // namespace

TEST_F(L2CachePersistenceUTest, InsertsParseAndSummaryFromHalWithoutRecomputingRates)
{
    Infra::DataInventory inventory;
    auto parseRows = std::make_shared<std::vector<HalL2CacheRaw>>();
    parseRows->emplace_back(MakeRaw(1, 2, 3, {10, 20, 30, 40, 50, 60, 70, 80}));
    ASSERT_TRUE(inventory.Inject(parseRows));

    auto summaryRows = std::make_shared<std::vector<L2CacheSummaryRow>>();
    L2CacheSummaryRow summary;
    summary.taskType = 1;
    summary.streamId = 2;
    summary.taskId = 3;
    summary.hitRate = 0.333333;
    summary.victimRate = 0.0;
    summaryRows->emplace_back(summary);
    ASSERT_TRUE(inventory.Inject(summaryRows));

    L2CachePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));

    Infra::DBRunner runner(DB_PATH);
    std::vector<L2CacheParseDbRow> parseDb;
    ASSERT_TRUE(runner.QueryData("SELECT task_type, stream_id, task_id, event_lists FROM L2CacheParse", parseDb));
    ASSERT_EQ(1UL, parseDb.size());
    EXPECT_EQ("10,20,30,40,50,60,70,80", std::get<3>(parseDb[0]));

    std::vector<L2CacheSummaryDbRow> summaryDb;
    ASSERT_TRUE(
        runner.QueryData("SELECT task_type, stream_id, task_id, hit_rate, victim_rate FROM L2CacheSummary", summaryDb));
    ASSERT_EQ(1UL, summaryDb.size());
    EXPECT_DOUBLE_EQ(0.333333, std::get<3>(summaryDb[0]));
    EXPECT_DOUBLE_EQ(0.0, std::get<4>(summaryDb[0]));
}

TEST_F(L2CachePersistenceUTest, EmptyInventoryReturnsOk)
{
    Infra::DataInventory inventory;
    L2CachePersistence persistence;
    EXPECT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));
    EXPECT_FALSE(Utils::File::Exist(DB_PATH));
}

TEST_F(L2CachePersistenceUTest, WritesCurrentBatchParseRows)
{
    Infra::DataInventory inventory;
    auto parseRows = std::make_shared<std::vector<HalL2CacheRaw>>();
    parseRows->emplace_back(MakeRaw(0, 2, 3, {300, 100, 120, 60, 40, 0, 0, 0}));
    parseRows->emplace_back(MakeRaw(0, 2, 4, {3, 0, 1, 0, 0, 0, 0, 0}));
    ASSERT_TRUE(inventory.Inject(parseRows));

    L2CachePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));

    Infra::DBRunner runner(DB_PATH);
    std::vector<L2CacheParseDbRow> parseDb;
    ASSERT_TRUE(runner.QueryData("SELECT task_type, stream_id, task_id, event_lists FROM L2CacheParse", parseDb));
    ASSERT_EQ(2UL, parseDb.size());
    EXPECT_EQ(3U, std::get<2>(parseDb[0]));
    EXPECT_EQ(4U, std::get<2>(parseDb[1]));
}

TEST_F(L2CachePersistenceUTest, SummaryOnlyWritesSummaryTable)
{
    Infra::DataInventory inventory;
    auto summaryRows = std::make_shared<std::vector<L2CacheSummaryRow>>();
    L2CacheSummaryRow summary;
    summary.taskType = 0;
    summary.streamId = 2;
    summary.taskId = 3;
    summary.hitRate = 0.45;
    summary.victimRate = 0.1;
    summaryRows->emplace_back(summary);
    ASSERT_TRUE(inventory.Inject(summaryRows));

    L2CachePersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));

    Infra::DBRunner runner(DB_PATH);
    std::vector<SummaryTuple> rows;
    ASSERT_TRUE(
        runner.QueryData("SELECT task_type, stream_id, task_id, hit_rate, victim_rate FROM L2CacheSummary", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_DOUBLE_EQ(0.45, std::get<3>(rows[0]));
    std::vector<std::tuple<std::string>> tables;
    ASSERT_TRUE(runner.QueryData("SELECT name FROM sqlite_master WHERE type='table' AND name='L2CacheParse'", tables));
    EXPECT_TRUE(tables.empty());
}
}  // namespace Domain
}  // namespace Analysis
