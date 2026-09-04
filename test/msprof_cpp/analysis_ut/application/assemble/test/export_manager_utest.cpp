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

#include <algorithm>
#include <memory>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
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
protected:
    DataInventory dataInventory_;
};

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenPathIsInvalid)
{
    std::string path = "/home/prof_0";
    ExportManager manager(path);
    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenContextInitFail)
{
    ExportManager manager(PROF_PATH);
    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessFail)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessSuccessWithDB)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::DB}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessSuccessWithReportJson)
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
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
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
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
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
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));

    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::SUMMARY}));
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
    EXPECT_EQ(selection.timelineProcesses.front(), JsonProcess::ASCEND);
    ASSERT_EQ(selection.summaryDeliverables.size(), 1UL);
    EXPECT_EQ(selection.summaryDeliverables.front(), "op_summary");
}

TEST_F(ExportManagerUTest, ShouldAddLowPowerToSelectionWhenFreqIsEnabled)
{
    nlohmann::json reports = {{"json_process", {{"freq", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    ASSERT_EQ(selection.timelineProcesses.size(), 2UL);
    EXPECT_EQ(selection.timelineProcesses[0], JsonProcess::FREQ);
    EXPECT_EQ(selection.timelineProcesses[1], JsonProcess::LOW_POWER);
}

TEST_F(ExportManagerUTest, ShouldKeepDefaultTimelineSelectionWhenTimelineConfigIsInvalid)
{
    nlohmann::json reports = {{"json_process", {{"invalid_process", true}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    ASSERT_TRUE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    EXPECT_EQ(selection.timelineProcesses, allProcesses);
}

TEST_F(ExportManagerUTest, ShouldRejectInvalidOrEmptySummarySelection)
{
    const auto reportsPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportSelection selection;

    nlohmann::json invalidType = {{"summary_process", {{"op_summary", "true"}}}};
    CreateReportsJson(invalidType);
    ExportManager invalidTypeManager(PROF_PATH, reportsPath);
    EXPECT_FALSE(invalidTypeManager.GetExportSelection({ExportMode::SUMMARY}, selection));

    nlohmann::json allDisabled = {{"summary_process", {{"op_summary", false}}}};
    CreateReportsJson(allDisabled);
    ExportManager allDisabledManager(PROF_PATH, reportsPath);
    EXPECT_FALSE(allDisabledManager.GetExportSelection({ExportMode::SUMMARY}, selection));
}

TEST_F(ExportManagerUTest, ShouldRejectTimelineWhenAllDeliverablesAreDisabled)
{
    nlohmann::json reports = {{"json_process", {{"ascend", false}, {"hbm", false}}}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    EXPECT_FALSE(manager.GetExportSelection({ExportMode::TIMELINE}, selection));
    EXPECT_EQ(selection.timelineProcesses, allProcesses);
}

TEST_F(ExportManagerUTest, ShouldRejectSummaryConfigWhenItIsNotAnObject)
{
    nlohmann::json reports = {{"summary_process", "op_summary"}};
    CreateReportsJson(reports);
    ExportManager manager(PROF_PATH, File::PathJoin({BASE_PATH, REPORTS_JSON}));
    ExportSelection selection;

    EXPECT_FALSE(manager.GetExportSelection({ExportMode::SUMMARY}, selection));
    EXPECT_TRUE(selection.summaryDeliverables.empty());
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonProcessNotExist)
{
    nlohmann::json reports = {
        {"json_processxxxx", {}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonPathNotExist)
{
    auto jsonPath = File::PathJoin({BASE_PATH, "missing_reports.json"});
    ASSERT_FALSE(File::Exist(jsonPath));
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldRejectEmptyAndUnsupportedExportModes)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));

    EXPECT_FALSE(manager.Run({}));
    EXPECT_FALSE(manager.Run({static_cast<ExportMode>(99)}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenProcessorTopologyRegistrationIsMissing)
{
    ExportManager manager(PROF_PATH, REPORTS_JSON);
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    TopologyRegistryGuard guard;
    TopoNodeRegistry::MutableDefinitions().erase({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_TASK});

    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldBuildProcessorDependencyClosure)
{
    Analysis::Infra::ProcessCollection processes;
    TopoBuildContext context;
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
    ASSERT_TRUE(DBAssembler::GetTopologyRoots(roots));
    ASSERT_TRUE(TimelineManager::GetTopologyRoots({JsonProcess::NPU_MEM}, roots));
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({"npu_memory"}, roots));

    const auto dbOutputPath = File::PathJoin({BASE_PATH, "combined_db"});
    ASSERT_TRUE(File::CreateDir(dbOutputPath));
    TopoBuildContext context;
    context.profPath = PROF_PATH;
    context.outputPath = dbOutputPath;
    context.timelineProcesses = {JsonProcess::NPU_MEM};
    context.dbSession = std::make_shared<DBAssembler>(PROF_PATH, dbOutputPath);
    context.timelineSession = std::make_shared<TimelineManager>(PROF_PATH, RESULT_PATH);
    Analysis::Infra::ProcessCollection processes;
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

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenCheckOutputPathFailed)
{
    ExportManager manager(PROF_PATH, REPORTS_JSON);
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));

    MOCKER_CPP(&File::Exist).
    stubs()
    .will(returnValue(false));

    MOCKER_CPP(&File::CreateDir)
    .stubs()
    .will(returnValue(false));

    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
    MOCKER_CPP(&File::Exist).reset();
    MOCKER_CPP(&File::CreateDir).reset();
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
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    const int analysisError = 1;

    MOCKER_CPP(&FileReader::ReadJson)
    .stubs()
    .will(returnValue(analysisError));

    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));

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
    
    // mock Init()
    MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&EnvContext::IsChipV6).stubs().will(returnValue(false));
    
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}
