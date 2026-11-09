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
 */#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/json_event_sink.h"
#include "analysis/csrc/application/pipeline/unified_pipeline.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

class UnifiedPipelinePlanUTest : public testing::Test {
protected:
    std::string root = "./PROF_unified_plan_utest";
    void SetUp() override
    {
        if (File::Exist(root)) { ASSERT_TRUE(File::RemoveDir(root, 0)); }
        File::CreateDir(root);
        File::CreateDir(File::PathJoin({root, "host"}));
        for (const auto& name : {"device_0", "device_1"}) {
            const auto device = File::PathJoin({root, name});
            File::CreateDir(device);
            File::CreateDir(File::PathJoin({device, "data"}));
            FileWriter(device + "/info.json").WriteText("{\"platform_version\":\"5\"}");
        }
    }
    void TearDown() override
    {
        try { GlobalMockObject::verify(); }
        catch (...) { File::RemoveDir(root, 0); throw; }
        File::RemoveDir(root, 0);
    }
    void ExpectNoUnifiedMarkers() const
    {
        for (const auto& directory : {root, root + "/host/sqlite", root + "/device_0/sqlite", root + "/device_1/sqlite"})
        {
            for (const auto& name : {".unified.complete", ".unified.incomplete"})
            {
                EXPECT_FALSE(File::Exist(File::PathJoin({directory, name})));
            }
        }
    }
    PipelineRequest Request(int flags)
    {
        PipelineRequest request;
        request.profPath = root;
        request.hostPath = File::PathJoin({root, "host"});
        request.devicePath = File::PathJoin({root, "device_0"});
        request.flags = flags;
        return request;
    }
    PipelineRequest StepTraceRequest()
    {
        auto request = Request(0x13);
        request.reportsJson = File::PathJoin({root, "reports.json"});
        FileWriter(request.reportsJson).WriteText("{\"summary_process\":{\"step_trace\":true}}");
        return request;
    }
    bool Build(const PipelineRequest& request, TopoBuildContext& context, TopoNodeCollection& definitions,
               std::vector<TopoNodeId>& roots, RuntimeProcessCollection& graph, TopoGraphBuilder& builder)
    {
        return PlanUnifiedPipeline(request, context, definitions, roots) &&
               builder.Build(context, roots, graph, &definitions);
    }
};

TEST_F(UnifiedPipelinePlanUTest, OnlyHostDataConsumersWaitForHostInPipelineAndLegacyExport)
{
    FileWriter(root + "/device_0/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    for (const bool legacyExport : {false, true})
    {
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(legacyExport ? builder.Build(root, {ExportMode::DB}, "", graph) : builder.Build(Request(3), graph));
        const auto& keys = builder.GetRuntimeNodeKeys();
        const auto host = keys.at({TopoNodeStage::HOST_PROCESSING, "HostC", "host"});
        for (const auto& scope : {"device_0", "device_1"})
        {
            for (const auto& name : {"Device.Init", "NpuMemParser", "NpuModuleMemParser", "DeviceTaskProcess"})
            {
                const auto key = keys.at({TopoNodeStage::DEVICE_PROCESSING, name, scope});
                const auto& dependencies = graph.at(key).processDependence;
                EXPECT_EQ(std::find(dependencies.begin(), dependencies.end(), host), dependencies.end()) << name;
            }
            for (const auto& name : {"LoadStreamExpandSpec", "LoadHostData", "CcuMissionParser"})
            {
                const auto entry = keys.find({TopoNodeStage::DEVICE_PROCESSING, name, scope});
                if (entry == keys.end()) { continue; }  // CCU is chip-specific.
                const auto& dependencies = graph.at(entry->second).processDependence;
                EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), host), dependencies.end()) << name;
            }
            const auto load = keys.at({TopoNodeStage::DEVICE_PROCESSING, "LoadHostData", scope});
            const auto model = keys.at({TopoNodeStage::DEVICE_PROCESSING,
                                       std::string(scope) == "device_0" ? "LogModelingV6" : "LogModeling", scope});
            const auto& dependencies = graph.at(load).processDependence;
            EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), model), dependencies.end());
            const auto taskInit = keys.at({TopoNodeStage::DEVICE_PROCESSING, "DeviceTaskProcess", scope});
            EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), taskInit), dependencies.end());
        }
    }
}

TEST_F(UnifiedPipelinePlanUTest, DeviceOnlyKeepsHostReadersWithoutSchedulingHost)
{
    FileWriter(root + "/device_0/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(2), graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::HOST_PROCESSING, "HostC", "host"}), 0U);
    for (const auto& name : {"LoadStreamExpandSpec", "LoadHostData", "CcuMissionParser"})
    {
        const auto key = keys.at({TopoNodeStage::DEVICE_PROCESSING, name, "device_0"});
        for (const auto dependency : graph.at(key).processDependence)
        {
            EXPECT_NE(graph.at(dependency).processName, "HostC");
        }
    }
}

TEST_F(UnifiedPipelinePlanUTest, LegacySingleDeviceStillBuildsWithoutHostLifecycleBindings)
{
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(never());
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(static_cast<bool (ProcessControl::*)()>(&ProcessControl::ExecuteProcess))
        .expects(once()).will(returnValue(true));
    MOCKER_CPP(&ProcessControl::GetExecuteStat).stubs().will(returnValue(ExecuteProcessStat{}));
    auto context = Domain::DeviceContext::Create(CHIP_V6_1_0);
    DataInventory inventory;
    std::string statistics;
    EXPECT_TRUE(Domain::RunSingleDevice(root + "/device_0", *context, inventory, statistics));
    EXPECT_FALSE(statistics.empty());
}

TEST_F(UnifiedPipelinePlanUTest, LegacySingleDeviceKeepsPersistenceFailurePolicyWithoutReaderPrecheck)
{
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    const auto id = TOPO_NODE(DEVICE_PROCESSING, "QosPersistence");
    const auto* registered = TopoNodeRegistry::Find(id);
    ASSERT_NE(registered, nullptr);
    auto persistence = *registered;
    // Isolate the real persistence node with no applicable reader or injected input.
    persistence.dependencyResolver = nullptr;
    TopoNodeRegistry::MutableDefinitions() = {{id, std::move(persistence)}};
    FileWriter(root + "/device_0/data/stars_soc_profile.0.slice_0").WriteText("raw");
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(exactly(2)).will(returnValue(true));

    for (const bool mandatory : {false, true})
    {
        SCOPED_TRACE(mandatory ? "mandatory persistence" : "optional persistence");
        TopoNodeRegistry::MutableDefinitions().at(id).mandatory = mandatory;
        auto context = Domain::DeviceContext::Create(CHIP_V4_1_0);
        DataInventory inventory;
        std::string statistics;
        EXPECT_EQ(Domain::RunSingleDevice(root + "/device_0", *context, inventory, statistics), !mandatory);
        EXPECT_NE(statistics.find("[device_0/QosPersistence]"), std::string::npos);
        EXPECT_NE(statistics.find("return: 0x1"), std::string::npos);
    }
}

TEST_F(UnifiedPipelinePlanUTest, IndependentDeviceParsersRunWhileHostIsStillRunning)
{
    for (const auto& scope : {"device_0", "device_1"})
    {
        FileWriter(root + "/" + scope + "/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(3), graph));
    std::mutex mutex;
    std::condition_variable changed;
    bool hostStarted = false;
    bool hostFinished = false;
    size_t independentParsers = 0;
    size_t hostReaders = 0;
    for (auto& entry : graph)
    {
        const auto name = entry.second.processName;
        entry.second.creator = [&, name]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([&, name](DataInventory&)
            {
                std::unique_lock<std::mutex> lock(mutex);
                if (name == "HostC")
                {
                    hostStarted = true;
                    changed.notify_all();
                    const bool overlapped = changed.wait_for(lock, std::chrono::seconds(5),
                                                              [&] { return independentParsers == 2; });
                    EXPECT_TRUE(overlapped);
                    hostFinished = true;
                }
                else if (name == "NpuMemParser")
                {
                    const bool started = changed.wait_for(lock, std::chrono::seconds(5), [&] { return hostStarted; });
                    EXPECT_TRUE(started);
                    EXPECT_FALSE(hostFinished);
                    ++independentParsers;
                    changed.notify_all();
                }
                else if (name == "LoadStreamExpandSpec" || name == "LoadHostData" || name == "CcuMissionParser")
                {
                    EXPECT_TRUE(hostFinished) << name;
                    ++hostReaders;
                }
                return true;
            }));
        };
    }
    ProcessControlOptions options;
    options.maxWorkerThreads = 4;
    options.maxActiveDevices = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(independentParsers, 2U);
    EXPECT_EQ(hostReaders, 6U);
}

TEST_F(UnifiedPipelinePlanUTest, HostFailureBlocksItsReadersButIndependentDevicesFinishAndReleaseSlots)
{
    for (const auto& scope : {"device_0", "device_1"})
    {
        FileWriter(root + "/" + scope + "/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(3), graph));
    for (auto& entry : graph)
    {
        const bool host = entry.second.processName == "HostC";
        entry.second.creator = [host]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([host](DataInventory&) { return !host; }));
        };
    }
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    options.maxActiveDevices = 1;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    const auto& keys = builder.GetRuntimeNodeKeys();
    for (const auto& scope : {"device_0", "device_1"})
    {
        for (const auto& name : {"Device.Init", "NpuMemParser", "NpuMemPersistence", "CcuChannelParser"})
        {
            EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, name, scope})).state,
                      RuntimeProcessState::SUCCEEDED) << name;
        }
        for (const auto& name : {"LoadStreamExpandSpec", "LoadHostData", "CcuMissionParser", "Device.Ready"})
        {
            EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, name, scope})).state,
                      RuntimeProcessState::BLOCKED) << name;
        }
    }
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::FLOW_CONTROL, "Pipeline.Done", "profile"})).state,
              RuntimeProcessState::SUCCEEDED);
}

