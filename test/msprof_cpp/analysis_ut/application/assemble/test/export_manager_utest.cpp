/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/application/include/report_map.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/application/include/export_mode_enum.h"


using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Domain;
using EnvContext = Analysis::Domain::Environment::Context;

namespace {
class TopologyRegistryGuard
{
   public:
    TopologyRegistryGuard() : backup_(TopoNodeRegistry::GetDefinitions()) {}
    ~TopologyRegistryGuard() { TopoNodeRegistry::MutableDefinitions() = backup_; }

   private:
    TopoNodeCollection backup_;
};

const int DEPTH = 0;
const std::string BASE_PATH = "./export_test";
const std::string DEVICE = "device_0";
const std::string DB_NAME = "trace.db";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
const std::string REPORTS_JSON = "reports.json";
}
using ReduceDataType = std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t>>;
using GetNextDataType = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t>>;
using TraceDataType = std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t>>;
ReduceDataType reduce{{0, 2, 1, 313056236532, 313055200207, 313055459640},
                      {0, 2, 2, 318016202377, 318015356897, 318015374534}};
GetNextDataType next{{2, 1, 313055463171, 313055464468}, {2, 2, 318015397631, 318015399357}};
TraceDataType trace{{0, 1, 1, 306258517346, 306258521644, 306258522140, 292028, 4298, 496, 0},
                    {0, 2, 1, 313055197979, 313056220356, 313056236532, 1044232, 1022377, 16176, 6796675839},
                    {0, 3, 1, 0, 317098454510, 317099294827, 75569326, 0, 840317, 0}};

class ExportManagerUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE, SQLITE})));
        EXPECT_TRUE(CreateReduceData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), reduce, "all_reduce"));
        EXPECT_TRUE(CreateNextData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), next, "get_next"));
        EXPECT_TRUE(CreateTraceData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), trace, "training_trace"));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    static bool CreateReduceData(const std::string &dbPath, ReduceDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
    static bool CreateNextData(const std::string &dbPath, GetNextDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
    static bool CreateTraceData(const std::string &dbPath, TraceDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
    static void CreateReportsJson(nlohmann::json &reports)
    {
        FileWriter reportsWriter(File::PathJoin({BASE_PATH, REPORTS_JSON}));
        reportsWriter.WriteText(reports.dump());
    }
    bool Plan(ExportManager &manager, const std::set<ExportMode> &modes)
    {
        TopoBuildContext context;
        std::vector<TopoNodeId> roots;
        return manager.Plan(modes, context, roots);
    }
};

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenPathIsInvalid)
{
    std::string path = "/home/prof_0";
    ExportManager manager(path);
    EXPECT_FALSE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldPlanWithoutLoadingEnvironment)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&EnvContext::Load).expects(never());
    MOCKER_CPP(&File::CreateDir).expects(never());
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE}, context, roots));
    EXPECT_FALSE(roots.empty());
    EXPECT_TRUE(context.dbPath.empty());
}

TEST_F(ExportManagerUTest, ShouldPlanTimelineRoots)
{
    ExportManager manager(PROF_PATH);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldPlanDatabaseRoots)
{
    ExportManager manager(PROF_PATH);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::DB}));
}

