/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "gtest/gtest.h"

#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/system/host_platform_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_platform_data.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string DATA_DIR = "./host_platform_processor";
const std::string PROF_DIR = File::PathJoin({DATA_DIR, "PROF"});
const std::string CHILD_PROF_DIR = File::PathJoin({PROF_DIR, "PROF_0"});
const std::string SQLITE_DIR = File::PathJoin({CHILD_PROF_DIR, "host", "sqlite"});
const std::string PLATFORM_DB = File::PathJoin({SQLITE_DIR, "platform.db"});
const std::string THREAD_DB = File::PathJoin({SQLITE_DIR, "thread.db"});

void CreateNumaDatabase()
{
    DBRunner db(PLATFORM_DB);
    ASSERT_TRUE(db.CreateTable("p_levels_hierarchy_names", {{"id", "INTEGER", true}, {"title0_id", "INTEGER"},
                                                             {"title1_id", "INTEGER"}, {"title2_id", "INTEGER"}}));
    ASSERT_TRUE(db.CreateTable("p_metrics", {{"id", "INTEGER", true}, {"ts", "INTEGER"}, {"value", "REAL"},
                                               {"levels_id", "INTEGER"}}));
    ASSERT_TRUE(db.CreateTable("p_scaling_values", {{"id", "INTEGER", true}, {"level_id", "INTEGER"},
                                                      {"max_value", "REAL"}}));
    ASSERT_TRUE(db.CreateTable("p_titles_names", {{"id", "INTEGER", true}, {"name", "TEXT"},
                                                    {"description", "TEXT"}, {"summary_flag", "INTEGER"},
                                                    {"measurement_unit", "TEXT"}, {"unique_id", "INTEGER"}}));
    ASSERT_TRUE(db.InsertData("p_levels_hierarchy_names",
                              std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>{{1, 2, 3, 4}}));
    ASSERT_TRUE(db.InsertData("p_metrics", std::vector<std::tuple<uint64_t, uint64_t, double, uint64_t>>{{5, 6, 7.5, 1}}));
    ASSERT_TRUE(db.InsertData("p_scaling_values", std::vector<std::tuple<uint64_t, uint64_t, double>>{{8, 2, 9.5}}));
    ASSERT_TRUE(db.InsertData("p_titles_names",
                              std::vector<std::tuple<uint64_t, std::string, std::string, uint64_t, std::string, uint64_t>>{
                                  {10, "title", "description", 1, "ns", 11}}));
}

void CreateThreadDatabase()
{
    DBRunner db(THREAD_DB);
    ASSERT_TRUE(db.CreateTable("p_thread", {{"id", "INTEGER", true}, {"tid", "INTEGER"}, {"name", "TEXT"},
                                            {"process_id", "INTEGER"}, {"parent_id", "INTEGER"},
                                            {"start_ts", "INTEGER"}, {"end_ts", "INTEGER"}}));
    ASSERT_TRUE(db.CreateTable("p_process", {{"id", "INTEGER", true}, {"pid", "INTEGER"}, {"name", "TEXT"},
                                             {"start_ts", "INTEGER"}, {"end_ts", "INTEGER"}}));
    ASSERT_TRUE(db.CreateTable("p_core_metric_desc", {{"id", "INTEGER", true}, {"name", "TEXT"},
                                                      {"description", "TEXT"}, {"measurement_unit", "TEXT"}}));
    ASSERT_TRUE(db.CreateTable("p_core_metric", {{"id", "INTEGER", true}, {"ts", "INTEGER"}, {"value", "REAL"},
                                                  {"desc_id", "INTEGER"}, {"tid_id", "INTEGER"}, {"cpu_id", "INTEGER"}}));
    const auto thread = std::make_tuple<int64_t, int64_t, std::string, int64_t, int64_t, int64_t, int64_t>(
        1, 2, "thread", 3, 0, 4, 5);
    const auto process = std::make_tuple<int64_t, int64_t, std::string, int64_t, int64_t>(3, 4, "process", 5, 6);
    const auto metricDesc = std::make_tuple<int64_t, std::string, std::string, std::string>(7, "metric", "desc", "ns");
    const auto metric = std::make_tuple<int64_t, int64_t, double, int64_t, int64_t, int64_t>(8, 9, 1.5, 7, 1, 0);
    ASSERT_TRUE(db.InsertData(
        "p_thread", std::vector<std::tuple<int64_t, int64_t, std::string, int64_t, int64_t, int64_t, int64_t>>{
                        thread}));
    ASSERT_TRUE(db.InsertData("p_process",
                              std::vector<std::tuple<int64_t, int64_t, std::string, int64_t, int64_t>>{
                                  process}));
    ASSERT_TRUE(db.InsertData("p_core_metric_desc",
                              std::vector<std::tuple<int64_t, std::string, std::string, std::string>>{
                                  metricDesc}));
    ASSERT_TRUE(db.InsertData(
        "p_core_metric", std::vector<std::tuple<int64_t, int64_t, double, int64_t, int64_t, int64_t>>{
                             metric}));
}
}  // namespace

class HostPlatformProcessorUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (File::Exist(DATA_DIR))
        {
            ASSERT_TRUE(File::RemoveDir(DATA_DIR, 0));
        }
        ASSERT_TRUE(File::CreateDir(DATA_DIR));
        ASSERT_TRUE(File::CreateDir(PROF_DIR));
        ASSERT_TRUE(File::CreateDir(CHILD_PROF_DIR));
        ASSERT_TRUE(File::CreateDir(File::PathJoin({CHILD_PROF_DIR, "host"})));
        ASSERT_TRUE(File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override
    {
        if (File::Exist(DATA_DIR))
        {
            EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
        }
    }
};

TEST_F(HostPlatformProcessorUTest, ShouldReadThreadDatabaseIntoDataInventory)
{
    CreateThreadDatabase();

    DataInventory inventory;
    HostPlatformProcessor processor(CHILD_PROF_DIR);
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_HOST_PLATFORM));
    const auto threads = inventory.GetPtr<std::vector<HostCoreThreadData>>();
    const auto processes = inventory.GetPtr<std::vector<HostCoreProcessData>>();
    const auto descs = inventory.GetPtr<std::vector<HostCoreMetricDescData>>();
    const auto metrics = inventory.GetPtr<std::vector<HostCoreMetricData>>();
    ASSERT_NE(threads, nullptr);
    ASSERT_NE(processes, nullptr);
    ASSERT_NE(descs, nullptr);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(threads->at(0).name, "thread");
    EXPECT_EQ(processes->at(0).pid, 4);
    EXPECT_EQ(descs->at(0).measurementUnit, "ns");
    EXPECT_DOUBLE_EQ(metrics->at(0).value, 1.5);
}

TEST_F(HostPlatformProcessorUTest, ShouldReadNumaDatabaseIntoDataInventory)
{
    CreateNumaDatabase();

    DataInventory inventory;
    HostPlatformProcessor processor(CHILD_PROF_DIR);
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_HOST_PLATFORM));
    const auto hierarchies = inventory.GetPtr<std::vector<NumaLevelsHierarchyData>>();
    const auto metrics = inventory.GetPtr<std::vector<NumaMetricsData>>();
    const auto scalingValues = inventory.GetPtr<std::vector<NumaScalingValuesData>>();
    const auto titlesNames = inventory.GetPtr<std::vector<NumaTitlesNamesData>>();
    ASSERT_NE(hierarchies, nullptr);
    ASSERT_NE(metrics, nullptr);
    ASSERT_NE(scalingValues, nullptr);
    ASSERT_NE(titlesNames, nullptr);
    EXPECT_EQ(hierarchies->at(0).title2_id, 4U);
    EXPECT_DOUBLE_EQ(metrics->at(0).value, 7.5);
    EXPECT_DOUBLE_EQ(scalingValues->at(0).max_value, 9.5);
    EXPECT_EQ(titlesNames->at(0).measurement_unit, "ns");
}

TEST_F(HostPlatformProcessorUTest, ShouldIgnoreMissingSourceTables)
{
    {
        DBRunner db(THREAD_DB);
        ASSERT_TRUE(db.CreateTable("p_process", {{"id", "INTEGER", true}, {"pid", "INTEGER"},
                                                 {"name", "TEXT"}, {"start_ts", "INTEGER"}, {"end_ts", "INTEGER"}}));
        const auto process = std::make_tuple<int64_t, int64_t, std::string, int64_t, int64_t>(
            3, 4, "process", 5, 6);
        ASSERT_TRUE(db.InsertData("p_process",
                                  std::vector<std::tuple<int64_t, int64_t, std::string, int64_t, int64_t>>{
                                      process}));
    }

    DataInventory inventory;
    HostPlatformProcessor processor(CHILD_PROF_DIR);
    ASSERT_TRUE(processor.Run(inventory, PROCESSOR_NAME_HOST_PLATFORM));
    const auto threads = inventory.GetPtr<std::vector<HostCoreThreadData>>();
    const auto processes = inventory.GetPtr<std::vector<HostCoreProcessData>>();
    const auto descs = inventory.GetPtr<std::vector<HostCoreMetricDescData>>();
    const auto metrics = inventory.GetPtr<std::vector<HostCoreMetricData>>();
    ASSERT_NE(processes, nullptr);
    ASSERT_EQ(processes->size(), 1UL);
    EXPECT_EQ(processes->at(0).name, "process");
    EXPECT_EQ(threads, nullptr);
    EXPECT_EQ(descs, nullptr);
    EXPECT_EQ(metrics, nullptr);
}