TEST_F(UnifiedPipelinePlanUTest, SelectedSummaryExpandsOnlyRequiredDeviceChain)
{
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(StepTraceRequest(), context, definitions, roots, graph, builder));
    std::map<std::string, std::set<std::string>> names;
    for (const auto& node : graph) {
        names[node.second.scope].insert(node.second.processName);
        EXPECT_NE(node.second.processName, "DeviceC");
        EXPECT_NE(node.second.processName, "Parse.Ready");
        EXPECT_NE(node.second.processName, "DB.Done");
        EXPECT_NE(node.second.processName, "Timeline.Done");
    }
    const std::set<std::string> expected{"Device.Init", "LoadStreamExpandSpec", "TsTrackParser",
                                         "StepTraceProcess", "TracePersistence", "Device.Ready"};
    EXPECT_EQ(names["device_0"], expected);
    EXPECT_EQ(names["device_1"], expected);
    EXPECT_EQ(names["host"], (std::set<std::string>{"HostC"}));
    EXPECT_EQ(names["export"], (std::set<std::string>{"Export.Init", "STEP_TRACE", "Export.Finalize"}));
    EXPECT_NE(context.scopeContexts.at("device_0"), context.scopeContexts.at("device_1"));
    EXPECT_NE(context.scopeInventories.at("device_0"), context.scopeInventories.at("device_1"));
    const auto processor = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DATA_PROCESSING, "STEP_TRACE", "export"});
    for (const auto& scope : {"device_0", "device_1"}) {
        const auto persistence = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope});
        const auto& deps = graph.at(processor).processDependence;
        EXPECT_NE(std::find(deps.begin(), deps.end(), persistence), deps.end());
    }
    EXPECT_FALSE(File::Exist(File::PathJoin({root, "mindstudio_profiler_output"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({root, "device_0", "sqlite"})));
}

TEST_F(UnifiedPipelinePlanUTest, DeviceCompletionWaitsOnlyForItsOwnSelectedOperations)
{
    EXPECT_EQ(TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PROCESSING, "DeviceC")), nullptr);
    const auto* init = TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PROCESSING, "Device.Init"));
    ASSERT_NE(init, nullptr);
    EXPECT_TRUE(init->opensDeviceScope);
    EXPECT_FALSE(init->closesDeviceScope);
    const auto count = TopoNodeRegistry::GetDefinitions().size();
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(StepTraceRequest(), context, definitions, roots, graph, builder));
    EXPECT_EQ(TopoNodeRegistry::GetDefinitions().size(), count);
    for (const auto& scope : {"device_0", "device_1"}) {
        const auto ready = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope});
        const auto optional = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "StepTraceProcess", scope});
        const auto consumer = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope});
        EXPECT_TRUE(graph.at(ready).closesDeviceScope);
        EXPECT_EQ(graph.at(ready).completionDependence, (std::vector<RuntimeProcessKey>{optional}));
        const auto& businessDependencies = graph.at(consumer).processDependence;
        EXPECT_NE(std::find(businessDependencies.begin(), businessDependencies.end(), optional), businessDependencies.end());
        for (auto dependency : graph.at(ready).processDependence) { EXPECT_EQ(graph.at(dependency).scope, scope); }
    }
}

TEST_F(UnifiedPipelinePlanUTest, SelectedChainRunsWithOneActiveDeviceAndNoNestedExecutor)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(once()).will(returnValue(ANALYSIS_OK));
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(exactly(2)).will(returnValue(true));
    MOCKER(Domain::RunSingleDevice).expects(never());
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(StepTraceRequest(), graph));
    std::atomic<size_t> persisted{0};
    std::atomic<size_t> processed{0};
    const auto processor = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DATA_PROCESSING, "STEP_TRACE", "export"});
    for (auto& node : graph) {
        const auto name = node.second.processName;
        if (node.second.scope.find("device_") == 0 && name != "Device.Init" && name != "Device.Ready") {
            node.second.creator = [&, name]() -> std::unique_ptr<Process> {
                return std::unique_ptr<Process>(new TopoCallbackProcess([&, name](DataInventory&) {
                    if (name == "TracePersistence") { ++persisted; }
                    return true;
                }));
            };
        }
        if (node.second.scope == "export") {
            const bool isProcessor = node.first == processor;
            node.second.creator = [&, isProcessor]() -> std::unique_ptr<Process> {
                return std::unique_ptr<Process>(new TopoCallbackProcess([&, isProcessor](DataInventory&) {
                    if (isProcessor) { EXPECT_EQ(persisted.load(), 2U); ++processed; }
                    return true;
                }));
            };
        }
    }
    ProcessControlOptions options;
    options.maxActiveDevices = 1;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(processed.load(), 1U);
    const auto results = executor.GetRuntimeResults();
    for (const auto& scope : {"device_0", "device_1"})
    {
        const auto ready = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope});
        EXPECT_EQ(results.at(ready).state, RuntimeProcessState::SUCCEEDED);
    }
    ExpectNoUnifiedMarkers();
}

TEST_F(UnifiedPipelinePlanUTest, BlockedDeviceReadyReleasesSingleSlotWithoutWritingMarkers)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(once()).will(returnValue(ANALYSIS_OK));
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(exactly(2)).will(returnValue(true));
    MOCKER(Domain::RunSingleDevice).expects(never());
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(StepTraceRequest(), graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto failed = keys.at({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", "device_0"});
    const auto next = keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Init", "device_1"});
    // Make device_0 acquire the only slot first, without requiring its success for device_1.
    graph.at(next).completionDependence = {failed};
    std::atomic<size_t> persisted{0};
    for (auto& node : graph)
    {
        const auto name = node.second.processName;
        if (node.second.scope.find("device_") != 0 || name == "Device.Init" || name == "Device.Ready")
        {
            continue;
        }
        const bool fail = node.first == failed;
        node.second.creator = [&, name, fail]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([&, name, fail](DataInventory&)
            {
                if (fail) { return false; }
                if (name == "TracePersistence") { ++persisted; }
                return true;
            }));
        };
    }
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    options.maxActiveDevices = 1;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    EXPECT_EQ(persisted.load(), 1U);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", "device_0"})).state,
              RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", "device_1"})).state,
              RuntimeProcessState::SUCCEEDED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::FLOW_CONTROL, "Pipeline.Done", "profile"})).state,
              RuntimeProcessState::SUCCEEDED);
    EXPECT_EQ(results.at(keys.at({TopoNodeStage::FLOW_CONTROL, "Export.Finalize", "export"})).state,
              RuntimeProcessState::BLOCKED);
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite/.unified.complete"));
    EXPECT_FALSE(File::Exist(root + "/device_1/sqlite/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/device_1/sqlite/.unified.complete"));
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, RequestBuildIncludesParsingAndExportsWithoutExecutingNodes)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(never());
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());
    const auto registrationCount = TopoNodeRegistry::GetDefinitions().size();
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(StepTraceRequest(), graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::HOST_PROCESSING, "HostC", "host"}), 1U);
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "HcclCalculator", scope}), 0U);
    }
    EXPECT_EQ(keys.count({TopoNodeStage::SUMMARY_GENERATION, "STEP_TRACE", "export"}), 1U);
    EXPECT_EQ(TopoNodeRegistry::GetDefinitions().size(), registrationCount);
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExistingDataExportUsesSameBuilderWithoutParsingOrImplicitDatabase)
{
    auto request = StepTraceRequest();
    request.flags = 0x10;
    // Existing-data export does not inspect raw Device metadata or schedule its parsers.
    FileWriter(root + "/device_0/info.json").WriteText("invalid metadata");
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(request, graph));
    for (const auto& node : builder.GetRuntimeNodeKeys())
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::HOST_PROCESSING);
        EXPECT_NE(node.first.stage, TopoNodeStage::DEVICE_PROCESSING);
        EXPECT_NE(node.first.stage, TopoNodeStage::DATABASE_PERSISTENCE);
    }
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::SUMMARY_GENERATION, "STEP_TRACE", "export"}), 1U);
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
}

TEST_F(UnifiedPipelinePlanUTest, DbOnlyExportWritesDatabaseWithoutFileOutputDirectory)
{
    const std::vector<std::pair<bool, int>> cases{
        {false, PIPELINE_DB}, {false, PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA | PIPELINE_DB}, {true, PIPELINE_DB}};
    for (const auto& entry : cases)
    {
        SCOPED_TRACE(testing::Message() << "legacyExport=" << entry.first << ", flags=" << entry.second);
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(entry.first ? builder.Build(root, {ExportMode::DB}, "", graph)
                                : builder.Build(Request(entry.second), graph));
        for (auto& node : graph)
        {
            if (node.second.processName == "Export.Init" || node.second.processName == "DB:ENUM")
            {
                continue;
            }
            node.second.creator = []() -> std::unique_ptr<Process>
            { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
        }
        ASSERT_TRUE(ExecuteTopoGraph(builder, graph, root));
        EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
        const auto files = File::GetFilesWithPrefix(root, "msprof_");
        ASSERT_EQ(files.size(), 1U);
        DBRunner db(files.front());
        EXPECT_TRUE(db.CheckTableExists(TABLE_NAME_ENUM_API_TYPE));
        EXPECT_TRUE(File::DeleteFile(files.front()));
    }
}

TEST_F(UnifiedPipelinePlanUTest, TimelineAndSummaryInitializationStillCreatesFileOutputDirectory)
{
    for (const int flags : {PIPELINE_TIMELINE, PIPELINE_SUMMARY, PIPELINE_DB | PIPELINE_TIMELINE,
                           PIPELINE_DB | PIPELINE_SUMMARY, PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY})
    {
        SCOPED_TRACE(flags);
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(Request(flags), graph));
        const auto init = builder.GetRuntimeNodeKeys().at({TopoNodeStage::FLOW_CONTROL, "Export.Init", "export"});
        auto process = graph.at(init).creator();
        ASSERT_NE(process, nullptr);
        DataInventory inventory;
        Infra::Context context;
        EXPECT_EQ(process->Run(inventory, context), ANALYSIS_OK);
        const auto output = root + "/mindstudio_profiler_output";
        EXPECT_TRUE(File::Exist(output));
        EXPECT_TRUE(File::RemoveDir(output, 0));
    }
}

TEST_F(UnifiedPipelinePlanUTest, DbOnlyInitializationPreservesExistingFileOutputDirectory)
{
    const auto output = root + "/mindstudio_profiler_output";
    ASSERT_TRUE(File::CreateDir(output));
    const auto existing = File::PathJoin({output, "existing.csv"});
    FileWriter(existing).WriteText("value\n7\n");
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(PIPELINE_DB), graph));
    const auto init = builder.GetRuntimeNodeKeys().at({TopoNodeStage::FLOW_CONTROL, "Export.Init", "export"});
    auto process = graph.at(init).creator();
    ASSERT_NE(process, nullptr);
    DataInventory inventory;
    Infra::Context context;
    EXPECT_EQ(process->Run(inventory, context), ANALYSIS_OK);
    std::vector<std::string> content;
    ASSERT_EQ(FileReader(existing).ReadText(content), ANALYSIS_OK);
    EXPECT_EQ(content, (std::vector<std::string>{"value", "7"}));
}

TEST_F(UnifiedPipelinePlanUTest, DatabaseSelectionDoesNotDependOnReportPath)
{
    auto request = Request(0x07);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(request, graph));
    std::map<std::string, std::set<std::string>> withoutReports;
    for (const auto& node : graph)
    {
        withoutReports[node.second.scope].insert(node.second.processName);
    }
    // DB roots are complete and do not consume a Timeline/Summary report configuration.
    request.reportsJson = root + "/unused_reports.json";
    ASSERT_TRUE(builder.Build(request, graph));
    std::map<std::string, std::set<std::string>> withReports;
    for (const auto& node : graph)
    {
        withReports[node.second.scope].insert(node.second.processName);
    }
    EXPECT_EQ(withReports, withoutReports);
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, SummaryWithoutReportsDoesNotAddDatabaseOrUnrelatedDeviceOperations)
{
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(0x13), context, definitions, roots, graph, builder));
    EXPECT_EQ(context.dbSession, nullptr);
    for (const auto& node : builder.GetRuntimeNodeKeys())
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::DATABASE_PERSISTENCE);
    }
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}), 1U);
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", scope}), 0U);
    }
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, PlanPreparesSessionsWithoutFilesystemWritesOrExecution)
{
    const auto request = StepTraceRequest();
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(never());
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());
    MOCKER_CPP(&File::CreateDir).expects(never());
    ExportManager manager(root, request.reportsJson);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB, ExportMode::SUMMARY}, context, roots));
    EXPECT_NE(context.dbSession, nullptr);
    EXPECT_EQ(context.timelineSession, nullptr);
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
    EXPECT_TRUE(File::GetFilesWithPrefix(root, "msprof_").empty());
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
}

