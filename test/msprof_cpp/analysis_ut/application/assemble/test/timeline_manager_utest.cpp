/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <stdexcept>
#include <typeindex>
#include <utility>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"

#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/infrastructure/context/include/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;

namespace
{
const std::string PROF_PATH = "./timeline_test/PROF_0";
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});

class TopologyRegistryGuard
{
   public:
    TopologyRegistryGuard() : backup_(TopoNodeRegistry::GetDefinitions()) {}
    ~TopologyRegistryGuard() { TopoNodeRegistry::MutableDefinitions() = backup_; }

   private:
    TopoNodeCollection backup_;
};
}  // namespace

TEST(TimelineManagerUTest, ShouldBuildSelectedNodesWithPreAndPostDump)
{
    const std::vector<JsonProcess> selection{JsonProcess::NPU_MEM};
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots(selection, roots));
    ASSERT_EQ(roots.size(), 2UL);
    EXPECT_EQ(roots[0], (TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_NPU_MEM}));
    EXPECT_EQ(roots[1], (TopoNodeId{TopoNodeStage::FLOW_CONTROL, "TIMELINE:POST_DUMP"}));

    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    context.outputPath = RESULT_PATH;
    context.timelineProcesses = selection;
    context.timelineSession = std::make_shared<TimelineManager>(PROF_PATH, RESULT_PATH);
    RuntimeProcessCollection processes;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, processes));

    RuntimeProcessCollection::const_iterator pre = processes.end();
    RuntimeProcessCollection::const_iterator timeline = processes.end();
    const RuntimeProcessInfo* postDump = nullptr;
    for (const auto& process : processes)
    {
        if (process.second.processName == "TIMELINE:PRE_DUMP")
        {
            pre = processes.find(process.first);
        }
        else if (process.second.processName == PROCESS_NPU_MEM && process.second.paramTypes.size() == 1UL)
        {
            timeline = processes.find(process.first);
            EXPECT_EQ(process.second.paramTypes.size(), 1UL);
        }
        else if (process.second.processName == "TIMELINE:POST_DUMP")
        {
            postDump = &process.second;
        }
    }
    ASSERT_NE(pre, processes.end());
    ASSERT_NE(timeline, processes.end());
    ASSERT_NE(postDump, nullptr);
    const RuntimeProcessKey preKey = pre->first;
    const RuntimeProcessKey timelineKey = timeline->first;
    EXPECT_TRUE(pre->second.runOnDependencyFailure);
    EXPECT_FALSE(timeline->second.runOnDependencyFailure);
    EXPECT_TRUE(postDump->runOnDependencyFailure);
    EXPECT_NE(std::find(timeline->second.processDependence.begin(), timeline->second.processDependence.end(), preKey),
              timeline->second.processDependence.end());
    EXPECT_NE(std::find(postDump->processDependence.begin(), postDump->processDependence.end(), preKey),
              postDump->processDependence.end());
    EXPECT_NE(std::find(postDump->processDependence.begin(), postDump->processDependence.end(), timelineKey),
              postDump->processDependence.end());
    EXPECT_TRUE(postDump->completionDependence.empty());
}

TEST(TimelineManagerUTest, ShouldBuildOnlySelectedReports)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots({JsonProcess::ASCEND}, roots));
    ASSERT_EQ(roots.size(), 2UL);
    EXPECT_EQ(roots.front(), (TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_TASK}));
}

TEST(TimelineManagerUTest, PreDumpShouldOnlyWaitForMsprofTxHostData)
{
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.timelineProcesses = {JsonProcess::NPU_MEM};
    EXPECT_TRUE(TimelineManager::ResolveTimelinePreDumpDependencies(context, {}).empty());

    context.timelineProcesses = {JsonProcess::MSPROFTX};
    const std::vector<TopoNodeId> dependencies =
        TimelineManager::ResolveTimelinePreDumpDependencies(context, {});
    ASSERT_EQ(dependencies.size(), 1UL);
    EXPECT_EQ(dependencies.front(), (TopoNodeId{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_MSTX}));
}

TEST(TimelineManagerUTest, PreDumpShouldDeclareMsprofTxHostData)
{
    const TopoNodeDefinition* definition =
        TopoNodeRegistry::Find({TopoNodeStage::FLOW_CONTROL, "TIMELINE:PRE_DUMP"});
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->inputDataTypes, std::vector<std::type_index>{typeid(std::vector<MsprofTxHostData>)});
    EXPECT_NE(definition->dependencyResolver, nullptr);
    EXPECT_EQ(definition->completionDependencyResolver, nullptr);
    EXPECT_TRUE(definition->runOnDependencyFailure);
}