TEST_F(ExportManagerUTest, ShouldPlanSelectedDatabaseRoots)
{
    nlohmann::json reports = {
        {"db_process", {
            {"ddr", true},
            {"hbm", true},
            {"ascend_hardware", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    EXPECT_FALSE(context.dbPath.empty());
    // The two selected savers, the 4 shared savers every DB export carries, and the StringIds node.
    ASSERT_EQ(roots.size(), 7UL);
    EXPECT_EQ(roots[0].name, "DB:" + PROCESSOR_NAME_DDR);
    EXPECT_EQ(roots[1].name, "DB:" + PROCESSOR_NAME_HBM);
    EXPECT_EQ(roots.back().name, "DB:" + TABLE_NAME_STRING_IDS);
    EXPECT_EQ(roots.back().stage, TopoNodeStage::FLOW_CONTROL);
}

TEST_F(ExportManagerUTest, ShouldExpandHardwareDomainToItsDatabaseTables)
{
    nlohmann::json reports = {
        {"db_process", {
            {"ascend_hardware", true}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    // The domain also plans the tables whose rows key on TASK.globalTaskId and the enum tables its columns
    // use, plus the 4 shared savers and the StringIds node.
    ASSERT_EQ(roots.size(), 10UL);
    EXPECT_EQ(roots[0].name, "DB:" + PROCESSOR_NAME_TASK);
    EXPECT_EQ(roots[1].name, "DB:" + PROCESSOR_NAME_COMPUTE_TASK_INFO);
    EXPECT_EQ(roots[2].name, "DB:" + PROCESSOR_NAME_MEMCPY_INFO);
    EXPECT_EQ(roots[3].name, "DB:" + PROCESSOR_NAME_TASK_PMU_INFO);
    EXPECT_EQ(roots[4].name, "DB:" + PROCESSOR_NAME_ENUM);
    EXPECT_EQ(roots.back().name, "DB:" + TABLE_NAME_STRING_IDS);
}

TEST_F(ExportManagerUTest, ShouldPlanEachLinkedTableOnceWhenDomainsOverlap)
{
    nlohmann::json reports = {
        {"db_process", {
            {"ascend_hardware", true},
            {"cann_api", true}}}
    };
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    // The enum tables are shared by both domains, and the plan keeps them only once. The 4 shared savers
    // and the StringIds node follow the 6 distinct savers of the two domains.
    ASSERT_EQ(roots.size(), 11UL);
    EXPECT_EQ(roots[0].name, "DB:" + PROCESSOR_NAME_TASK);
    EXPECT_EQ(roots[1].name, "DB:" + PROCESSOR_NAME_COMPUTE_TASK_INFO);
    EXPECT_EQ(roots[2].name, "DB:" + PROCESSOR_NAME_MEMCPY_INFO);
    EXPECT_EQ(roots[3].name, "DB:" + PROCESSOR_NAME_TASK_PMU_INFO);
    EXPECT_EQ(roots[4].name, "DB:" + PROCESSOR_NAME_ENUM);
    EXPECT_EQ(roots[5].name, "DB:" + PROCESSOR_NAME_API);
    EXPECT_EQ(roots.back().name, "DB:" + TABLE_NAME_STRING_IDS);
}

TEST_F(ExportManagerUTest, ShouldSkipDatabaseExportWhenSectionIsAbsent)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", true}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    // The section selects no table, so the DB deliverable is skipped while planning and the plan gets no root at all.
    EXPECT_TRUE(roots.empty());
}

TEST_F(ExportManagerUTest, ShouldRejectInvalidDatabaseSelectionWhenStrict)
{
    nlohmann::json reports = {
        {"db_process", {
            {"invalid_process", true}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    context.strictSelection = true;
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(manager.Plan({ExportMode::DB}, context, roots));
}

TEST_F(ExportManagerUTest, ShouldKeepAllDatabaseDeliverablesWhenInvalidSelectionIsNotStrict)
{
    nlohmann::json reports = {
        {"db_process", {
            {"invalid_process", true}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    EXPECT_FALSE(context.dbPath.empty());
    // 37 distinct savers across every report key, the 4 shared savers every DB export carries, and the
    // StringIds node that closes the export.
    EXPECT_EQ(roots.size(), 42UL);
}

TEST_F(ExportManagerUTest, ShouldPlanSelectedTimelineRoots)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", true},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonValueError)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", "true"},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenSummaryDeliverableIsInvalid)
{
    nlohmann::json reports = {
        {"summary_process", {
            {"invalid_deliverable", true}
        }}
    };
    CreateReportsJson(reports);
    const auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    TopoBuildContext context;
    context.strictSelection = true;
    std::vector<TopoNodeId> roots;

    EXPECT_FALSE(manager.Plan({Analysis::Application::ExportMode::SUMMARY}, context, roots));
}

TEST_F(ExportManagerUTest, ShouldSelectEnabledTimelineAndSummaryDeliverables)
{
    nlohmann::json reports = {
        {"json_process", {{"ascend", true}, {"hbm", false}}},
        {"summary_process", {{"op_summary", true}, {"npu_memory", false}}}
    };
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE, ExportMode::SUMMARY}, selection));
    ASSERT_EQ(selection.timelineProcesses.size(), 1UL);
    EXPECT_EQ(selection.timelineProcesses.front(), "ascend");
    ASSERT_EQ(selection.summaryProcesses.size(), 1UL);
    EXPECT_EQ(selection.summaryProcesses.front(), "op_summary");
}

TEST_F(ExportManagerUTest, ShouldMapAicpuReportAliasesToSameSummaryNode)
{
    nlohmann::json reports = {{"summary_process", {{"aicpu", true}, {"dp", true}, {"aicpu_mi", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;
    ASSERT_TRUE(manager.GetExportSelection({ExportMode::SUMMARY}, selection));
    // A JSON object keeps its keys sorted, so the selection order is the key order instead of the order in the file.
    EXPECT_EQ(selection.summaryProcesses, (std::vector<std::string>{"aicpu", "aicpu_mi", "dp"}));
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::SUMMARY}, context, roots));
    // The three keys share one summary node, so the plan keeps a single root for it.
    ASSERT_EQ(roots.size(), 1UL);
    EXPECT_EQ(roots.front(), (TopoNodeId{TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_AICPU}));
}

TEST_F(ExportManagerUTest, ShouldRemoveDuplicateRootsFromDefaultSelection)
{
    ExportManager manager(PROF_PATH);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE, ExportMode::SUMMARY}, context, roots));
    // The default selection expands every report key, and the aicpu, dp and aicpu_mi keys share one summary node.
    const TopoNodeId aicpuRoot{TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_AICPU};
    std::unordered_set<TopoNodeId, TopoNodeIdHash> uniqueRoots;
    size_t aicpuRootCount = 0;
    for (const auto &root : roots)
    {
        uniqueRoots.insert(root);
        if (root == aicpuRoot)
        {
            ++aicpuRootCount;
        }
    }
    EXPECT_EQ(aicpuRootCount, 1UL);
    EXPECT_EQ(uniqueRoots.size(), roots.size());
}

TEST_F(ExportManagerUTest, ShouldRejectProcessorNameUsedAsReportKey)
{
    nlohmann::json reports = {{"summary_process", {{PROCESSOR_OP_SUMMARY, true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;
    EXPECT_FALSE(manager.GetExportSelection({ExportMode::SUMMARY}, selection, true));
    EXPECT_TRUE(selection.summaryProcesses.empty());
}

TEST_F(ExportManagerUTest, ShouldCoverEverySummaryDeliverableInReportsKeys)
{
    // Every summary deliverable must be reachable from the reports keys, otherwise the default selection skips it
    // silently and its csv file is never exported.
    std::vector<std::string> defaultAssemblers;
    ASSERT_TRUE(SummaryManager::GetAssemblerList({}, defaultAssemblers));
    std::unordered_set<std::string> reachable;
    for (const auto &reportKey : SUMMARY_JSON_NODE_KEYS)
    {
        EXPECT_TRUE(SummaryManager::IsDeliverableSupported(reportKey.first)) << reportKey.first;
        reachable.insert(reportKey.second);
    }
    for (const auto &assembler : defaultAssemblers)
    {
        EXPECT_EQ(reachable.count(assembler), 1UL) << assembler;
    }
}

TEST_F(ExportManagerUTest, ShouldKeepFreqJsonKeyInSelection)
{
    nlohmann::json reports = {{"json_process", {{"freq", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    ASSERT_EQ(selection.timelineProcesses, std::vector<std::string>{"freq"});
    EXPECT_EQ(GetTimelineNodeKey("freq"), PROCESS_LOW_POWER);
}

TEST_F(ExportManagerUTest, ShouldKeepDefaultTimelineSelectionWhenTimelineConfigIsInvalid)
{
    nlohmann::json reports = {{"json_process", {{"invalid_process", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    EXPECT_EQ(selection.timelineProcesses, GetAllTimelineJsonKeys());
}

TEST_F(ExportManagerUTest, ShouldKeepDefaultDeliverablesWhenReportConfigIsNotAnObject)
{
    nlohmann::json reports = nlohmann::json::array({"json_process"});
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    EXPECT_EQ(selection.timelineProcesses, GetAllTimelineJsonKeys());
    EXPECT_FALSE(manager.GetExportSelection({ExportMode::SUMMARY}, selection, true));
}

TEST_F(ExportManagerUTest, ShouldRejectInvalidSummarySelectionWhenStrict)
{
    const auto reportsPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportSelection selection;

    nlohmann::json invalidType = {{"summary_process", {{"op_summary", "true"}}}};
    CreateReportsJson(invalidType);
    ExportManager invalidTypeManager(PROF_PATH, reportsPath);
    EXPECT_FALSE(invalidTypeManager.GetExportSelection({ExportMode::SUMMARY}, selection, true));
}

TEST_F(ExportManagerUTest, ShouldKeepDefaultSummaryWhenInvalidSelectionIsNotStrict)
{
    const auto reportsPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportSelection selection;

    nlohmann::json invalidType = {{"summary_process", {{"op_summary", "true"}}}};
    CreateReportsJson(invalidType);
    ExportManager manager(PROF_PATH, reportsPath);
    ASSERT_TRUE(manager.GetExportSelection({ExportMode::SUMMARY}, selection));
    EXPECT_EQ(selection.summaryProcesses, GetAllSummaryJsonKeys());
}

TEST_F(ExportManagerUTest, ShouldDisableSummaryWhenAllConfiguredDeliverablesAreFalse)
{
    ExportSelection selection;
    const auto reportsPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    nlohmann::json allDisabled = {{"summary_process", {{"op_summary", false}}}};
    CreateReportsJson(allDisabled);
    ExportManager allDisabledManager(PROF_PATH, reportsPath);
    ASSERT_TRUE(allDisabledManager.GetExportSelection({ExportMode::SUMMARY}, selection));
    EXPECT_TRUE(selection.summaryProcesses.empty());

    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(allDisabledManager.Plan({ExportMode::SUMMARY}, context, roots));
    EXPECT_TRUE(roots.empty());
    Analysis::Infra::RuntimeProcessCollection processes;
    EXPECT_TRUE(TopoGraphBuilder().Build(PROF_PATH, {ExportMode::SUMMARY}, reportsPath, processes));
    EXPECT_TRUE(processes.empty());
    MOCKER_CPP(&EnvContext::Load).expects(once()).will(returnValue(true));
    EXPECT_TRUE(allDisabledManager.Run({ExportMode::SUMMARY}));
}

TEST_F(ExportManagerUTest, ShouldKeepSummarySelectionEmptyWhenConfigIsMissingNullOrEmpty)
{
    for (const auto &config : std::vector<nlohmann::json>{nlohmann::json::object(),
             {{"summary_process", nullptr}}, {{"summary_process", nlohmann::json::object()}}})
    {
        auto reports = config;
        CreateReportsJson(reports);
        ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
        ExportSelection selection;
        selection.summaryProcesses = {"previous_selection"};
        ASSERT_TRUE(manager.GetExportSelection({ExportMode::SUMMARY}, selection));
        EXPECT_TRUE(selection.summaryProcesses.empty());
        TopoBuildContext context;
        std::vector<TopoNodeId> roots;
        ASSERT_TRUE(manager.Plan({ExportMode::SUMMARY}, context, roots));
        EXPECT_TRUE(roots.empty());
    }
}

TEST_F(ExportManagerUTest, ShouldKeepCannTimelineWhenSummaryIsDisabled)
{
    nlohmann::json reports = {{"json_process", {{"cann", true}}},
                              {"summary_process", {{"op_summary", false}, {"api_statistic", false}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE, ExportMode::SUMMARY}, context, roots));
    EXPECT_FALSE(roots.empty());
    bool cannKept = false;
    for (const auto &root : roots)
    {
        EXPECT_NE(root.stage, TopoNodeStage::SUMMARY_GENERATION);
        cannKept = cannKept || root == TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_API};
    }
    EXPECT_TRUE(cannKept);
}

TEST_F(ExportManagerUTest, ShouldKeepTimelineSelectionEmptyWhenAllDeliverablesAreDisabled)
{
    nlohmann::json reports = {{"json_process", {{"ascend", false}, {"hbm", false}}}};
    CreateReportsJson(reports);
    const auto reportsPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, reportsPath);
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    EXPECT_TRUE(selection.timelineProcesses.empty());

    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE}, context, roots));
    EXPECT_TRUE(roots.empty());

    Analysis::Infra::RuntimeProcessCollection processes;
    EXPECT_TRUE(TopoGraphBuilder().Build(PROF_PATH, {ExportMode::TIMELINE}, reportsPath, processes));
    EXPECT_TRUE(processes.empty());

    MOCKER_CPP(&EnvContext::Load).expects(once()).will(returnValue(true));
    EXPECT_TRUE(manager.Run({ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldKeepTimelineSelectionEmptyWhenJsonProcessIsAbsent)
{
    nlohmann::json reports = {{"summary_process", {{"op_summary", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE, ExportMode::SUMMARY}, selection));
    EXPECT_TRUE(selection.timelineProcesses.empty());
    EXPECT_EQ(selection.summaryProcesses, std::vector<std::string>{"op_summary"});
}

TEST_F(ExportManagerUTest, ShouldExpandDefaultDeliverablesOnlyWhenReportPathIsMissing)
{
    ExportManager manager(PROF_PATH);
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE, ExportMode::SUMMARY}, selection));
    EXPECT_EQ(selection.timelineProcesses, GetAllTimelineJsonKeys());
    EXPECT_EQ(selection.summaryProcesses, GetAllSummaryJsonKeys());
}

TEST_F(ExportManagerUTest, ShouldHonorCompleteReportConfigWithSingleTimelineDeliverable)
{
    nlohmann::json reports = {
        {"json_process", {
            {"ascend", false}, {"acc_pmu", false}, {"cann", true}, {"ddr", false}, {"stars_chip_trans", false},
            {"hbm", false}, {"communication", false}, {"ccu", false}, {"hccs", false}, {"os_runtime_api", false},
            {"network_usage", false}, {"disk_usage", false}, {"memory_usage", false}, {"cpu_usage", false},
            {"cpu_freq", false}, {"msproftx", false}, {"npu_mem", false}, {"overlap_analyse", false},
            {"pcie", false}, {"sio", false}, {"stars_soc", false}, {"step_trace", false}, {"freq", false},
            {"llc", false}, {"nic", false}, {"roce", false}, {"qos", false}, {"device_tx", false},
            {"biu_perf", false}, {"ub", false}, {"block_detail", false}, {"dpu", false}, {"fusion_task", false}}
        },
        {"summary_process", {
            {"op_summary", false}, {"comm_statistic", false}, {"op_statistic", false}, {"npu_memory", false},
            {"npu_module_memory", false}, {"api_statistic", false}, {"fusion_op", false}, {"task_time", false},
            {"step_trace", false}, {"page_fault", false}, {"aicpu", false}, {"dp", false}, {"aicpu_mi", false}}
        }
    };
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE, ExportMode::SUMMARY}, selection));
    EXPECT_EQ(selection.timelineProcesses, std::vector<std::string>{"cann"});
    EXPECT_TRUE(selection.summaryProcesses.empty());

    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE, ExportMode::SUMMARY}, context, roots));
    EXPECT_FALSE(roots.empty());
    bool cannKept = false;
    for (const auto& root : roots)
    {
        EXPECT_NE(root.stage, TopoNodeStage::SUMMARY_GENERATION);
        cannKept = cannKept || root == TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_API};
    }
    EXPECT_TRUE(cannKept);
}

TEST_F(ExportManagerUTest, ShouldRejectSummaryConfigWhenItIsNotAnObjectInStrictMode)
{
    nlohmann::json reports = {{"summary_process", "op_summary"}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    EXPECT_FALSE(manager.GetExportSelection({ExportMode::SUMMARY}, selection, true));
    EXPECT_TRUE(selection.summaryProcesses.empty());

    ExportSelection looseSelection;
    EXPECT_TRUE(manager.GetExportSelection({ExportMode::SUMMARY}, looseSelection));
    EXPECT_TRUE(looseSelection.summaryProcesses.empty());
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonProcessNotExist)
{
    nlohmann::json reports = {
        {"json_processxxxx", {}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonPathNotExist)
{
    auto jsonPath = File::PathJoin({BASE_PATH, "missing_reports.json"});
    ASSERT_FALSE(File::Exist(jsonPath));
    ExportManager manager(PROF_PATH, jsonPath);
    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldRejectEmptyAndUnsupportedExportModes)
{
    ExportManager manager(PROF_PATH);

    EXPECT_FALSE(Plan(manager, {}));
    EXPECT_FALSE(Plan(manager, {static_cast<ExportMode>(99)}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenProcessorTopologyRegistrationIsMissing)
{
    ExportManager manager(PROF_PATH);
    TopologyRegistryGuard guard;
    TopoNodeRegistry::MutableDefinitions().erase({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_TASK});
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::TIMELINE}, context, roots));
    Analysis::Infra::RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, roots, graph));
    EXPECT_TRUE(graph.empty());
}

TEST_F(ExportManagerUTest, ShouldBuildProcessorDependencyClosure)
{
    Analysis::Infra::RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_OVERLAP_ANALYSIS}}, processes));

    EXPECT_EQ(processes.size(), 5UL);
    bool overlapFound = false;
    for (const auto& process : processes) {
        if (process.second.processName == PROCESSOR_NAME_OVERLAP_ANALYSIS) {
            overlapFound = true;
            EXPECT_EQ(process.second.processDependence.size(), 3UL);
            EXPECT_EQ(process.second.paramTypes.size(), 3UL);
        }
        EXPECT_NE(process.second.processName, "DataProcessorCollector");
    }
    EXPECT_TRUE(overlapFound);
}

TEST_F(ExportManagerUTest, ShouldBuildCombinedDbTimelineAndSummaryTopology)
{
    std::vector<TopoNodeId> roots;
    DBAssembler::GetTopologyRoots(GetAllDBJsonKeys(), roots);
    TimelineManager::GetTopologyRoots({"npu_mem"}, roots);
    SummaryManager::GetTopologyRoots({"npu_memory"}, roots);

    const auto dbOutputPath = File::PathJoin({BASE_PATH, "combined_db"});
    ASSERT_TRUE(File::CreateDir(dbOutputPath));
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    context.dbPath = DBAssembler::MakeDBPath(dbOutputPath);
    Analysis::Infra::RuntimeProcessCollection processes;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, processes));

    bool dbSaverFound = false;
    bool timelineFound = false;
    bool summaryFound = false;
    for (const auto& process : processes)
    {
        if (process.second.processName == std::string("DB:") + PROCESSOR_NAME_NPU_MEM)
        {
            dbSaverFound = true;
        }
        else if (process.second.processName == Analysis::Domain::PROCESS_NPU_MEM &&
                 process.second.paramTypes.size() == 1UL)
        {
            timelineFound = true;
        }
        else if (process.second.processName == PROCESSOR_NAME_NPU_MEM && process.second.paramTypes.size() == 1UL)
        {
            summaryFound = true;
        }
    }
    EXPECT_TRUE(dbSaverFound);
    EXPECT_TRUE(timelineFound);
    EXPECT_TRUE(summaryFound);
}

TEST_F(ExportManagerUTest, ShouldPlanWithoutCreatingOutputPath)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&File::CreateDir).expects(never());
    EXPECT_TRUE(Plan(manager, {ExportMode::TIMELINE, ExportMode::SUMMARY}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenAnalysisReportJsonFailed)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", true},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    const int analysisError = 1;

    MOCKER_CPP(&FileReader::ReadJson)
    .stubs()
    .will(returnValue(analysisError));

    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));

    MOCKER_CPP(&FileReader::ReadJson).reset();
}

TEST_F(ExportManagerUTest, ShouldAddLowPowerWhenFreqIsTrue)
{
    nlohmann::json reports = {
        {"json_process", {
            {"freq", true}
        }}
    };
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);

    EXPECT_TRUE(Plan(manager, {Analysis::Application::ExportMode::TIMELINE}));
}