TEST_F(UnifiedPipelinePlanUTest, PlanRejectsInvalidReportsBeforePreparingSessions)
{
    const auto request = StepTraceRequest();
    FileWriter(request.reportsJson).WriteText("{\"summary_process\":{\"unknown_deliverable\":true}}");
    ExportManager manager(root, request.reportsJson);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(manager.Plan({ExportMode::DB, ExportMode::SUMMARY}, context, roots));
    EXPECT_EQ(context.dbSession, nullptr);
    EXPECT_EQ(context.timelineSession, nullptr);
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportManagerRunKeepsPlannedSummaryAndExistingDatabaseSkip)
{
    MOCKER(IsTopoGraphDumpEnabled).stubs().will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());
    const auto request = StepTraceRequest();
    {
        DBRunner database(root + "/msprof_123.db");
        ASSERT_TRUE(database.CreateTable("ExistingExport", {{"value", "INTEGER"}}));
    }
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        DBRunner database(File::PathJoin({sqlite, "existing.db"}));
        ASSERT_TRUE(database.CreateTable("ParsedData", {{"value", "INTEGER"}}));
        FileWriter(File::PathJoin({sqlite, "optional.db"})).WriteText("");
        DBRunner emptyDatabase(File::PathJoin({sqlite, "empty_schema.db"}));
        ASSERT_TRUE(emptyDatabase.UpdateData("PRAGMA user_version = 1"));
    }
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    // Isolate business data processing while exercising the public entry, graph, initialization and finalization.
    for (const auto stage : {TopoNodeStage::DATA_PROCESSING, TopoNodeStage::SUMMARY_GENERATION})
    {
        auto& definition = TopoNodeRegistry::MutableDefinitions().at({stage, "STEP_TRACE"});
        definition.inputDataTypes.clear();
        definition.creatorFactory = [](const TopoBuildContext&) -> ProcessCreator
        {
            return []() -> std::unique_ptr<Process>
            { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
        };
    }
    ExportManager manager(root, request.reportsJson);
    const std::set<ExportMode> modes{ExportMode::DB, ExportMode::SUMMARY};
    TopoBuildContext plannedContext;
    std::vector<TopoNodeId> plannedRoots;
    ASSERT_TRUE(manager.Plan(modes, plannedContext, plannedRoots));
    ASSERT_EQ(plannedRoots.size(), 1U);
    EXPECT_EQ(plannedRoots.front().stage, TopoNodeStage::SUMMARY_GENERATION);
    EXPECT_EQ(plannedRoots.front().name, "STEP_TRACE");
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
    ASSERT_TRUE(manager.Run(modes));
    EXPECT_TRUE(File::Exist(root + "/mindstudio_profiler_log/unified_pipeline_graph.dot"));
    nlohmann::json report;
    FileReader reader(root + "/mindstudio_profiler_log/unified_pipeline_graph.json");
    ASSERT_EQ(reader.ReadJson(report), ANALYSIS_OK);
    bool summaryFound = false;
    size_t hostNodes = 0;
    size_t deviceNodes = 0;
    for (const auto& node : report.at("nodes"))
    {
        EXPECT_EQ(node.at("state"), "SUCCEEDED");
        const auto stage = node.at("stage").get<uint8_t>();
        hostNodes += stage == static_cast<uint8_t>(TopoNodeStage::HOST_PROCESSING);
        if (stage == static_cast<uint8_t>(TopoNodeStage::DEVICE_PROCESSING))
        {
            ++deviceNodes;
            EXPECT_TRUE(node.at("name") == "Device.Init" || node.at("name") == "Device.Ready");
        }
        EXPECT_NE(stage, static_cast<uint8_t>(TopoNodeStage::DATABASE_PERSISTENCE));
        summaryFound = summaryFound || stage == static_cast<uint8_t>(TopoNodeStage::SUMMARY_GENERATION);
    }
    EXPECT_TRUE(summaryFound);
    EXPECT_EQ(hostNodes, 1U);
    EXPECT_EQ(deviceNodes, 4U);
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/host/sqlite/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite/.unified.incomplete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportManagerRunRecordsFailedExecutionBeforeReturningFalse)
{
    MOCKER(IsTopoGraphDumpEnabled).stubs().will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    const auto request = StepTraceRequest();
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        DBRunner database(File::PathJoin({sqlite, "existing.db"}));
        ASSERT_TRUE(database.CreateTable("ParsedData", {{"value", "INTEGER"}}));
    }
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    for (const auto stage : {TopoNodeStage::DATA_PROCESSING, TopoNodeStage::SUMMARY_GENERATION})
    {
        auto& definition = TopoNodeRegistry::MutableDefinitions().at({stage, "STEP_TRACE"});
        definition.inputDataTypes.clear();
        definition.creatorFactory = [stage](const TopoBuildContext&) -> ProcessCreator
        {
            return [stage]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess(
                    [stage](DataInventory&) { return stage != TopoNodeStage::SUMMARY_GENERATION; }));
            };
        };
    }

    ExportManager manager(root, request.reportsJson);
    EXPECT_FALSE(manager.Run({ExportMode::SUMMARY}));
    nlohmann::json report;
    FileReader reader(root + "/mindstudio_profiler_log/unified_pipeline_graph.json");
    ASSERT_EQ(reader.ReadJson(report), ANALYSIS_OK);
    bool failedSummary = false;
    bool blockedFinalize = false;
    for (const auto& node : report.at("nodes"))
    {
        if (node.at("stage") == static_cast<uint8_t>(TopoNodeStage::SUMMARY_GENERATION))
        {
            failedSummary = node.at("state") == "FAILED" && node.at("return_code") != ANALYSIS_OK;
        }
        if (node.at("name") == "Export.Finalize")
        {
            blockedFinalize = node.at("state") == "BLOCKED";
        }
    }
    EXPECT_TRUE(failedSummary);
    EXPECT_TRUE(blockedFinalize);
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportManagerContinuesIndependentSummaryAfterProcessorFailure)
{
    MOCKER(IsTopoGraphDumpEnabled).stubs().will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    const auto request = StepTraceRequest();
    FileWriter(request.reportsJson).WriteText(
        "{\"summary_process\":{\"step_trace\":true,\"npu_memory\":true}}");
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        DBRunner database(File::PathJoin({sqlite, "existing.db"}));
        ASSERT_TRUE(database.CreateTable("ParsedData", {{"value", "INTEGER"}}));
    }
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    for (const auto stage : {TopoNodeStage::DATA_PROCESSING, TopoNodeStage::SUMMARY_GENERATION})
    {
        for (const std::string name : {"STEP_TRACE", "NPU_MEM"})
        {
            auto& definition = TopoNodeRegistry::MutableDefinitions().at({stage, name});
            definition.inputDataTypes.clear();
            if (stage == TopoNodeStage::DATA_PROCESSING)
            {
                definition.processDependencies.clear();
                definition.dependencyResolver = nullptr;
            }
            const auto artifact = root + "/independent_summary.csv";
            definition.creatorFactory = [stage, name, artifact](const TopoBuildContext&) -> ProcessCreator
            {
                return [stage, name, artifact]() -> std::unique_ptr<Process>
                {
                    return std::unique_ptr<Process>(new TopoCallbackProcess(
                        [stage, name, artifact](DataInventory&)
                        {
                            if (name == "STEP_TRACE") { return false; }
                            if (stage == TopoNodeStage::SUMMARY_GENERATION)
                            {
                                FileWriter writer(artifact);
                                writer.WriteText("value\n1\n");
                                return writer.IsGood();
                            }
                            return true;
                        }));
                };
            };
        }
    }
    auto& independent = TopoNodeRegistry::MutableDefinitions().at({TopoNodeStage::SUMMARY_GENERATION, "NPU_MEM"});
    independent.completionDependencies = {{TopoNodeStage::DATA_PROCESSING, "STEP_TRACE"}};
    ExportManager manager(root, request.reportsJson);
    EXPECT_FALSE(manager.Run({ExportMode::SUMMARY}));
    EXPECT_TRUE(File::Exist(root + "/independent_summary.csv"));
    nlohmann::json report;
    FileReader reader(root + "/mindstudio_profiler_log/unified_pipeline_graph.json");
    ASSERT_EQ(reader.ReadJson(report), ANALYSIS_OK);
    std::map<std::pair<int, std::string>, std::string> states;
    for (const auto& node : report.at("nodes"))
    {
        states[{node.at("stage").get<int>(), node.at("name").get<std::string>()}] =
            node.at("state").get<std::string>();
    }
    EXPECT_EQ(states[std::make_pair(static_cast<int>(TopoNodeStage::DATA_PROCESSING), "STEP_TRACE")], "FAILED");
    EXPECT_EQ(states[std::make_pair(static_cast<int>(TopoNodeStage::SUMMARY_GENERATION), "STEP_TRACE")], "BLOCKED");
    EXPECT_EQ(states[std::make_pair(static_cast<int>(TopoNodeStage::SUMMARY_GENERATION), "NPU_MEM")], "SUCCEEDED");
    EXPECT_EQ(states[std::make_pair(static_cast<int>(TopoNodeStage::FLOW_CONTROL), "Pipeline.Done")], "SUCCEEDED");
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, GraphDumpIsDisabledByDefaultWithoutChangingExecutionOrStatistics)
{
    EXPECT_FALSE(IsTopoGraphDumpEnabled());
    MOCKER(DescribeTopoGraph).expects(never());
    MOCKER(RecordProcessStat).expects(exactly(2)).will(invoke(
        +[](const ExecuteProcessStat& stat, const std::string& path, std::string& text)
        {
            EXPECT_FALSE(stat.allLevelStat.empty());
            EXPECT_FALSE(path.empty());
            text = "process statistics retained";
        }));
    for (const bool processSuccess : {true, false})
    {
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(Request(1), graph));
        for (auto& entry : graph)
        {
            const bool host = entry.second.processName == "HostC";
            entry.second.creator = [host, processSuccess]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess([host, processSuccess](DataInventory&)
                {
                    return !host || processSuccess;
                }));
            };
        }
        EXPECT_EQ(ExecuteTopoGraph(builder, graph, root), processSuccess);
        EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_log/unified_pipeline_graph.json"));
        EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_log/unified_pipeline_graph.dot"));
    }
}