TEST(TimelineManagerUTest, FailedMsprofTxProcessorDoesNotBlockOtherLayersOrJsonFinalization)
{
    const std::string output = "./timeline_branch_isolation_test";
    if (File::Exist(output))
    {
        ASSERT_TRUE(File::RemoveDir(output, 0));
    }
    ASSERT_TRUE(File::CreateDir(output));
    struct OutputGuard
    {
        std::string path;
        ~OutputGuard() { File::RemoveDir(path, 0); }
    } outputGuard{output};
    TopologyRegistryGuard registryGuard;
    auto& processor = TopoNodeRegistry::MutableDefinitions().at({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_MSTX});
    processor.processDependencies.clear();
    processor.dependencyResolver = nullptr;
    processor.inputDataTypes.clear();
    processor.creatorFactory = [](const TopoBuildContext&)
    {
        return []() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return false; }));
        };
    };
    const TopoNodeId good{TopoNodeStage::TIMELINE_EXPORT, PROCESS_NPU_MEM};
    auto& layer = TopoNodeRegistry::MutableDefinitions().at(good);
    layer.processDependencies = {{TopoNodeStage::FLOW_CONTROL, "TIMELINE:PRE_DUMP"}};
    layer.inputDataTypes.clear();
    // Keep the real NPU memory assembler: it contributes no events without input data.
    TopoBuildContext context;
    context.profPath = PROF_PATH;
    context.outputPath = output;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.timelineProcesses = {JsonProcess::MSPROFTX, JsonProcess::NPU_MEM};
    context.timelineSession = std::make_shared<TimelineManager>(PROF_PATH, output);
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots(context.timelineProcesses, roots));
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph));
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_MSTX, "export"})).state,
              RuntimeProcessState::FAILED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::TIMELINE_EXPORT, PROCESS_MSPROFTX, "export"})).state,
              RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(keys.at({good.stage, good.name, "export"})).state, RuntimeProcessState::SUCCEEDED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::FLOW_CONTROL, "TIMELINE:POST_DUMP", "export"})).state,
              RuntimeProcessState::SUCCEEDED);
    const auto files = File::GetOriginData(output, {MSPROF_JSON_FILE}, {});
    EXPECT_TRUE(files.empty());
}

TEST(TimelineManagerUTest, PostDumpRunsAfterPreDumpFailureWithoutCreatingEmptyFiles)
{
    const std::string output = "./timeline_pre_failure_cleanup_test";
    if (File::Exist(output)) { ASSERT_TRUE(File::RemoveDir(output, 0)); }
    ASSERT_TRUE(File::CreateDir(output));
    struct OutputGuard
    {
        std::string path;
        ~OutputGuard() { File::RemoveDir(path, 0); }
    } outputGuard{output};
    TopologyRegistryGuard registryGuard;
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    const TopoNodeId pre{TopoNodeStage::FLOW_CONTROL, "TIMELINE:PRE_DUMP"};
    definitions.at(pre).creatorFactory = [](const TopoBuildContext& context)
    {
        const auto session = context.timelineSession;
        return [session]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([session](DataInventory& inventory)
            {
                // Simulate initialization failing before any assembler contributes events.
                EXPECT_TRUE(session->PreDumpJson({}, inventory));
                return false;
            }));
        };
    };
    const TopoNodeId layer{TopoNodeStage::TIMELINE_EXPORT, PROCESS_NPU_MEM};
    definitions.at(layer).processDependencies = {pre};
    definitions.at(layer).inputDataTypes.clear();
    TopoBuildContext context;
    context.profPath = PROF_PATH;
    context.outputPath = output;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.timelineProcesses = {JsonProcess::NPU_MEM};
    context.timelineSession = std::make_shared<TimelineManager>(PROF_PATH, output);
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots(context.timelineProcesses, roots));
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph));
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(results.at(keys.at({pre.stage, pre.name, "export"})).state, RuntimeProcessState::FAILED);
    EXPECT_EQ(results.at(keys.at({layer.stage, layer.name, "export"})).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::FLOW_CONTROL, "TIMELINE:POST_DUMP", "export"})).state,
              RuntimeProcessState::SUCCEEDED);
    const auto files = File::GetOriginData(output, {MSPROF_JSON_FILE}, {});
    EXPECT_TRUE(files.empty());
}