TEST_F(UnifiedPipelinePlanUTest, ExecuteTopoGraphContinuesIndependentWorkAndKeepsBooleanFailure)
{
    MOCKER(IsTopoGraphDumpEnabled).stubs().will(returnValue(true));
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    TopoNodeCollection definitions;
    const TopoNodeId failed{TopoNodeStage::DATA_PROCESSING, "Failed", "export"};
    const TopoNodeId blocked{TopoNodeStage::SUMMARY_GENERATION, "Blocked", "export"};
    const TopoNodeId good{TopoNodeStage::SUMMARY_GENERATION, "Good", "export"};
    std::atomic<size_t> completed{0};
    for (const auto& id : {failed, blocked, good})
    {
        TopoNodeDefinition definition;
        definition.id = id;
        definition.mandatory = true;
        definition.creatorFactory = [&, id](const TopoBuildContext&)
        {
            return [&, id]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess([&, id](DataInventory&)
                {
                    if (id == good) { ++completed; }
                    return !(id == failed);
                }));
            };
        };
        if (id == blocked) { definition.processDependencies = {failed}; }
        if (id == good) { definition.completionDependencies = {failed}; }
        definitions.emplace(id, std::move(definition));
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {blocked, good}, graph, &definitions));
    EXPECT_FALSE(ExecuteTopoGraph(builder, graph, root));
    EXPECT_EQ(completed.load(), 1U);
    nlohmann::json report;
    FileReader reader(root + "/mindstudio_profiler_log/unified_pipeline_graph.json");
    ASSERT_EQ(reader.ReadJson(report), ANALYSIS_OK);
    for (const auto& node : report.at("nodes"))
    {
        if (node.at("name") == "Blocked")
        {
            EXPECT_EQ(node.at("state"), "BLOCKED");
            EXPECT_EQ(node.at("failed_dependencies"),
                      nlohmann::json::array({builder.GetRuntimeNodeKeys().at(failed)}));
        }
        if (node.at("name") == "Good") { EXPECT_EQ(node.at("state"), "SUCCEEDED"); }
    }
}

TEST_F(UnifiedPipelinePlanUTest, IndependentExportNodesPrepareConcurrentlyAndFinalizeSharedOutputs)
{
    using IdPool = Analysis::Application::Credential::IdPool;
    struct IdPoolGuard
    {
        ~IdPoolGuard() { IdPool::GetInstance().Clear(); }
    } poolGuard;
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(PIPELINE_DB | PIPELINE_TIMELINE), context, definitions, roots, graph, builder));
    ASSERT_NE(context.dbSession, nullptr);
    const auto dbPath = context.dbSession->msprofDB_.dbRunner->GetPath();
    std::map<RuntimeProcessKey, uint32_t> targets;
    size_t savers = 0;
    size_t assemblers = 0;
    for (const auto& entry : builder.GetRuntimeNodeKeys())
    {
        if (entry.first.stage == TopoNodeStage::DATABASE_PERSISTENCE && savers < 2)
        {
            targets.emplace(entry.second, ++savers);
        }
        if (entry.first.stage == TopoNodeStage::TIMELINE_EXPORT && assemblers < 2)
        {
            targets.emplace(entry.second, 2 + ++assemblers);
        }
    }
    ASSERT_EQ(savers, 2U);
    ASSERT_EQ(assemblers, 2U);
    std::mutex mutex;
    std::condition_variable changed;
    size_t preparing = 0;
    const size_t eventsPerAssembler = 64;
    std::set<std::string> expectedRows;
    for (auto& entry : graph)
    {
        const auto target = targets.find(entry.first);
        if (target == targets.end())
        {
            // Preserve real initialization, STRING_IDS and output finalizers, while isolating
            // the export consumers from parser/processor input requirements.
            if (entry.second.processName == "Environment.Load" || entry.second.processName == "Export.Init" ||
                entry.second.processName == "Export.Finalize" || entry.second.processName == "Pipeline.Done" ||
                entry.second.processName == "DB:STRING_IDS" || entry.second.processName.find("TIMELINE:") == 0)
            {
                continue;
            }
            entry.second.creator = []() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; }));
            };
            continue;
        }
        const auto id = target->second;
        const auto name = entry.second.processName;
        if (id <= 2) { expectedRows.insert(name); }
        entry.second.creator = [&, id, name]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([&, id, name](DataInventory&)
            {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    ++preparing;
                    changed.notify_all();
                    // Both savers and both assemblers must enter before any starts writing.
                    // A whole-node resource reservation makes this rendezvous time out.
                    if (!changed.wait_for(lock, std::chrono::seconds(5), [&] { return preparing == targets.size(); }))
                    {
                        ADD_FAILURE() << "Independent export nodes did not prepare concurrently: " << name;
                        return false;
                    }
                }
                if (id <= 2)
                {
                    const auto stringId = IdPool::GetInstance().GetUint64Id(name);
                    return DeliverableConsumer::Instance().InsertRows(
                        dbPath, "parallel_rows", {{"string_id", "INTEGER"}, {"name", "TEXT"}},
                        std::vector<std::tuple<uint64_t, std::string>>{{stringId, name}});
                }
                JsonEventSink sink;
                sink.Open({{MSPROF_JSON_FILE, FileCategory::MSPROF}}, root);
                for (size_t i = 0; i < eventsPerAssembler; ++i)
                {
                    sink.push_back(std::make_shared<Domain::DurationEvent>(id, 1, 1.5, "1.0", name));
                }
                sink.Flush();
                sink.Sync();
                return DeliverableConsumer::Instance().Ok();
            }));
        };
    }
    // Exercise four concurrent nodes even on hosts with fewer than four logical CPUs.
    ProcessControlOptions options;
    options.maxWorkerThreads = 4;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    auto& consumer = DeliverableConsumer::Instance();
    consumer.ResetOk();
    ProcessControl executor(graph, options);
    const bool success = executor.ExecuteProcess();
    consumer.FinishTimeline();
    consumer.CloseAllDb();
    ASSERT_TRUE(success && consumer.Ok());
    EXPECT_EQ(preparing, 4U);
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
    EXPECT_FALSE(DeliverableConsumer::Instance().HasDbWriterForTest(dbPath));
    DBRunner database(dbPath);
    std::vector<std::tuple<std::string, std::string>> rows;
    ASSERT_TRUE(database.QueryData("SELECT r.name, s.value FROM parallel_rows r "
                                   "JOIN STRING_IDS s ON r.string_id = s.id", rows));
    ASSERT_EQ(rows.size(), 2U);
    std::set<std::string> actualRows;
    for (const auto& row : rows)
    {
        EXPECT_EQ(std::get<0>(row), std::get<1>(row));
        actualRows.insert(std::get<0>(row));
    }
    EXPECT_EQ(actualRows, expectedRows);
    const auto files = File::GetOriginData(root + "/mindstudio_profiler_output", {MSPROF_JSON_FILE}, {});
    ASSERT_EQ(files.size(), 1U);
    nlohmann::json events;
    ASSERT_EQ(FileReader(files.front()).ReadJson(events), ANALYSIS_OK);
    ASSERT_TRUE(events.is_array());
    ASSERT_EQ(events.size(), eventsPerAssembler * 2);
    std::map<uint32_t, size_t> counts;
    for (const auto& event : events) { ++counts[event.at("pid").get<uint32_t>()]; }
    EXPECT_EQ(counts.size(), 2U);
    EXPECT_EQ(counts[3], eventsPerAssembler);
    EXPECT_EQ(counts[4], eventsPerAssembler);
}

TEST_F(UnifiedPipelinePlanUTest, TimelineControlNodesRunAfterInjectedExportInitializationDependencyFails)
{
    MOCKER(IsTopoGraphDumpEnabled).stubs().will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    auto request = Request(0x08);
    request.reportsJson = root + "/reports.json";
    FileWriter(request.reportsJson).WriteText("{\"json_process\":{\"npu_mem\":true}}");
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(request, graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto init = keys.at({TopoNodeStage::FLOW_CONTROL, "Export.Init", "export"});
    const auto output = root + "/mindstudio_profiler_output";
    graph.at(init).creator = [output]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new TopoCallbackProcess([output](DataInventory&)
        {
            EXPECT_TRUE(File::CreateDir(output));
            return false;
        }));
    };
    for (const auto name : {"TIMELINE:PRE_DUMP", "TIMELINE:POST_DUMP"})
    {
        const auto key = keys.at({TopoNodeStage::FLOW_CONTROL, name, "export"});
        EXPECT_TRUE(graph.at(key).runOnDependencyFailure);
        const auto& dependencies = graph.at(key).processDependence;
        EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), init), dependencies.end());
    }
    EXPECT_FALSE(ExecuteTopoGraph(builder, graph, root));
    nlohmann::json report;
    FileReader reader(root + "/mindstudio_profiler_log/unified_pipeline_graph.json");
    ASSERT_EQ(reader.ReadJson(report), ANALYSIS_OK);
    std::map<std::string, std::string> states;
    for (const auto& node : report.at("nodes"))
    {
        const auto name = node.at("name").get<std::string>();
        states[name] = node.at("state").get<std::string>();
        if (name == "TIMELINE:PRE_DUMP" || name == "TIMELINE:POST_DUMP")
        {
            EXPECT_EQ(node.at("run_on_dependency_failure"), true);
        }
    }
    EXPECT_EQ(states["Export.Init"], "FAILED");
    EXPECT_EQ(states["TIMELINE:PRE_DUMP"], "SUCCEEDED");
    EXPECT_EQ(states["TIMELINE:POST_DUMP"], "SUCCEEDED");
    EXPECT_EQ(states["NPU MEM"], "BLOCKED");
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportBuildReusesPreparedEnvironmentAndSessions)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(never());
    ASSERT_TRUE(File::RemoveDir(root + "/host", 0));
    TopoBuildContext context;
    context.profPath = root;
    context.outputPath = root + "/mindstudio_profiler_output";
    context.environmentLoaded = true;
    context.dbSession = std::make_shared<DBAssembler>(root, root);
    context.timelineSession = std::make_shared<TimelineManager>(root, context.outputPath);
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    std::vector<TopoNodeId> roots;
    for (const auto stage : {TopoNodeStage::DATABASE_PERSISTENCE, TopoNodeStage::TIMELINE_EXPORT})
    {
        TopoNodeDefinition definition;
        definition.id = {stage, "PreparedSession"};
        definition.mandatory = true;
        const auto database = context.dbSession;
        const auto timeline = context.timelineSession;
        definition.creatorFactory = [database, timeline](const TopoBuildContext& bindings) -> ProcessCreator
        {
            const auto boundDatabase = bindings.dbSession;
            const auto boundTimeline = bindings.timelineSession;
            return [database, timeline, boundDatabase, boundTimeline]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess(
                    [database, timeline, boundDatabase, boundTimeline](DataInventory&)
                    { return boundDatabase == database && boundTimeline == timeline; }));
            };
        };
        roots.push_back(definition.id);
        ASSERT_TRUE(TopoNodeRegistry::Register(std::move(definition)));
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.BuildExport(context, roots, graph));
    EXPECT_FALSE(File::Exist(context.outputPath));
    EXPECT_TRUE(ExecuteTopoGraph(builder, graph, root));
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportEntryBuildDiscoversHostAndAllDeviceScopes)
{
    const auto request = StepTraceRequest();
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(root, {ExportMode::SUMMARY}, request.reportsJson, graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::HOST_PROCESSING, "HostC", "host"}), 1U);
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TsTrackParser", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "HcclCalculator", scope}), 0U);
    }
    EXPECT_EQ(keys.count({TopoNodeStage::SUMMARY_GENERATION, "STEP_TRACE", "export"}), 1U);
    for (const auto& node : keys)
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::DATABASE_PERSISTENCE);
    }
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportBuildReusesParsedDataWithEmptyOptionalDatabases)
{
    const auto request = StepTraceRequest();
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        DBRunner database(File::PathJoin({sqlite, "existing.db"}));
        ASSERT_TRUE(database.CreateTable("ParsedData", {{"value", "INTEGER"}}));
        FileWriter(File::PathJoin({sqlite, "optional.db"})).WriteText("");
        DBRunner emptyDatabase(File::PathJoin({sqlite, "empty_schema.db"}));
        ASSERT_TRUE(emptyDatabase.UpdateData("PRAGMA user_version = 1"));
    }
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(request, context, definitions, roots, graph, builder));
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        EXPECT_TRUE(context.scopeReuse.at(scope));
    }
    ASSERT_TRUE(builder.Build(root, {ExportMode::SUMMARY}, request.reportsJson, graph));
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TsTrackParser", scope}), 0U);
    }
}

TEST_F(UnifiedPipelinePlanUTest, EmptyIntermediateDatabasesDoNotMarkScopeAsParsed)
{
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        FileWriter(File::PathJoin({sqlite, "optional.db"})).WriteText("");
        DBRunner emptyDatabase(File::PathJoin({sqlite, "empty_schema.db"}));
        ASSERT_TRUE(emptyDatabase.UpdateData("PRAGMA user_version = 1"));
    }
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(StepTraceRequest(), context, definitions, roots, graph, builder));
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        EXPECT_FALSE(context.scopeReuse.at(scope));
    }
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TsTrackParser", "device_0"}), 1U);
}

TEST_F(UnifiedPipelinePlanUTest, CorruptedIntermediateDatabaseIsRejected)
{
    const auto sqlite = root + "/device_0/sqlite";
    ASSERT_TRUE(File::CreateDir(sqlite));
    FileWriter(sqlite + "/broken.db").WriteText("not a SQLite database");
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(StepTraceRequest(), graph));
    EXPECT_TRUE(graph.empty());
}

TEST_F(UnifiedPipelinePlanUTest, EmptyFinalExportDatabaseIsRejected)
{
    const auto path = root + "/msprof_123.db";
    FileWriter(path).WriteText("");
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(StepTraceRequest(), graph));
    EXPECT_TRUE(graph.empty());
    ASSERT_TRUE(File::DeleteFile(path));
    DBRunner emptyDatabase(path);
    ASSERT_TRUE(emptyDatabase.UpdateData("PRAGMA user_version = 1"));
    EXPECT_FALSE(builder.Build(StepTraceRequest(), graph));
    EXPECT_TRUE(graph.empty());
}

TEST_F(UnifiedPipelinePlanUTest, ExportBuildExtendsPreparedRootsWithoutSelectingReportsAgain)
{
    const auto request = StepTraceRequest();
    ExportManager manager(root, request.reportsJson);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::SUMMARY}, context, roots));
    const auto selectedRoots = roots;
    // The export entry owns selection. Graph building must consume that selection, not read the report again.
    FileWriter(request.reportsJson).WriteText("{\"summary_process\":{\"unknown_deliverable\":true}}");
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(never());
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());

    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.BuildExport(context, roots, graph));
    EXPECT_EQ(roots, selectedRoots);
    EXPECT_TRUE(context.scopeContexts.empty());
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::HOST_PROCESSING, "HostC", "host"}), 1U);
    EXPECT_EQ(keys.count({TopoNodeStage::SUMMARY_GENERATION, "STEP_TRACE", "export"}), 1U);
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "HcclCalculator", scope}), 0U);
    }
    for (const auto& node : keys)
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::DATABASE_PERSISTENCE);
    }
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportBuildRejectsInvalidProfileAndClearsPreviousGraph)
{
    const auto request = StepTraceRequest();
    ExportManager manager(root, request.reportsJson);
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::SUMMARY}, context, roots));
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.BuildExport(context, roots, graph));

    context.profPath = File::PathJoin({root, "missing"});
    EXPECT_FALSE(builder.BuildExport(context, roots, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportBuildKeepsExistingDatabaseSkipWithoutAddingExportRoots)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());
    MOCKER(Domain::GetDeviceDirectories).expects(never());
    TopoBuildContext context;
    context.profPath = root;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.BuildExport(context, {}, graph));
    {
        DBRunner database(File::PathJoin({root, "msprof_123.db"}));
        ASSERT_TRUE(database.CreateTable("ExistingExport", {{"value", "INTEGER"}}));
    }
    const nlohmann::json completed = {{"version", 1}, {"sentinel", "preserved"}};
    ASSERT_TRUE(File::WriteTextAtomic(root + "/.unified.complete", completed.dump()));
    // An empty export must not inspect raw Device metadata or prepare a parse graph.
    FileWriter(root + "/device_0/info.json").WriteText("invalid Device metadata");
    ExportManager manager(root);
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(manager.Plan({ExportMode::DB}, context, roots));
    ASSERT_TRUE(roots.empty());
    ASSERT_TRUE(builder.BuildExport(context, roots, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
    ASSERT_TRUE(manager.Run({ExportMode::DB}));
    nlohmann::json after;
    ASSERT_EQ(FileReader(root + "/.unified.complete").ReadJson(after), ANALYSIS_OK);
    EXPECT_EQ(after, completed);
    EXPECT_FALSE(File::Exist(root + "/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/device_1/sqlite"));
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
}

TEST_F(UnifiedPipelinePlanUTest, EmptyExportIgnoresLegacyMarkersButRejectsUnhealthyExistingDatabase)
{
    TopoBuildContext context;
    context.profPath = root;
    const auto dbPath = root + "/msprof_123.db";
    {
        DBRunner database(dbPath);
        ASSERT_TRUE(database.CreateTable("ExistingExport", {{"value", "INTEGER"}}));
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.BuildExport(context, {}, graph));
    const auto markerPath = root + "/.unified.incomplete";
    FileWriter(markerPath).WriteText("unfinished");
    EXPECT_TRUE(builder.BuildExport(context, {}, graph));
    std::vector<std::string> marker;
    ASSERT_EQ(FileReader(markerPath).ReadText(marker), ANALYSIS_OK);
    EXPECT_EQ(marker, (std::vector<std::string>{"unfinished"}));
    for (const auto& content : {"not a SQLite database", ""})
    {
        ASSERT_TRUE(File::WriteTextAtomic(dbPath, content));
        EXPECT_FALSE(builder.BuildExport(context, {}, graph));
        EXPECT_TRUE(graph.empty());
        EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
    }
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, ExplicitHostParseStillRunsWhenExistingDatabaseSkipsDBExport)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(once()).will(returnValue(ANALYSIS_OK));
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(never());
    {
        DBRunner database(root + "/msprof_123.db");
        ASSERT_TRUE(database.CreateTable("ExistingExport", {{"value", "INTEGER"}}));
    }
    const auto request = Request(PIPELINE_CANN_TRACE | PIPELINE_DB);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(request, graph));
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::HOST_PROCESSING, "HostC", "host"}), 1U);
    for (const auto& node : builder.GetRuntimeNodeKeys())
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::DATABASE_PERSISTENCE);
        EXPECT_NE(node.first.stage, TopoNodeStage::DEVICE_PROCESSING);
    }
    ASSERT_TRUE(ExecuteTopoGraph(builder, graph, root));
    EXPECT_FALSE(File::Exist(root + "/host/sqlite/.unified.complete"));
}

TEST_F(UnifiedPipelinePlanUTest, ExportEntryBuildSupportsProfilesWithoutHost)
{
    const auto request = StepTraceRequest();
    ASSERT_TRUE(File::RemoveDir(root + "/host", 0));
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(root, {ExportMode::SUMMARY}, request.reportsJson, graph));
    for (const auto& node : builder.GetRuntimeNodeKeys())
    {
        EXPECT_NE(node.first.stage, TopoNodeStage::HOST_PROCESSING);
    }
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", "device_0"}),
              1U);
}

TEST_F(UnifiedPipelinePlanUTest, ExportEntryRejectsInvalidModesAndClearsPreviousGraph)
{
    const auto request = StepTraceRequest();
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(root, {ExportMode::SUMMARY}, request.reportsJson, graph));
    EXPECT_FALSE(builder.Build(root, {ExportMode::SUMMARY, static_cast<ExportMode>(99)}, request.reportsJson, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
    EXPECT_FALSE(builder.Build(root, {}, request.reportsJson, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, FailedRequestRebuildClearsPreviouslyBuiltGraphAndKeys)
{
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    const auto valid = StepTraceRequest();
    ASSERT_TRUE(builder.Build(valid, graph));
    auto invalid = valid;
    invalid.flags = 0;
    EXPECT_FALSE(builder.Build(invalid, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());

    invalid = valid;
    invalid.flags |= 0x20;
    EXPECT_FALSE(builder.Build(invalid, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());

    ASSERT_TRUE(builder.Build(valid, graph));
    FileWriter(valid.reportsJson).WriteText("{\"summary_process\":{\"unknown_deliverable\":true}}");
    EXPECT_FALSE(builder.Build(valid, graph));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
    EXPECT_FALSE(File::Exist(root + "/host/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, UnrelatedUnsupportedRawDoesNotExpandSelectedSummary)
{
    const auto request = StepTraceRequest();
    FileWriter(File::PathJoin({root, "device_0", "data", "stars_soc_profile.data.0.slice_0"})).WriteText("raw");
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    EXPECT_TRUE(PlanUnifiedPipeline(request, context, definitions, roots));
}

TEST_F(UnifiedPipelinePlanUTest, RequiredMissingProducerFailsBeforeAnyWrites)
{
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    TopoNodeRegistry::MutableDefinitions().erase({TopoNodeStage::DEVICE_PROCESSING, "NpuMemParser"});
    auto request = StepTraceRequest();
    FileWriter(request.reportsJson).WriteText("{\"summary_process\":{\"npu_memory\":true}}");
    FileWriter(File::PathJoin({root, "device_0", "data", "npu_mem.data.0.slice_0"})).WriteText("raw");
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(PlanUnifiedPipeline(request, context, definitions, roots));
    EXPECT_FALSE(File::Exist(File::PathJoin({root, "device_0", "sqlite"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({root, "host", "sqlite"})));
}

TEST_F(UnifiedPipelinePlanUTest, FullParseIncludesAllSupportedDeviceOperations)
{
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(3), context, definitions, roots, graph, builder));
    for (const auto& scope : {"device_0", "device_1"}) {
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "HcclCalculator", scope}), 1U);
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}), 1U);
        for (const auto& name : {"StarsSocProfileParser", "ChipTransPersistence", "InterSocPersistence", "SioPersistence"})
        {
            EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, name, scope}), 1U);
        }
        EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "LowPowerPersistence", scope}), 0U);
    }
    for (const auto& node : graph) { EXPECT_NE(node.second.scope, "export"); }
}