TEST(TimelineManagerUTest, ShouldBuildAllRegisteredReports)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots(allProcesses, roots));
    ASSERT_FALSE(roots.empty());
    EXPECT_EQ(roots.back(), (TopoNodeId{TopoNodeStage::FLOW_CONTROL, "TIMELINE:POST_DUMP"}));
    for (std::vector<TopoNodeId>::const_iterator it = roots.begin(); it != roots.end() - 1; ++it)
    {
        EXPECT_EQ(it->stage, TopoNodeStage::TIMELINE_EXPORT);
        EXPECT_NE(TopoNodeRegistry::Find(*it), nullptr);
    }
}

TEST(TimelineManagerUTest, ShouldRejectExecutionListNodeWithoutRegistration)
{
    TopologyRegistryGuard guard;
    TopoNodeRegistry::MutableDefinitions().erase({TopoNodeStage::TIMELINE_EXPORT, PROCESS_NPU_MEM});
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(TimelineManager::GetTopologyRoots({JsonProcess::NPU_MEM}, roots));
    EXPECT_TRUE(roots.empty());
}

TEST(TimelineManagerUTest, ShouldDeduplicateSelectedReports)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(TimelineManager::GetTopologyRoots({JsonProcess::ASCEND, JsonProcess::ASCEND, JsonProcess::CANN}, roots));
    ASSERT_EQ(roots.size(), 3UL);
    EXPECT_EQ(roots[0], (TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_TASK}));
    EXPECT_EQ(roots[1], (TopoNodeId{TopoNodeStage::TIMELINE_EXPORT, PROCESS_API}));
}

TEST(TimelineManagerUTest, ShouldNotCreateJsonWhenNoAssemblerWritesEvents)
{
    const std::string basePath = "./timeline_empty_json_test";
    const std::string profPath = File::PathJoin({basePath, "PROF_0"});
    const std::string outputPath = File::PathJoin({profPath, Analysis::Common::OUTPUT_PATH});
    if (File::Exist(basePath))
    {
        ASSERT_TRUE(File::RemoveDir(basePath, 0));
    }
    ASSERT_TRUE(File::CreateDir(basePath));
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(outputPath));

    TimelineManager manager(profPath, outputPath);
    DataInventory dataInventory;
    ASSERT_TRUE(manager.PreDumpJson({}, dataInventory));
    ASSERT_TRUE(manager.PostDumpJson());

    const std::vector<std::string> files = File::GetOriginData(outputPath, {MSPROF_JSON_FILE}, {});
    EXPECT_TRUE(files.empty());
    EXPECT_TRUE(File::RemoveDir(basePath, 0));
}

TEST(TimelineManagerUTest, ShouldReportUnwritableOutputWhenFirstEventsArrive)
{
    const std::string profPath = "./PROF_timeline_write_failure";
    ASSERT_TRUE(File::CreateDir(profPath));
    const auto output = File::PathJoin({profPath, Analysis::Common::OUTPUT_PATH});
    { FileWriter file(output); file.WriteText("file blocks output directory"); }
    auto& consumer = DeliverableConsumer::Instance();
    consumer.ResetOk();
    TimelineManager manager(profPath, output);
    DataInventory inventory;
    EXPECT_TRUE(manager.PreDumpJson({}, inventory));
    consumer.AcceptJson({MSPROF_JSON_FILE}, profPath,
                        {std::make_shared<DurationEvent>(1, 1, 1.5, "1.0", "op")});
    EXPECT_FALSE(manager.PostDumpJson());
    consumer.ResetOk();
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}

TEST(TimelineManagerUTest, ShouldReportAsyncWriterFailureAtPostDump)
{
    const std::string profPath = "./PROF_timeline_post_failure";
    ASSERT_TRUE(File::CreateDir(profPath));
    const auto output = File::PathJoin({profPath, Analysis::Common::OUTPUT_PATH});
    ASSERT_TRUE(File::CreateDir(output));
    auto& consumer = DeliverableConsumer::Instance();
    consumer.ResetOk();
    consumer.SetBeforeWriteForTest([] { throw std::runtime_error("injected write failure"); });
    TimelineManager manager(profPath, output);
    DataInventory inventory;
    EXPECT_TRUE(manager.PreDumpJson({}, inventory));
    consumer.AcceptJson({MSPROF_JSON_FILE}, profPath,
                        {std::make_shared<DurationEvent>(1, 1, 1.5, "1.0", "op")});
    EXPECT_FALSE(manager.PostDumpJson());
    consumer.SetBeforeWriteForTest({});
    consumer.ResetOk();
    EXPECT_TRUE(File::RemoveDir(profPath, 0));
}