TEST_F(UnifiedPipelinePlanUTest, ExplicitFullDBKeepsAllSaverDependencies)
{
    auto request = StepTraceRequest();
    request.flags |= 4;
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(request, context, definitions, roots, graph, builder));
    ASSERT_NE(context.dbSession, nullptr);
    EXPECT_TRUE(File::GetFilesWithPrefix(root, "msprof_").empty());
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
    size_t savers = 0;
    for (const auto& node : builder.GetRuntimeNodeKeys()) {
        if (node.first.stage == TopoNodeStage::DATABASE_PERSISTENCE) { ++savers; }
    }
    EXPECT_GT(savers, 1U);
}

TEST_F(UnifiedPipelinePlanUTest, InvalidSelectionDoesNotFallBackToFullExport)
{
    auto request = StepTraceRequest();
    request.flags = 0x0b;
    for (const auto& config : {"{", "[]", "{\"json_process\":{\"unknown\":true}}", "{\"json_process\":false}"}) {
        FileWriter(request.reportsJson).WriteText(config);
        TopoBuildContext context;
        TopoNodeCollection definitions;
        std::vector<TopoNodeId> roots;
        EXPECT_FALSE(PlanUnifiedPipeline(request, context, definitions, roots));
    }
}

TEST_F(UnifiedPipelinePlanUTest, LegacyMarkersAloneDoNotMakeIntermediateDataReusable)
{
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        FileWriter(File::PathJoin({sqlite, ".unified.incomplete"})).WriteText("unfinished");
        FileWriter(File::PathJoin({sqlite, ".unified.complete"})).WriteText("invalid marker");
    }
    MOCKER(File::GetMetadata).expects(never());
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(PlanUnifiedPipeline(Request(3), context, definitions, roots));
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        EXPECT_FALSE(context.scopeReuse.at(scope));
    }
}

TEST_F(UnifiedPipelinePlanUTest, LegacyUnifiedMarkersDoNotBlockExecutionOrGetModified)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(once()).will(returnValue(ANALYSIS_OK));
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(exactly(2)).will(returnValue(true));
    MOCKER(File::GetMetadata).expects(never());
    std::vector<std::string> markerPaths;
    for (const auto& directory : {root, root + "/host/sqlite", root + "/device_0/sqlite", root + "/device_1/sqlite"})
    {
        ASSERT_TRUE(File::CreateDir(directory));
        for (const auto& name : {".unified.complete", ".unified.incomplete"})
        {
            const auto path = File::PathJoin({directory, name});
            FileWriter(path).WriteText("legacy marker");
            markerPaths.push_back(path);
        }
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(StepTraceRequest(), graph));
    for (auto& node : graph)
    {
        const auto name = node.second.processName;
        const bool deviceBusiness = node.second.scope.find("device_") == 0 &&
                                    name != "Device.Init" && name != "Device.Ready";
        const bool exportBusiness = name == "STEP_TRACE";
        if (deviceBusiness || exportBusiness)
        {
            node.second.creator = []() -> std::unique_ptr<Process>
            { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
        }
    }
    ASSERT_TRUE(ExecuteTopoGraph(builder, graph, root));
    EXPECT_TRUE(File::Exist(root + "/mindstudio_profiler_output"));
    for (const auto& path : markerPaths)
    {
        std::vector<std::string> content;
        ASSERT_EQ(FileReader(path).ReadText(content), ANALYSIS_OK);
        EXPECT_EQ(content, (std::vector<std::string>{"legacy marker"}));
    }
}

TEST_F(UnifiedPipelinePlanUTest, ExistingDatabasesAreReusedWithoutInputSnapshotChecks)
{
    const auto request = Request(PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA);
    for (const auto& scope : {"host", "device_0", "device_1"})
    {
        const auto sqlite = File::PathJoin({root, scope, "sqlite"});
        ASSERT_TRUE(File::CreateDir(sqlite));
        DBRunner db(File::PathJoin({sqlite, "existing.db"}));
        ASSERT_TRUE(db.CreateTable("ParsedData", {{"value", "INTEGER"}}));
        FileWriter(File::PathJoin({sqlite, ".unified.complete"})).WriteText("legacy input snapshot");
    }
    MOCKER(File::GetMetadata).expects(never());
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    for (const auto& content : {"before", "after!"})
    {
        FileWriter(root + "/host/input.bin").WriteText(content);
        FileWriter(root + "/device_0/data/raw.slice_0").WriteText(content);
        ASSERT_TRUE(PlanUnifiedPipeline(request, context, definitions, roots));
        for (const auto& scope : {"host", "device_0", "device_1"})
        {
            EXPECT_TRUE(context.scopeReuse.at(scope));
        }
    }
}

TEST_F(UnifiedPipelinePlanUTest, MixedDeviceDatabasesKeepPerScopeReuseWithoutMarkers)
{
    const auto sqlite = root + "/device_0/sqlite";
    ASSERT_TRUE(File::CreateDir(sqlite));
    DBRunner db(sqlite + "/existing.db");
    ASSERT_TRUE(db.CreateTable("ParsedData", {{"value", "INTEGER"}}));
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(PIPELINE_DEVICE_DATA), context, definitions, roots, graph, builder));
    EXPECT_TRUE(context.scopeReuse.at("device_0"));
    EXPECT_FALSE(context.scopeReuse.at("device_1"));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", "device_0"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", "device_1"}), 1U);
    ExpectNoUnifiedMarkers();
}

TEST_F(UnifiedPipelinePlanUTest, CorruptIntermediateDatabaseIsRejectedDespiteLegacyCompletionMarker)
{
    const auto sqlite = root + "/device_0/sqlite";
    ASSERT_TRUE(File::CreateDir(sqlite));
    FileWriter(sqlite + "/invalid.db").WriteText("not a SQLite database");
    const auto completed = sqlite + "/.unified.complete";
    FileWriter(completed).WriteText("legacy marker");
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(PlanUnifiedPipeline(Request(PIPELINE_DEVICE_DATA), context, definitions, roots));
    std::vector<std::string> content;
    ASSERT_EQ(FileReader(completed).ReadText(content), ANALYSIS_OK);
    EXPECT_EQ(content, (std::vector<std::string>{"legacy marker"}));
    EXPECT_FALSE(File::Exist(sqlite + "/.unified.incomplete"));
    EXPECT_FALSE(File::Exist(root + "/mindstudio_profiler_output"));
}

TEST_F(UnifiedPipelinePlanUTest, RepeatedSelectedExportsReuseDatabasesWithoutDroppingTables)
{
    const auto request = StepTraceRequest();
    struct RegistryGuard
    {
        TopoNodeCollection original = TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    for (auto& entry : TopoNodeRegistry::MutableDefinitions())
    {
        auto& definition = entry.second;
        if (definition.scopeKind != TopoScopeKind::DEVICE || definition.opensDeviceScope ||
            definition.closesDeviceScope)
        {
            continue;
        }
        const auto name = definition.id.name;
        definition.inputDataTypes = {typeid(std::string)};
        definition.creatorFactory = [name](const TopoBuildContext&) -> ProcessCreator
        {
            return [name]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess(
                    [name](DataInventory& data)
                    {
                        if (name != "TracePersistence")
                        {
                            return true;
                        }
                        const auto path = data.GetPtr<std::string>();
                        if (!path)
                        {
                            return false;
                        }
                        DBRunner db(File::PathJoin({*path, "sqlite", "trace.db"}));
                        return db.CreateTable("training_trace", {{"value", "INTEGER"}}) &&
                               db.InsertData("training_trace", std::vector<std::tuple<int>>{std::make_tuple(1)});
                    }));
            };
        };
    }
    MOCKER_CPP(&Domain::Environment::Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).stubs().will(returnValue(ANALYSIS_OK));
    MOCKER_CPP(&Domain::DeviceContext::Init).stubs().will(returnValue(true));
    for (size_t run = 0; run < 2; ++run) {
        TopoBuildContext context;
        TopoNodeCollection definitions;
        std::vector<TopoNodeId> roots;
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(Build(request, context, definitions, roots, graph, builder));
        for (const auto& scope : {"device_0", "device_1"}) {
            EXPECT_EQ(context.scopeReuse.at(scope), run != 0);
            EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope}),
                      run == 0 ? 1U : 0U);
            const auto path = context.scopePaths.at(scope);
            ASSERT_TRUE(context.scopeInventories.at(scope)->Inject(std::make_shared<std::string>(path)));
            if (run == 0) {
                ASSERT_TRUE(File::CreateDir(File::PathJoin({path, "sqlite"})));
                DBRunner db(File::PathJoin({path, "sqlite", "trace.db"}));
                ASSERT_TRUE(db.CreateTable("OtherProducer", {{"value", "INTEGER"}}));
                ASSERT_TRUE(db.InsertData("OtherProducer", std::vector<std::tuple<int>>{std::make_tuple(7)}));
            }
        }
        for (auto& node : graph) {
            if (node.second.scope == "export") {
                node.second.creator = []() -> std::unique_ptr<Process> {
                    return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; }));
                };
            }
        }
        ProcessControl executor(graph);
        ASSERT_TRUE(executor.ExecuteProcess());
        for (const auto& scope : {"device_0", "device_1"}) {
            DBRunner db(File::PathJoin({root, scope, "sqlite", "trace.db"}));
            std::vector<std::tuple<int>> rows;
            ASSERT_TRUE(db.QueryData("SELECT value FROM training_trace", rows));
            ASSERT_EQ(rows.size(), 1U);
            EXPECT_EQ(std::get<0>(rows.front()), 1);
            rows.clear();
            ASSERT_TRUE(db.QueryData("SELECT value FROM OtherProducer", rows));
            ASSERT_EQ(rows.size(), 1U);
            EXPECT_EQ(std::get<0>(rows.front()), 7);
        }
    }
}

TEST(UnifiedDeviceRegistrationUTest, CentralConfigurationUsesNativeTopoDefinitions)
{
    size_t businessNodes = 0;
    for (const auto& entry : TopoNodeRegistry::GetDefinitions())
    {
        const auto& definition = entry.second;
        if (definition.scopeKind != TopoScopeKind::DEVICE || definition.opensDeviceScope ||
            definition.closesDeviceScope)
        {
            continue;
        }
        ++businessNodes;
        EXPECT_TRUE(definition.id.scope.empty());
        EXPECT_TRUE(static_cast<bool>(definition.creatorFactory));
        const auto& dependencies = definition.processDependencies;
        EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), TOPO_NODE(DEVICE_PROCESSING, "Device.Init")),
                  dependencies.end());
    }
    EXPECT_EQ(businessNodes, 44U);
    const auto* parser = TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PROCESSING, "TsTrackParser"));
    ASSERT_NE(parser, nullptr);
    EXPECT_NE(std::find(parser->processDependencies.begin(), parser->processDependencies.end(),
                        TOPO_NODE(DEVICE_PROCESSING, "LoadStreamExpandSpec")),
              parser->processDependencies.end());
    const auto* trace = TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PROCESSING, "TracePersistence"));
    ASSERT_NE(trace, nullptr);
    EXPECT_NE(std::find(trace->processDependencies.begin(), trace->processDependencies.end(),
                        TOPO_NODE(DEVICE_PROCESSING, "StepTraceProcess")), trace->processDependencies.end());
    const auto* ccu = TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PROCESSING, "CcuPersistence"));
    ASSERT_NE(ccu, nullptr);
    EXPECT_EQ(ccu->chipIds, (std::vector<uint32_t>{CHIP_V6_1_0, CHIP_V6_1_1}));
}

TEST_F(UnifiedPipelinePlanUTest, MissingOptionalQosDoesNotFailFullParseOrHoldDeviceSlots)
{
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::DeviceContext::Init).expects(exactly(2)).will(returnValue(true));
    MOCKER_CPP(&Domain::KernelParserWorker::Run).expects(never());
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(PIPELINE_DEVICE_DATA), graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto final = keys.at({TopoNodeStage::FLOW_CONTROL, "Pipeline.Done", "profile"});
    EXPECT_TRUE(graph.at(final).mandatory);
    EXPECT_TRUE(graph.at(final).runOnDependencyFailure);
    for (const auto& scope : {"device_0", "device_1"})
    {
        const auto qos = keys.at({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", scope});
        const auto required = keys.at({TopoNodeStage::DEVICE_PROCESSING, "TracePersistence", scope});
        const auto ready = keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope});
        EXPECT_FALSE(graph.at(qos).mandatory);
        EXPECT_TRUE(graph.at(required).mandatory);
        const auto& successDependencies = graph.at(ready).processDependence;
        const auto& completionDependencies = graph.at(ready).completionDependence;
        EXPECT_EQ(std::find(successDependencies.begin(), successDependencies.end(), qos), successDependencies.end());
        EXPECT_NE(std::find(completionDependencies.begin(), completionDependencies.end(), qos), completionDependencies.end());
        EXPECT_NE(std::find(successDependencies.begin(), successDependencies.end(), required), successDependencies.end());
    }
    const auto firstQos = keys.at({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", "device_0"});
    const auto nextInit = keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Init", "device_1"});
    graph.at(nextInit).completionDependence = {firstQos};
    for (auto& node : graph)
    {
        const auto& name = node.second.processName;
        if (node.second.scope.find("device_") != 0 || name == "Device.Init" || name == "Device.Ready" ||
            name == "QosPersistence")
        {
            continue;
        }
        // Keep real QoS persistence: no reader injects QoS data, so its existing missing-input path fails.
        node.second.creator = []() -> std::unique_ptr<Process>
        { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
    }
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    options.maxActiveDevices = 1;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", scope})).state,
                  RuntimeProcessState::FAILED);
        EXPECT_EQ(results.at(keys.at({TopoNodeStage::DEVICE_PROCESSING, "Device.Ready", scope})).state,
                  RuntimeProcessState::SUCCEEDED);
        EXPECT_FALSE(File::Exist(root + "/" + scope + "/sqlite/.unified.complete"));
        EXPECT_FALSE(File::Exist(root + "/" + scope + "/sqlite/.unified.incomplete"));
    }
    EXPECT_EQ(results.at(final).state, RuntimeProcessState::SUCCEEDED);
}

TEST_F(UnifiedPipelinePlanUTest, PipelineDoneStillHonorsGlobalDfxStop)
{
    class StopContext final : public Context
    {
       public:
        const std::string& GetDfxStopAtName() const override { return stopAt_; }
       private:
        std::string stopAt_ = "HostC";
    };
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(Request(PIPELINE_CANN_TRACE), graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto host = keys.at({TopoNodeStage::HOST_PROCESSING, "HostC", "host"});
    const auto final = keys.at({TopoNodeStage::FLOW_CONTROL, "Pipeline.Done", "profile"});
    graph.at(host).context = std::make_shared<StopContext>();
    EXPECT_TRUE(graph.at(final).runOnDependencyFailure);
    for (auto& node : graph)
    {
        node.second.creator = []() -> std::unique_ptr<Process>
        { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
    }
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    EXPECT_TRUE(results.at(host).statistics.dfxStop);
    EXPECT_EQ(results.at(final).state, RuntimeProcessState::CANCELLED);
}

TEST_F(UnifiedPipelinePlanUTest, MigratedRegistrationKeepsUpstreamV6ReadersAndStreamIdInputs)
{
    FileWriter(root + "/device_1/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(3), context, definitions, roots, graph, builder));
    const auto& keys = builder.GetRuntimeNodeKeys();
    for (const auto& scope : {"device_0", "device_1"})
    {
        for (const auto& name : {"NpuMemParser", "NpuModuleMemParser", "NpuMemPersistence", "NpuModuleMemPersistence"})
        {
            EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, name, scope}), 1U);
        }
        const auto association = keys.at({TopoNodeStage::DEVICE_PROCESSING, "AscendTaskAssociation", scope});
        const auto aicpu = keys.at({TopoNodeStage::DEVICE_PROCESSING, "AicpuParser", scope});
        const auto& dependencies = graph.at(association).processDependence;
        EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), aicpu), dependencies.end());
        const auto& inputs = graph.at(association).paramTypes;
        EXPECT_NE(std::find(inputs.begin(), inputs.end(), typeid(Domain::DeviceStreamInfo)), inputs.end());
    }
    const auto acc = keys.at({TopoNodeStage::DEVICE_PROCESSING, "AccPmuPersistence", "device_1"});
    const auto log = keys.at({TopoNodeStage::DEVICE_PROCESSING, "StarsSocParserV6", "device_1"});
    const auto profile = keys.at({TopoNodeStage::DEVICE_PROCESSING, "StarsSocProfileParser", "device_1"});
    const auto& dependencies = graph.at(acc).processDependence;
    EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), log), dependencies.end());
    EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), profile), dependencies.end());
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "InterSocPersistence", "device_1"}), 1U);
    for (const auto& name : {"FreqParser", "FreqPersistence"})
    {
        const auto* definition = TopoNodeRegistry::Find({TopoNodeStage::DEVICE_PROCESSING, name});
        ASSERT_NE(definition, nullptr);
        EXPECT_TRUE(TopoNodeRegistry::SupportsChip(*definition, CHIP_V6_1_0));
        EXPECT_TRUE(TopoNodeRegistry::SupportsChip(*definition, CHIP_V6_1_1));
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, name, "device_1"}), 1U);
    }
}

TEST_F(UnifiedPipelinePlanUTest, FullParseSelectsChipSpecificDefinitions)
{
    FileWriter(root + "/device_0/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_0}}.dump());
    FileWriter(root + "/device_1/info.json").WriteText(nlohmann::json{{"platform_version", CHIP_V6_1_1}}.dump());
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(Request(3), context, definitions, roots, graph, builder));
    const auto& keys = builder.GetRuntimeNodeKeys();
    for (const auto& scope : {"device_0", "device_1"})
    {
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LogModeling", scope}), 0U);
        ASSERT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LogModelingV6", scope}), 1U);
        EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "CcuPersistence", scope}), 1U);
        const auto load = keys.at({TopoNodeStage::DEVICE_PROCESSING, "LoadHostData", scope});
        const auto model = keys.at({TopoNodeStage::DEVICE_PROCESSING, "LogModelingV6", scope});
        const auto& dependencies = graph.at(load).processDependence;
        EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), model), dependencies.end());
    }
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "StarsSocProfileParser", "device_0"}), 1U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LowPowerPersistence", "device_0"}), 1U);
    ASSERT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", "device_0"}), 1U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "QosParser", "device_0"}), 0U);
    const auto qos = keys.at({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", "device_0"});
    const auto profile = keys.at({TopoNodeStage::DEVICE_PROCESSING, "StarsSocProfileParser", "device_0"});
    const auto& qosDependencies = graph.at(qos).processDependence;
    EXPECT_NE(std::find(qosDependencies.begin(), qosDependencies.end(), profile), qosDependencies.end());
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "StarsSocProfileParser", "device_1"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LowPowerPersistence", "device_1"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "QosPersistence", "device_1"}), 0U);
}

TEST_F(UnifiedPipelinePlanUTest, SelectedTaskAndAicpuIncludeTaskInitializationWithoutLogModeling)
{
    const std::map<std::string, uint32_t> chips{{"device_0", CHIP_V6_2_0}, {"device_1", CHIP_V1_1_1}};
    for (const auto& chip : chips)
    {
        FileWriter(root + "/" + chip.first + "/info.json")
            .WriteText(nlohmann::json{{"platform_version", chip.second}}.dump());
    }
    auto request = Request(0x12);
    request.reportsJson = root + "/reports.json";
    for (const auto& report : {"task_time", "aicpu"})
    {
        SCOPED_TRACE(report);
        FileWriter(request.reportsJson).WriteText(nlohmann::json{{"summary_process", {{report, true}}}}.dump());
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(request, graph));
        const auto& keys = builder.GetRuntimeNodeKeys();
        for (const auto& chip : chips)
        {
            SCOPED_TRACE(chip.first);
            const TopoNodeId init{TopoNodeStage::DEVICE_PROCESSING, "DeviceTaskProcess", chip.first};
            const TopoNodeId load{TopoNodeStage::DEVICE_PROCESSING, "LoadHostData", chip.first};
            ASSERT_EQ(keys.count(init), 1U);
            ASSERT_EQ(keys.count(load), 1U);
            const auto& dependencies = graph.at(keys.at(load)).processDependence;
            EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), keys.at(init)), dependencies.end());
            EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LogModeling", chip.first}), 0U);
            EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "LogModelingV6", chip.first}), 0U);
        }
    }
}

TEST_F(UnifiedPipelinePlanUTest, HostDataLoadsWithRealTaskInitializationWithoutLogModeling)
{
    using DeviceTasks = std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>;
    const std::map<std::string, uint32_t> chips{{"device_0", CHIP_V6_2_0}, {"device_1", CHIP_V1_1_1}};
    TopoBuildContext context;
    std::vector<TopoNodeId> roots;
    for (const auto& chip : chips)
    {
        auto device = Domain::DeviceContext::Create(chip.second);
        device->deviceContextInfo.deviceFilePath = root + "/" + chip.first;
        context.scopeContexts[chip.first] = device;
        context.scopeInventories[chip.first] = std::make_shared<DataInventory>();
        context.scopeChips[chip.first] = chip.second;
        roots.push_back({TopoNodeStage::DEVICE_PROCESSING, "LoadHostData", chip.first});
    }
    // Isolate the real producer/consumer chain from metadata loading and raw parsing.
    TopoNodeDefinition init;
    init.id = TOPO_NODE(DEVICE_PROCESSING, "Device.Init");
    init.scopeKind = TopoScopeKind::DEVICE;
    init.mandatory = true;
    init.creatorFactory = [](const TopoBuildContext&) -> ProcessCreator
    {
        return []() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; }));
        };
    };
    TopoNodeCollection definitions{{init.id, init}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph, &definitions));
    for (const auto& chip : chips)
    {
        EXPECT_EQ(context.scopeInventories.at(chip.first)->GetPtr<DeviceTasks>(), nullptr);
    }
    ProcessControlOptions options;
    options.releaseUnusedData = false;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    for (const auto& chip : chips)
    {
        SCOPED_TRACE(chip.first);
        const auto key = builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "LoadHostData", chip.first});
        EXPECT_EQ(results.at(key).state, RuntimeProcessState::SUCCEEDED);
        const auto tasks = context.scopeInventories.at(chip.first)->GetPtr<DeviceTasks>();
        ASSERT_NE(tasks, nullptr);
        EXPECT_TRUE(tasks->empty());
        EXPECT_NE(context.scopeInventories.at(chip.first)->GetPtr<Domain::TaskId2HostTask>(), nullptr);
    }
    EXPECT_NE(context.scopeInventories.at("device_0")->GetPtr<DeviceTasks>(),
              context.scopeInventories.at("device_1")->GetPtr<DeviceTasks>());
}

TEST_F(UnifiedPipelinePlanUTest, SelectedV6TsTrackProducerUsesNativeTopology)
{
    for (const auto chip : {CHIP_V6_1_0, CHIP_V6_1_1, CHIP_V6_2_0})
    {
        const auto request = StepTraceRequest();
        FileWriter(root + "/device_0/info.json").WriteText(nlohmann::json{{"platform_version", chip}}.dump());
        FileWriter(root + "/device_0/data/ts_track.0.slice_0").WriteText("raw");
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(request, graph));
        for (const auto& name : {"TsTrackParser", "StepTraceProcess", "TracePersistence"})
        {
            EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PROCESSING, name, "device_0"}), 1U);
        }
        for (const auto& name : {"StepTraceProcess", "StepTracePersistence", "TracePersistence", "TsTrackPersistence"})
        {
            const auto* definition = TopoNodeRegistry::Find({TopoNodeStage::DEVICE_PROCESSING, name});
            ASSERT_NE(definition, nullptr);
            EXPECT_TRUE(TopoNodeRegistry::SupportsChip(*definition, chip));
        }
        EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
    }
}

TEST_F(UnifiedPipelinePlanUTest, SelectedExternalInputDoesNotBlockExportPlanning)
{
    auto request = StepTraceRequest();
    FileWriter(request.reportsJson).WriteText("{\"summary_process\":{\"page_fault\":true}}");
    FileWriter(root + "/device_0/data/socpmu.0.slice_0").WriteText("raw");
    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    EXPECT_TRUE(PlanUnifiedPipeline(request, context, definitions, roots));
    EXPECT_FALSE(File::Exist(root + "/device_0/sqlite"));
}

TEST_F(UnifiedPipelinePlanUTest, FusionTaskWithRawDataAndMissingIntermediateUsesExistingNoDataPath)
{
    auto request = Request(PIPELINE_DEVICE_DATA | PIPELINE_TIMELINE);
    request.reportsJson = root + "/reports.json";
    FileWriter(request.reportsJson).WriteText("{\"json_process\":{\"fusion_task\":true}}");
    FileWriter(root + "/device_0/data/stars_soc.0.slice_0").WriteText("raw");
    MOCKER_CPP(&Domain::Environment::Context::Load).expects(once()).will(returnValue(true));
    MOCKER_CPP(&Domain::Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));

    TopoBuildContext context;
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(Build(request, context, definitions, roots, graph, builder));
    EXPECT_FALSE(context.scopeReuse.at("device_0"));
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count(
                  {TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_FUSION_TASK, "export"}), 1U);
    // Other parsers may create the sqlite directory without producing optional FusionTask data.
    for (const auto& scope : {"device_0", "device_1"})
    {
        ASSERT_TRUE(File::CreateDir(File::PathJoin({root, scope, "sqlite"})));
        EXPECT_FALSE(File::Exist(File::PathJoin({root, scope, "sqlite", "fusion_task.db"})));
    }
    // Keep the real Processor and Assembler: missing optional data must not fail the export.
    EXPECT_TRUE(ExecuteTopoGraph(builder, graph, root));
    EXPECT_FALSE(File::Exist(root + "/.unified.complete"));
    EXPECT_TRUE(File::GetFilesWithPrefix(root + "/mindstudio_profiler_output", "msprof_").empty());
}

TEST_F(UnifiedPipelinePlanUTest, StringIdsPersistAfterSaverFailureButDoNotRunAfterDfxStop)
{
    using IdPool = Analysis::Application::Credential::IdPool;
    struct IdPoolGuard
    {
        ~IdPoolGuard() { IdPool::GetInstance().Clear(); }
    } poolGuard;
    class StopContext : public Infra::Context
    {
    public:
        const std::string& GetDfxStopAtName() const override { return name_; }
    private:
        const std::string name_ = "DB:TestFailure";
    };
    for (const bool stop : {false, true})
    {
        SCOPED_TRACE(stop ? "DFX stop" : "saver failure");
        IdPool::GetInstance().Clear();
        const auto directory = root + (stop ? "/dfx" : "/failure");
        ASSERT_TRUE(File::CreateDir(directory));
        TopoBuildContext context;
        context.profPath = directory;
        context.scopeContexts["export"] = std::make_shared<Infra::Context>();
        context.scopeInventories["export"] = std::make_shared<DataInventory>();
        context.dbSession = std::make_shared<DBAssembler>(directory, directory);
        const TopoNodeId good{TopoNodeStage::DATABASE_PERSISTENCE, "DB:TestSuccess"};
        const TopoNodeId failed{TopoNodeStage::DATABASE_PERSISTENCE, "DB:TestFailure"};
        const TopoNodeId blocked{TopoNodeStage::DATABASE_PERSISTENCE, "DB:TestBlocked"};
        const TopoNodeId stringIds{TopoNodeStage::FLOW_CONTROL, "DB:STRING_IDS"};
        std::atomic<uint64_t> savedId{0};
        TopoNodeCollection definitions;
        for (const auto& id : {good, failed, blocked})
        {
            TopoNodeDefinition definition;
            definition.id = id;
            definition.mandatory = true;
            definition.creatorFactory = [&, id, stop](const TopoBuildContext&)
            {
                return [&, id, stop]() -> std::unique_ptr<Process>
                {
                    return std::unique_ptr<Process>(new TopoCallbackProcess([&, id, stop](DataInventory&)
                    {
                        if (id == good)
                        {
                            savedId = IdPool::GetInstance().GetUint64Id("saved-before-failure");
                            return true;
                        }
                        if (id == failed) { return stop; }
                        ADD_FAILURE() << "A normal saver must stay blocked";
                        return true;
                    }));
                };
            };
            if (id == failed) { definition.processDependencies = {good}; }
            if (id == blocked) { definition.processDependencies = {failed}; }
            definitions.emplace(id, std::move(definition));
        }
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(context, {good, failed, blocked, stringIds}, graph, &definitions));
        const auto& keys = builder.GetRuntimeNodeKeys();
        const auto stringKey = keys.at({stringIds.stage, stringIds.name, "export"});
        const auto failedKey = keys.at({failed.stage, failed.name, "export"});
        EXPECT_TRUE(graph.at(stringKey).runOnDependencyFailure);
        EXPECT_EQ(graph.at(stringKey).processDependence.size(), 3U);
        if (stop) { graph.at(failedKey).context = std::make_shared<StopContext>(); }
        ProcessControlOptions options;
        options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
        ProcessControl executor(graph, options);
        EXPECT_FALSE(executor.ExecuteProcess());
        const auto results = executor.GetRuntimeResults();
        EXPECT_EQ(results.at(failedKey).state, RuntimeProcessState::FAILED);
        EXPECT_EQ(results.at(keys.at({blocked.stage, blocked.name, "export"})).state, RuntimeProcessState::BLOCKED);
        const auto files = File::GetFilesWithPrefix(directory, "msprof_");
        if (stop)
        {
            EXPECT_EQ(results.at(stringKey).state, RuntimeProcessState::CANCELLED);
            EXPECT_TRUE(files.empty());
            continue;
        }
        EXPECT_EQ(results.at(stringKey).state, RuntimeProcessState::SUCCEEDED);
        ASSERT_EQ(files.size(), 1U);
        DBRunner database(files.front());
        std::vector<std::tuple<uint64_t, std::string>> rows;
        ASSERT_TRUE(database.QueryData("SELECT * FROM STRING_IDS", rows));
        ASSERT_EQ(rows.size(), 1U);
        EXPECT_EQ(std::get<0>(rows.front()), savedId.load());
        EXPECT_EQ(std::get<1>(rows.front()), "saved-before-failure");
    }
}

TEST_F(UnifiedPipelinePlanUTest, ExecutionFinalizesConsumerOnSuccessAndFailure)
{
    const auto output = File::PathJoin({root, Analysis::Common::OUTPUT_PATH});
    ASSERT_TRUE(File::CreateDir(output));
    const auto dbPath = File::PathJoin({root, "consumer.db"});
    for (const bool processSuccess : {true, false})
    {
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(Request(1), graph));
        for (auto& entry : graph)
        {
            const bool host = entry.second.processName == "HostC";
            entry.second.creator = [this, dbPath, host, processSuccess]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess([this, dbPath, host, processSuccess](DataInventory&)
                {
                    if (!host) { return true; }
                    auto& consumer = DeliverableConsumer::Instance();
                    consumer.AcceptJson({MSPROF_JSON_FILE}, root,
                                        {std::make_shared<Domain::DurationEvent>(1, 1, 1.5, "1.0", "op")});
                    EXPECT_TRUE(consumer.RunDb(dbPath, [](DBRunner&) { return true; }));
                    return processSuccess;
                }));
            };
        }
        EXPECT_EQ(ExecuteTopoGraph(builder, graph, root), processSuccess);
        EXPECT_FALSE(DeliverableConsumer::Instance().HasDbWriterForTest(dbPath));
        const auto files = File::GetOriginData(output, {MSPROF_JSON_FILE}, {});
        ASSERT_EQ(files.size(), 1U);
        nlohmann::json content;
        ASSERT_EQ(FileReader(files.front()).ReadJson(content), ANALYSIS_OK);
        ASSERT_TRUE(content.is_array());
        EXPECT_EQ(content.size(), 1U);
    }
}

TEST_F(UnifiedPipelinePlanUTest, ExecutionReportsConsumerFailureAndResetsForNextRun)
{
    const auto blocker = File::PathJoin({root, "write_blocker"});
    { FileWriter file(blocker); file.WriteText("file blocks directory creation"); }
    for (const bool failWrite : {true, false})
    {
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(Request(1), graph));
        for (auto& entry : graph)
        {
            const bool host = entry.second.processName == "HostC";
            entry.second.creator = [blocker, host, failWrite]() -> std::unique_ptr<Process>
            {
                return std::unique_ptr<Process>(new TopoCallbackProcess([blocker, host, failWrite](DataInventory&)
                {
                    if (host && failWrite)
                    {
                        DeliverableConsumer::Instance().WriteCsv(blocker + "/report", {"value"}, {{"1"}}, {}, "fixed");
                    }
                    return true;
                }));
            };
        }
        EXPECT_EQ(ExecuteTopoGraph(builder, graph, root), !failWrite);
    }
}
