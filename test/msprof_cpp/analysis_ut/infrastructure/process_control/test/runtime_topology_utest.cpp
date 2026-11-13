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
 */#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Application;

namespace {
class TestContext : public Context {
public:
    explicit TestContext(uint32_t chip) : chip_(chip) {}
    uint32_t GetChipID() const override { return chip_; }
private:
    uint32_t chip_;
};

class TestProcess : public Process {
public:
    explicit TestProcess(std::function<uint32_t(DataInventory&, const Context&)> action) : action_(std::move(action)) {}
private:
    uint32_t ProcessEntry(DataInventory& inventory, const Context& context) override { return action_(inventory, context); }
    std::function<uint32_t(DataInventory&, const Context&)> action_;
};

ProcessCreator Action(std::function<uint32_t(DataInventory&, const Context&)> action)
{
    return [action]() -> std::unique_ptr<Process> { return std::unique_ptr<Process>(new TestProcess(action)); };
}
TopoNodeCreatorFactory Factory(ProcessCreator creator)
{
    return [creator](const TopoBuildContext&) { return creator; };
}
void BindTest(TopoBuildContext& context, const std::string& scope, uint32_t chip)
{
    context.scopeContexts[scope] = std::make_shared<TestContext>(chip);
    context.scopeInventories[scope] = std::make_shared<DataInventory>();
    context.scopeChips[scope] = chip;
}
RuntimeProcessInfo Node(ProcessCreator action, const std::string& scope = "device_0")
{
    RuntimeProcessInfo info;
    info.creator = std::move(action);
    info.scope = scope;
    info.context = std::make_shared<TestContext>(5);
    info.inventory = std::make_shared<DataInventory>();
    info.processName = "test";
    info.mandatory = true;
    return info;
}
uint32_t Success(DataInventory&, const Context&) { return ANALYSIS_OK; }

TopoNodeDefinition RegisteredDeviceDefinition()
{
    TopoNodeDefinition definition;
    definition.id = TOPO_NODE(DEVICE_PROCESSING, "RuntimeTest.RegisteredDevice");
    definition.scopeKind = TopoScopeKind::DEVICE;
    definition.mandatory = true;
    definition.chipIds = {5, 15};
    definition.resources = {"runtime-test-resource"};
    definition.opensDeviceScope = true;
    definition.creatorFactory = [](const TopoBuildContext& context) {
        const auto path = context.scopePaths.at(context.currentScope);
        return Action([path](DataInventory&, const Context& device) {
            return path == std::to_string(device.GetChipID()) ? ANALYSIS_OK : ANALYSIS_ERROR;
        });
    };
    return definition;
}
REGISTER_TOPO_NODE_DEFINITION(RegisteredDeviceDefinition());

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroFailure"), true,
                            Factory(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; })),
                            TOPO_DEPS(), nullptr);
REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroDefault"), true,
                            Factory(Action(Success)), TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroFailure")),
                            nullptr);
REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroFalse"), true,
                            Factory(Action(Success)), TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroFailure")),
                            nullptr, false);
REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroTrue"), false,
                            Factory(Action(Success)), TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, "RuntimeTest.MacroFailure")),
                            nullptr, true);
}

TEST(RuntimeTopology, SequenceMacroDefaultsToFalseAndAcceptsExplicitDependencyFailurePolicy)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    const TopoNodeId defaultNode{TopoNodeStage::FLOW_CONTROL, "RuntimeTest.MacroDefault"};
    const TopoNodeId explicitFalse{TopoNodeStage::FLOW_CONTROL, "RuntimeTest.MacroFalse"};
    const TopoNodeId explicitTrue{TopoNodeStage::FLOW_CONTROL, "RuntimeTest.MacroTrue"};
    for (const auto& id : {defaultNode, explicitFalse, explicitTrue})
    {
        const auto* definition = TopoNodeRegistry::Find(id);
        ASSERT_NE(definition, nullptr);
        EXPECT_EQ(definition->runOnDependencyFailure, id == explicitTrue);
        EXPECT_EQ(definition->mandatory, !(id == explicitTrue));
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {defaultNode, explicitFalse, explicitTrue}, graph));
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(results.at(keys.at({defaultNode.stage, defaultNode.name, "export"})).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(keys.at({explicitFalse.stage, explicitFalse.name, "export"})).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(keys.at({explicitTrue.stage, explicitTrue.name, "export"})).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, StaticDefinitionInstantiatesDevicesWithFreshRequestBindings)
{
    const auto id = TOPO_NODE(DEVICE_PROCESSING, "RuntimeTest.RegisteredDevice");
    const auto* definition = TopoNodeRegistry::Find(id);
    ASSERT_NE(definition, nullptr);
    EXPECT_TRUE(definition->id.scope.empty());
    const auto definitionCount = TopoNodeRegistry::GetDefinitions().size();
    for (const auto& chips : {std::make_pair(5U, 15U), std::make_pair(15U, 5U)}) {
        TopoBuildContext context;
        BindTest(context, "device_0", chips.first);
        BindTest(context, "device_1", chips.second);
        context.scopePaths["device_0"] = std::to_string(chips.first);
        context.scopePaths["device_1"] = std::to_string(chips.second);
        RuntimeProcessCollection graph;
        TopoGraphBuilder builder;
        ASSERT_TRUE(builder.Build(context, {{id.stage, id.name, "device_0"}, {id.stage, id.name, "device_1"}}, graph));
        ASSERT_EQ(graph.size(), 2U);
        for (const auto& node : graph) {
            EXPECT_TRUE(node.second.opensDeviceScope);
            EXPECT_EQ(node.second.resources, std::vector<std::string>{"runtime-test-resource"});
        }
        ProcessControl executor(graph);
        EXPECT_TRUE(executor.ExecuteProcess());
    }
    EXPECT_EQ(TopoNodeRegistry::GetDefinitions().size(), definitionCount);
    EXPECT_EQ(TopoNodeRegistry::Find(id), definition);
}

TEST(RuntimeTopology, ReusesDefinitionWithIndependentDeviceBindings)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    TopoNodeCollection definitions;
    std::atomic<int> reads{0};
    TopoNodeDefinition producer;
    producer.id = {TopoNodeStage::DEVICE_PROCESSING, "producer"};
    producer.scopeKind = TopoScopeKind::DEVICE;
    producer.mandatory = true;
    producer.creatorFactory = Factory(Action([](DataInventory& inventory, const Context& ctx) {
        return inventory.Inject(std::make_shared<int>(ctx.GetChipID())) ? ANALYSIS_OK : ANALYSIS_ERROR;
    }));
    TopoNodeDefinition consumer;
    consumer.id = {TopoNodeStage::DEVICE_PROCESSING, "consumer"};
    consumer.scopeKind = TopoScopeKind::DEVICE;
    consumer.mandatory = true;
    consumer.inputDataTypes = {typeid(int)};
    consumer.processDependencies = {producer.id};
    consumer.creatorFactory = Factory(Action([&](DataInventory& inventory, const Context& ctx) {
        auto value = inventory.GetPtr<int>();
        if (!value || *value != static_cast<int>(ctx.GetChipID())) { return ANALYSIS_ERROR; }
        ++reads;
        return ANALYSIS_OK;
    }));
    definitions.emplace(producer.id, producer);
    definitions.emplace(consumer.id, consumer);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{consumer.id.stage, consumer.id.name, "device_0"},
                                       {consumer.id.stage, consumer.id.name, "device_1"}}, graph, &definitions));
    ASSERT_EQ(graph.size(), 4U);
    EXPECT_NE(builder.GetRuntimeNodeKeys().at({producer.id.stage, producer.id.name, "device_0"}),
              builder.GetRuntimeNodeKeys().at({producer.id.stage, producer.id.name, "device_1"}));
    ProcessControl executor(graph);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(reads, 2);
    EXPECT_EQ(context.scopeInventories.at("device_0")->Size(), 0U);
    EXPECT_EQ(context.scopeInventories.at("device_1")->Size(), 0U);
    std::string log;
    RecordProcessStat(executor.GetExecuteStat(), "PROF_test", log);
    EXPECT_NE(log.find("device_0/consumer"), std::string::npos);
    EXPECT_NE(log.find("device_1/consumer"), std::string::npos);
    EXPECT_NE(log.find("chip id: 15"), std::string::npos);
}

TEST(RuntimeTopology, FastDeviceAdvancesWhileOtherDeviceStillRunning)
{
    std::mutex mutex;
    std::condition_variable changed;
    bool childRan = false;
    RuntimeProcessCollection graph;
    graph.emplace(1, Node(Action([&](DataInventory&, const Context&) {
        std::unique_lock<std::mutex> lock(mutex);
        return changed.wait_for(lock, std::chrono::seconds(3), [&] { return childRan; }) ? ANALYSIS_OK : ANALYSIS_ERROR;
    }), "device_1"));
    graph.emplace(2, Node(Action(Success)));
    auto child = Node(Action([&](DataInventory&, const Context&) {
        std::lock_guard<std::mutex> lock(mutex);
        childRan = true;
        changed.notify_all();
        return ANALYSIS_OK;
    }));
    child.processDependence = {2};
    graph.emplace(3, std::move(child));
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_TRUE(childRan);
}

TEST(RuntimeTopology, SharedExportDependencyRunsOnce)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    TopoNodeCollection definitions;
    int calls = 0;
    TopoNodeDefinition shared;
    shared.id = {TopoNodeStage::DATA_PROCESSING, "shared"};
    shared.creatorFactory = Factory(Action([&](DataInventory&, const Context&) { ++calls; return ANALYSIS_OK; }));
    definitions.emplace(shared.id, shared);
    std::vector<TopoNodeId> roots;
    for (const auto& name : {"db", "timeline", "summary"}) {
        TopoNodeDefinition output;
        output.id = {TopoNodeStage::FLOW_CONTROL, name};
        output.processDependencies = {shared.id};
        output.creatorFactory = Factory(Action(Success));
        definitions.emplace(output.id, output);
        roots.push_back(output.id);
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph, &definitions));
    EXPECT_EQ(graph.size(), 4U);
    ProcessControl executor(graph);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(calls, 1);
}

TEST(RuntimeTopology, SupportsMoreThanSyntheticSlotLimit)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    TopoNodeCollection definitions;
    std::vector<TopoNodeId> roots;
    for (int index = 0; index < 100; ++index) {
        TopoNodeDefinition definition;
        definition.id = {TopoNodeStage::FLOW_CONTROL, std::to_string(index)};
        definition.creatorFactory = Factory(Action(Success));
        definitions.emplace(definition.id, definition);
        roots.push_back(definition.id);
    }
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph, &definitions));
    EXPECT_EQ(graph.size(), 100U);
    ProcessControl executor(graph);
    EXPECT_TRUE(executor.ExecuteProcess());
}

TEST(RuntimeTopology, RejectsMissingDependencyAndClearsPartialGraph)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    TopoNodeCollection definitions;
    TopoNodeDefinition definition;
    definition.id = {TopoNodeStage::FLOW_CONTROL, "consumer"};
    definition.processDependencies = {{TopoNodeStage::FLOW_CONTROL, "missing"}};
    definition.creatorFactory = Factory(Action(Success));
    definitions.emplace(definition.id, definition);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {definition.id}, graph, &definitions));
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(builder.GetRuntimeNodeKeys().empty());
}

TEST(RuntimeTopology, ScopedOverridesApplyToUnscopedDependencies)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    TopoNodeCollection definitions;
    std::atomic<bool> initialized{false};
    TopoNodeDefinition init;
    init.id = {TopoNodeStage::FLOW_CONTROL, "runtime-init", "export"};
    init.mandatory = true;
    init.creatorFactory = Factory(Action([&](DataInventory&, const Context&) { initialized = true; return ANALYSIS_OK; }));
    definitions.emplace(init.id, init);
    TopoNodeDefinition consumer;
    consumer.id = {TopoNodeStage::DATA_PROCESSING, "scoped-processor", "export"};
    consumer.mandatory = true;
    consumer.processDependencies = {init.id};
    consumer.creatorFactory = Factory(Action([&](DataInventory&, const Context&) { return initialized ? ANALYSIS_OK : ANALYSIS_ERROR; }));
    // A static definition supplies the scope classification; the request adds the lifecycle dependency.
    auto base = consumer;
    base.id.scope.clear();
    base.processDependencies.clear();
    definitions.emplace(base.id, base);
    definitions.emplace(consumer.id, consumer);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {base.id}, graph, &definitions));
    EXPECT_EQ(graph.size(), 2U);
    ProcessControl executor(graph);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_TRUE(initialized);
}

TEST(RuntimeTopology, RequiredFailureDrainsAlreadyRunningWork)
{
    std::mutex mutex;
    std::condition_variable changed;
    bool peerStarted = false;
    bool failureReturned = false;
    std::atomic<bool> peerFinished{false};
    RuntimeProcessCollection graph;
    graph.emplace(1, Node(Action([&](DataInventory&, const Context&) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!changed.wait_for(lock, std::chrono::seconds(3), [&] { return peerStarted; })) { return ANALYSIS_ERROR; }
        failureReturned = true;
        changed.notify_all();
        return ANALYSIS_ERROR;
    })));
    graph.emplace(2, Node(Action([&](DataInventory&, const Context&) {
        std::unique_lock<std::mutex> lock(mutex);
        peerStarted = true;
        changed.notify_all();
        if (!changed.wait_for(lock, std::chrono::seconds(3), [&] { return failureReturned; })) { return ANALYSIS_ERROR; }
        peerFinished = true;
        return ANALYSIS_OK;
    }), "device_1"));
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_TRUE(peerFinished);
}

TEST(RuntimeTopology, RejectsCycleAndUnsupportedChip)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    TopoNodeCollection definitions;
    TopoNodeDefinition definition;
    definition.id = {TopoNodeStage::DEVICE_PROCESSING, "cycle"};
    definition.scopeKind = TopoScopeKind::DEVICE;
    definition.creatorFactory = Factory(Action(Success));
    definition.processDependencies = {definition.id};
    definitions.emplace(definition.id, definition);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {{definition.id.stage, definition.id.name, "device_0"}}, graph, &definitions));
    definitions.at(definition.id).processDependencies.clear();
    definitions.at(definition.id).chipIds = {15};
    EXPECT_FALSE(builder.Build(context, {{definition.id.stage, definition.id.name, "device_0"}}, graph, &definitions));
}

TEST(RuntimeTopology, NullCreatorResultIsFailure)
{
    RuntimeProcessCollection graph;
    graph.emplace(1, Node([]() -> std::unique_ptr<Process> { return nullptr; }));
    ProcessControl executor(graph);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_EQ(executor.GetRuntimeResults().at(1).state, RuntimeProcessState::FAILED);
}

TEST(RuntimeTopology, ExceptionsPropagateAndFailedOptionalProducerBlocksRequiredConsumer)
{
    RuntimeProcessCollection graph;
    auto producer = Node(Action([](DataInventory&, const Context&) -> uint32_t { throw std::runtime_error("bad input"); }));
    producer.mandatory = false;
    graph.emplace(1, producer);
    std::atomic<bool> ran{false};
    auto consumer = Node(Action([&](DataInventory&, const Context&) { ran = true; return ANALYSIS_OK; }));
    consumer.processDependence = {1};
    graph.emplace(2, consumer);
    ProcessControl executor(graph);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_FALSE(ran);
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::BLOCKED);
}

TEST(RuntimeTopology, FailureLogsIdentifyExecutedProducerWithoutBlamingBlockedConsumer)
{
    RuntimeProcessCollection graph;
    auto producer = Node(Action([](DataInventory&, const Context&) { return 0x2aU; }));
    producer.processName = "optional_parser";
    producer.context = std::make_shared<TestContext>(15);
    producer.mandatory = false;
    graph.emplace(1, producer);
    auto consumer = Node(Action(Success));
    consumer.processName = "consumer";
    consumer.processDependence = {1};
    graph.emplace(2, consumer);
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool result = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();

    EXPECT_FALSE(result);
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::BLOCKED);
    EXPECT_THAT(
        messages,
        testing::Contains(testing::AllOf(
            testing::Field(&TestLogMessage::level, "[ERROR]"),
            testing::Field(&TestLogMessage::message,
                           "Process failed: node=optional_parser, chip_id=15, return_code=0x2a, mandatory=false."))));
    EXPECT_THAT(messages, testing::Not(testing::Contains(testing::Field(
                              &TestLogMessage::message, testing::HasSubstr("Process failed: node=consumer")))));
    EXPECT_THAT(messages, testing::Contains(testing::AllOf(
        testing::Field(&TestLogMessage::level, "[ERROR]"),
        testing::Field(&TestLogMessage::message,
            "Required process blocked: node=device_0/consumer, "
            "failed_dependencies=[device_0/optional_parser(return_code=0x2a)]."))));
    EXPECT_THAT(prints, testing::ElementsAre(testing::AllOf(
        testing::Field(&TestLogMessage::level, "[ERROR]"),
        testing::Field(&TestLogMessage::message, "Processing failed. Please check the logs for details."))));
}

TEST(RuntimeTopology, RequiredBlockedNodeReportsDistinctRootFailuresAcrossDependencyChains)
{
    RuntimeProcessCollection graph;
    for (RuntimeProcessKey key : {1U, 2U})
    {
        auto producer = Node(Action([](DataInventory&, const Context&) { return 0x2aU; }),
                             "device_" + std::to_string(key));
        producer.processName = "parser";
        producer.mandatory = false;
        graph.emplace(key, producer);
    }
    auto intermediate = Node(Action(Success));
    intermediate.processName = "intermediate";
    intermediate.mandatory = false;
    intermediate.processDependence = {1, 2};
    graph.emplace(3, intermediate);
    auto output = Node(Action(Success), "export");
    output.processName = "consumer";
    output.processDependence = {1, 3};
    graph.emplace(4, output);
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool success = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();
    EXPECT_FALSE(success);
    EXPECT_EQ(executor.GetRuntimeResults().at(4).failedDependencies, (std::vector<RuntimeProcessKey>{1, 2}));
    EXPECT_THAT(messages, testing::Contains(testing::AllOf(
        testing::Field(&TestLogMessage::level, "[ERROR]"),
        testing::Field(&TestLogMessage::message,
            "Required process blocked: node=export/consumer, failed_dependencies=["
            "device_1/parser(return_code=0x2a), device_2/parser(return_code=0x2a)]."))));
    EXPECT_EQ(prints.size(), 1U);
}

TEST(RuntimeTopology, QualifiedDfxStopIsLoggedWithoutFailureOrConsoleDetails)
{
    class StopContext final : public TestContext
    {
       public:
        StopContext() : TestContext(5) {}
        const std::string& GetDfxStopAtName() const override { return stopAt_; }

       private:
        std::string stopAt_ = "device_0/parser";
    };
    RuntimeProcessCollection graph;
    auto parser = Node(Action(Success));
    parser.processName = "parser";
    parser.context = std::make_shared<StopContext>();
    graph.emplace(1, parser);
    auto consumer = Node(Action(Success));
    consumer.processDependence = {1};
    graph.emplace(2, consumer);
    ProcessControl executor(graph);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool result = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();

    EXPECT_FALSE(result);
    EXPECT_TRUE(executor.GetRuntimeResults().at(1).statistics.dfxStop);
    EXPECT_THAT(messages,
                testing::Contains(testing::AllOf(
                    testing::Field(&TestLogMessage::level, "[INFO]"),
                    testing::Field(&TestLogMessage::message,
                                   "DFX stop requested: node=parser, chip_id=5, return_code=0x0, mandatory=true."))));
    EXPECT_THAT(messages, testing::Not(testing::Contains(
                              testing::Field(&TestLogMessage::message, testing::HasSubstr("Process failed:")))));
    EXPECT_THAT(messages, testing::Not(testing::Contains(testing::Field(&TestLogMessage::level, "[ERROR]"))));
    EXPECT_TRUE(prints.empty());
}

TEST(RuntimeTopology, DfxStopDoesNotPromoteUnrelatedOptionalFailureToOverallError)
{
    class StopContext final : public TestContext
    {
       public:
        StopContext() : TestContext(5) {}
        const std::string& GetDfxStopAtName() const override { return name_; }
       private:
        std::string name_ = "parser";
    };
    RuntimeProcessCollection graph;
    auto optional = Node(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; }));
    optional.mandatory = false;
    graph.emplace(1, optional);
    auto parser = Node(Action(Success));
    parser.processName = "parser";
    parser.context = std::make_shared<StopContext>();
    parser.completionDependence = {1};
    graph.emplace(2, parser);
    auto consumer = Node(Action(Success));
    consumer.processDependence = {2};
    graph.emplace(3, consumer);
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool success = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();
    EXPECT_FALSE(success);
    EXPECT_EQ(executor.GetRuntimeResults().at(3).state, RuntimeProcessState::BLOCKED);
    EXPECT_THAT(messages, testing::Not(testing::Contains(testing::Field(
        &TestLogMessage::message, testing::HasSubstr("Required process")))));
    EXPECT_TRUE(prints.empty());
}

TEST(RuntimeTopology, DeviceLimitReleasesAfterLastNodeWithoutCompletionCallbacks)
{
    for (const bool optionalFailure : {false, true})
    {
        RuntimeProcessCollection graph;
        std::atomic<int> active{0};
        std::atomic<int> maximum{0};
        std::atomic<int> completedBranches[3];
        for (uint64_t index = 0; index < 3; ++index)
        {
            completedBranches[index] = 0;
            const auto scope = "device_" + std::to_string(index);
            auto init = Node(Action([&](DataInventory&, const Context&)
            {
                const int count = ++active;
                maximum = std::max(maximum.load(), count);
                return ANALYSIS_OK;
            }), scope);
            init.opensDeviceScope = true;
            graph.emplace(index * 3 + 1, init);
            for (uint64_t branch = 0; branch < 2; ++branch)
            {
                const bool fail = optionalFailure && branch == 1;
                auto work = Node(Action([&, index, fail](DataInventory&, const Context&)
                {
                    if (++completedBranches[index] == 2) { --active; }
                    return fail ? ANALYSIS_ERROR : ANALYSIS_OK;
                }), scope);
                work.processDependence = {index * 3 + 1};
                work.mandatory = !fail;
                graph.emplace(index * 3 + branch + 2, work);
            }
        }
        ProcessControlOptions options;
        options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
        options.maxActiveDevices = 1;
        options.maxWorkerThreads = 2;
        ProcessControl executor(graph, options);
        EXPECT_TRUE(executor.ExecuteProcess());
        EXPECT_EQ(active.load(), 0);
        EXPECT_EQ(maximum.load(), 1);
    }
}

TEST(RuntimeTopology, ExclusiveResourcesAreAcquiredBeforeWorkerDispatch)
{
    RuntimeProcessCollection graph;
    std::atomic<int> active{0};
    std::atomic<bool> overlap{false};
    for (uint64_t key = 1; key <= 12; ++key) {
        auto node = Node(Action([&](DataInventory&, const Context&) {
            if (++active != 1) { overlap = true; }
            for (int index = 0; index < 100; ++index) { std::this_thread::yield(); }
            --active;
            return ANALYSIS_OK;
        }));
        node.resources = {"same-database"};
        graph.emplace(key, node);
    }
    ProcessControlOptions options;
    options.maxWorkerThreads = 4;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_FALSE(overlap);
}

TEST(RuntimeTopology, RejectsUnboundAndMalformedRuntimeGraphs)
{
    RuntimeProcessCollection graph;
    auto node = Node(Action(Success));
    node.context.reset();
    graph.emplace(1, node);
    ProcessControl missingBinding(graph);
    EXPECT_FALSE(missingBinding.ExecuteProcess());
    node = Node(Action(Success));
    node.processDependence = {42};
    graph.emplace(1, node);
    ProcessControl missingDependency(graph);
    EXPECT_FALSE(missingDependency.ExecuteProcess());
    node.processDependence = {1};
    graph.emplace(1, node);
    ProcessControl cycle(graph);
    EXPECT_FALSE(cycle.ExecuteProcess());
}

TEST(RuntimeTopology, CoarseDeviceBodiesRunExistingSchedulerBeforeSharedExport)
{
    TopoBuildContext context;
    BindTest(context, "host", 0);
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    BindTest(context, "export", 0);
    TopoNodeCollection definitions;
    std::atomic<int> hosts{0};
    std::atomic<int> devices{0};
    TopoNodeDefinition host;
    host.id = {TopoNodeStage::HOST_PROCESSING, "HostC", "host"};
    host.scopeKind = TopoScopeKind::HOST;
    host.mandatory = true;
    host.creatorFactory = Factory(Action([&](DataInventory&, const Context&) { ++hosts; return ANALYSIS_OK; }));
    definitions.emplace(host.id, host);
    TopoNodeDefinition device;
    device.id = {TopoNodeStage::DEVICE_PROCESSING, "DeviceC"};
    device.scopeKind = TopoScopeKind::DEVICE;
    device.mandatory = true;
    device.processDependencies = {host.id};
    device.opensDeviceScope = true;
    device.creatorFactory = Factory(Action([&](DataInventory&, const Context& ctx) {
        if (hosts != 1) { return ANALYSIS_ERROR; }
        auto internal = std::make_shared<DataInventory>();
        RuntimeProcessCollection operations;
        operations.emplace(1, RuntimeProcessInfo{Action([](DataInventory& data, const Context& chip) {
            return data.Inject(std::make_shared<int>(chip.GetChipID())) ? ANALYSIS_OK : ANALYSIS_ERROR;
        }), {}, {}, {CHIP_ID_ALL}, "Parser", true});
        operations.emplace(2, RuntimeProcessInfo{Action([](DataInventory& data, const Context& chip) {
            const auto value = data.GetPtr<int>();
            return value && *value == static_cast<int>(chip.GetChipID()) ? ANALYSIS_OK : ANALYSIS_ERROR;
        }), {typeid(int)}, {1}, {CHIP_ID_ALL}, "Persistence", true});
        for (auto& operation : operations)
        {
            operation.second.context = std::make_shared<TestContext>(ctx.GetChipID());
            operation.second.inventory = internal;
        }
        ProcessControlOptions options;
        options.maxWorkerThreads = 1;
        ProcessControl inner(operations, options);
        if (!inner.ExecuteProcess()) { return ANALYSIS_ERROR; }
        ++devices;
        return ANALYSIS_OK;
    }));
    definitions.emplace(device.id, device);
    TopoNodeDefinition output;
    output.id = {TopoNodeStage::FLOW_CONTROL, "Export", "export"};
    output.mandatory = true;
    output.processDependencies = {{device.id.stage, device.id.name, "device_0"},
                                  {device.id.stage, device.id.name, "device_1"}};
    output.creatorFactory = Factory(Action([&](DataInventory&, const Context&) {
        return devices == 2 ? ANALYSIS_OK : ANALYSIS_ERROR;
    }));
    definitions.emplace(output.id, output);
    TopoGraphBuilder builder;
    RuntimeProcessCollection graph;
    ASSERT_TRUE(builder.Build(context, {output.id}, graph, &definitions));
    ASSERT_EQ(graph.size(), 4U);
    ProcessControlOptions options;
    options.maxActiveDevices = 1;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(hosts, 1);
    EXPECT_EQ(devices, 2);
}

TEST(RuntimeTopology, InnerNullProcessFailureBlocksCoarseDeviceExport)
{
    RuntimeProcessCollection graph;
    graph.emplace(1, Node(Action([](DataInventory&, const Context& ctx) {
        RuntimeProcessCollection operations;
        operations.emplace(1, RuntimeProcessInfo{[]() -> std::unique_ptr<Process> { return nullptr; },
                           {}, {}, {CHIP_ID_ALL}, "Parser", true});
        operations.at(1).context = std::make_shared<TestContext>(ctx.GetChipID());
        operations.at(1).inventory = std::make_shared<DataInventory>();
        ProcessControl inner(operations);
        return inner.ExecuteProcess() ? ANALYSIS_OK : ANALYSIS_ERROR;
    })));
    std::atomic<bool> exported{false};
    auto output = Node(Action([&](DataInventory&, const Context&) { exported = true; return ANALYSIS_OK; }), "export");
    output.processDependence = {1};
    graph.emplace(2, output);
    ProcessControl executor(graph);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_FALSE(exported);
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::BLOCKED);
}

namespace {
std::atomic<size_t> nativeProduced{0};
std::atomic<size_t> nativeComputed{0};
std::atomic<bool> nativeConsumed{false};
TopoNodeDefinition NativeDefinition(const std::string& name, ProcessCreator creator,
                                    std::vector<TopoNodeId> dependencies = {}, std::vector<std::type_index> inputs = {},
                                    std::vector<uint32_t> chips = {CHIP_ID_ALL})
{
    TopoNodeDefinition definition;
    definition.id = {TopoNodeStage::DEVICE_PROCESSING, name};
    definition.scopeKind = TopoScopeKind::DEVICE;
    definition.mandatory = true;
    definition.creatorFactory = Factory(std::move(creator));
    definition.processDependencies = std::move(dependencies);
    definition.inputDataTypes = std::move(inputs);
    definition.chipIds = std::move(chips);
    return definition;
}
TopoNodeDefinition NativeInit()
{
    auto definition = NativeDefinition("Config.Init", Action(Success));
    definition.opensDeviceScope = true;
    return definition;
}
TopoNodeDefinition NativePersistence()
{
    auto definition =
        NativeDefinition("Config.Persistence",
                         Action(
                             [](DataInventory& data, const Context& context)
                             {
                                 const auto value = data.GetPtr<int>();
                                 if (!value || *value != static_cast<int>(context.GetChipID()))
                                 {
                                     return ANALYSIS_ERROR;
                                 }
                                 ++nativeProduced;
                                 return data.Inject(std::make_shared<long>(*value)) ? ANALYSIS_OK : ANALYSIS_ERROR;
                             }),
                         TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Parser")), {typeid(int)});
    definition.id.stage = TopoNodeStage::DEVICE_PERSISTENCE;
    return definition;
}
TopoNodeDefinition NativeAlternativeConsumer()
{
    auto definition =
        NativeDefinition("Config.Alternative", Action(Success), TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Init")));
    definition.dependencyResolver = [](const TopoBuildContext& context, const std::vector<TopoNodeId>&)
    {
        const auto chip = context.scopeChips.at(context.currentScope);
        return TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, chip == 5 ? "Config.ModelV4" : "Config.ModelV6"));
    };
    return definition;
}
REGISTER_TOPO_NODE_DEFINITION(NativeInit());
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition(
    "Config.Parser",
    Action([](DataInventory& data, const Context& context)
           { return data.Inject(std::make_shared<int>(context.GetChipID())) ? ANALYSIS_OK : ANALYSIS_ERROR; }),
    TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Init"))));
REGISTER_TOPO_NODE_DEFINITION(NativePersistence());
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.Calculation",
                                               Action(
                                                   [](DataInventory& data, const Context& context)
                                                   {
                                                       const auto value = data.GetPtr<long>();
                                                       if (!value || *value != static_cast<long>(context.GetChipID()))
                                                       {
                                                           return ANALYSIS_ERROR;
                                                       }
                                                       ++nativeComputed;
                                                       return ANALYSIS_OK;
                                                   }),
                                               TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "Config.Persistence")),
                                               {typeid(long)}));
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.UnusedMissing", Action(Success),
                                               TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Missing"))));
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.ModelV4", Action(Success),
                                               TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Init")), {}, {5}));
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.ModelV6", Action(Success),
                                               TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Init")), {}, {15}));
REGISTER_TOPO_NODE_DEFINITION(NativeAlternativeConsumer());
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.Failure",
                                               Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; })));
REGISTER_TOPO_NODE_DEFINITION(NativeDefinition("Config.FailedConsumer",
                                               Action(
                                                   [](DataInventory&, const Context&)
                                                   {
                                                       nativeConsumed = true;
                                                       return ANALYSIS_OK;
                                                   }),
                                               TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Failure"))));
}

TEST(RuntimeTopology, NativeConfigurationPrunesUnselectedMissingBranches)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context,
                              {{TopoNodeStage::DEVICE_PERSISTENCE, "Config.Persistence", "device_0"},
                               {TopoNodeStage::DEVICE_PERSISTENCE, "Config.Persistence", "device_1"}},
                              graph));
    EXPECT_EQ(graph.size(), 6U);
    for (const auto& node : graph)
    {
        EXPECT_NE(node.second.processName, "Config.UnusedMissing");
    }
    EXPECT_FALSE(
        builder.Build(context, {{TopoNodeStage::DEVICE_PROCESSING, "Config.UnusedMissing", "device_0"}}, graph));
    EXPECT_TRUE(graph.empty());
}

TEST(RuntimeTopology, NativeConfigurationSelectsChipAlternativesPerDevice)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context,
                              {{TopoNodeStage::DEVICE_PROCESSING, "Config.Alternative", "device_0"},
                               {TopoNodeStage::DEVICE_PROCESSING, "Config.Alternative", "device_1"}},
                              graph));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV4", "device_0"}), 1U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV4", "device_1"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV6", "device_0"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV6", "device_1"}), 1U);
}

TEST(RuntimeTopology, NativeConfigurationPreservesInputMetadata)
{
    const auto* definition = TopoNodeRegistry::Find(TOPO_NODE(DEVICE_PERSISTENCE, "Config.Persistence"));
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->inputDataTypes, std::vector<std::type_index>{typeid(int)});
    EXPECT_TRUE(TopoNodeRegistry::SupportsChip(*definition, 5));
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{definition->id.stage, definition->id.name, "device_0"}}, graph));
    const auto key = builder.GetRuntimeNodeKeys().at({definition->id.stage, definition->id.name, "device_0"});
    EXPECT_EQ(graph.at(key).paramTypes, definition->inputDataTypes);
}

TEST(RuntimeTopology, NativeDevicesReleaseSlotsBeforeCrossDeviceExportAndPreserveMemoryEffects)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    context.scopeContexts["export"] = std::make_shared<TestContext>(0);
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    nativeProduced = 0;
    nativeComputed = 0;
    TopoNodeCollection definitions;
    TopoNodeDefinition output;
    output.id = {TopoNodeStage::DATA_PROCESSING, "Aggregate", "export"};
    output.creatorFactory =
        Factory(Action([](DataInventory&, const Context&)
                       { return nativeProduced == 2 && nativeComputed == 2 ? ANALYSIS_OK : ANALYSIS_ERROR; }));
    std::vector<TopoNodeId> roots{output.id};
    for (const auto& scope : {"device_0", "device_1"}) {
        const TopoNodeId operation{TopoNodeStage::DEVICE_PROCESSING, "Config.Calculation", scope};
        output.processDependencies.push_back(operation);
    }
    definitions.emplace(output.id, output);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, graph, &definitions));
    ProcessControlOptions options;
    options.maxActiveDevices = 1;
    options.maxWorkerThreads = 2;
    ProcessControl executor(graph, options);
    EXPECT_TRUE(executor.ExecuteProcess());
    EXPECT_EQ(nativeProduced.load(), 2U);
    EXPECT_EQ(nativeComputed.load(), 2U);
}

TEST(RuntimeTopology, NativeRequiredProducerFailureBlocksItsConsumer)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    nativeConsumed = false;
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(
        builder.Build(context, {{TopoNodeStage::DEVICE_PROCESSING, "Config.FailedConsumer", "device_0"}}, graph));
    const auto consumer =
        builder.GetRuntimeNodeKeys().at({TopoNodeStage::DEVICE_PROCESSING, "Config.FailedConsumer", "device_0"});
    ProcessControl executor(graph);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_FALSE(nativeConsumed);
    EXPECT_EQ(executor.GetRuntimeResults().at(consumer).state, RuntimeProcessState::BLOCKED);
}

namespace
{
TopoNodeDefinition ExportWithDeviceDependencies(std::vector<TopoNodeId> dependencies)
{
    TopoNodeDefinition output;
    output.id = TOPO_NODE(DATA_PROCESSING, "Config.Export");
    output.runtimeType = typeid(double);
    output.creatorFactory = Factory(Action(Success));
    output.processDependencies = std::move(dependencies);
    return output;
}
void BindExport(TopoBuildContext& context)
{
    context.scopeContexts["export"] = std::make_shared<TestContext>(0);
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
}
}  // namespace

TEST(RuntimeTopology, UnscopedDeviceDependenciesExpandPerRequestedDevice)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    BindExport(context);
    auto output = ExportWithDeviceDependencies(TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "Config.Persistence")));
    TopoNodeCollection definitions{{output.id, output}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {output.id}, graph, &definitions));
    EXPECT_EQ(graph.size(), 7U);
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto key = keys.at({output.id.stage, output.id.name, "export"});
    const auto& dependencies = graph.at(key).processDependence;
    for (const auto& scope : {"device_0", "device_1"})
    {
        const auto producer = keys.at({TopoNodeStage::DEVICE_PERSISTENCE, "Config.Persistence", scope});
        EXPECT_NE(std::find(dependencies.begin(), dependencies.end(), producer), dependencies.end());
    }
    ProcessControl executor(graph);
    EXPECT_TRUE(executor.ExecuteProcess());
}

TEST(RuntimeTopology, UnscopedDeviceDependenciesFilterChipAlternatives)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    BindExport(context);
    auto output = ExportWithDeviceDependencies(
        TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.ModelV4"), TOPO_NODE(DEVICE_PROCESSING, "Config.ModelV6")));
    TopoNodeCollection definitions{{output.id, output}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {output.id}, graph, &definitions));
    const auto& keys = builder.GetRuntimeNodeKeys();
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV4", "device_0"}), 1U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV4", "device_1"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV6", "device_0"}), 0U);
    EXPECT_EQ(keys.count({TopoNodeStage::DEVICE_PROCESSING, "Config.ModelV6", "device_1"}), 1U);
}

TEST(RuntimeTopology, ReusedDeviceSkipsItsProducerInstances)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    BindExport(context);
    context.scopeReuse["device_0"] = true;
    auto output = ExportWithDeviceDependencies(TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "Config.Persistence")));
    TopoNodeCollection definitions{{output.id, output}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {output.id}, graph, &definitions));
    EXPECT_EQ(graph.size(), 4U);
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PERSISTENCE, "Config.Persistence", "device_0"}),
              0U);
    EXPECT_EQ(builder.GetRuntimeNodeKeys().count({TopoNodeStage::DEVICE_PERSISTENCE, "Config.Persistence", "device_1"}),
              1U);
}

TEST(RuntimeTopology, UnscopedUnknownDeviceDependencyFailsBeforeExecution)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindExport(context);
    auto output = ExportWithDeviceDependencies(TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Config.Unknown")));
    TopoNodeCollection definitions{{output.id, output}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {output.id}, graph, &definitions));
    EXPECT_TRUE(graph.empty());
}

TEST(RuntimeTopology, StandaloneExportBuildDoesNotScheduleDeviceParsing)
{
    TopoBuildContext context;
    BindExport(context);
    auto output = ExportWithDeviceDependencies(TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "Config.Persistence")));
    ASSERT_TRUE(TopoNodeRegistry::Register(output));
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    EXPECT_TRUE(builder.Build(context, {output.id}, graph));
    EXPECT_EQ(graph.size(), 1U);
    TopoNodeRegistry::MutableDefinitions().erase(output.id);
}

TEST(RuntimeTopology, IndependentModeRunsUnrelatedWorkAfterMandatoryFailure)
{
    RuntimeProcessCollection graph;
    graph[1] = Node(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; }));
    std::atomic<int> independent{0};
    graph[2] = Node(Action([&](DataInventory&, const Context&) { ++independent; return ANALYSIS_OK; }));
    graph[2].completionDependence = {1};
    graph[3] = Node(Action(Success));
    graph[3].processDependence = {1};
    graph[4] = Node(Action(Success));
    graph[4].processDependence = {3};
    graph[5] = Node(Action(Success));
    graph[5].completionDependence = {2, 4};
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    options.maxWorkerThreads = 1;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    EXPECT_EQ(independent.load(), 1);
    EXPECT_EQ(results.at(1).state, RuntimeProcessState::FAILED);
    EXPECT_EQ(results.at(2).state, RuntimeProcessState::SUCCEEDED);
    EXPECT_EQ(results.at(3).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(4).failedDependencies, (std::vector<RuntimeProcessKey>{1}));
    EXPECT_EQ(results.at(5).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, DependencyFailurePolicyWaitsForAllPredecessorsAndKeepsNormalNodesBlocked)
{
    RuntimeProcessCollection graph;
    graph[1] = Node(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; }));
    std::atomic<int> completed{0};
    graph[2] = Node(Action([&](DataInventory&, const Context&) { ++completed; return ANALYSIS_OK; }));
    graph[2].completionDependence = {1};
    graph[3] = Node(Action(Success));
    graph[3].processDependence = {1};
    std::atomic<int> handled{0};
    graph[4] = Node(Action([&](DataInventory&, const Context&)
    {
        EXPECT_EQ(completed.load(), 1);
        ++handled;
        return ANALYSIS_OK;
    }));
    graph[4].processDependence = {1, 2, 3};
    graph[4].runOnDependencyFailure = true;
    graph[5] = Node(Action(Success));
    graph[5].processDependence = {4};
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    const auto results = executor.GetRuntimeResults();
    EXPECT_EQ(handled.load(), 1);
    EXPECT_EQ(results.at(3).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(results.at(4).state, RuntimeProcessState::SUCCEEDED);
    EXPECT_EQ(results.at(5).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, CompilerPreservesDependencyFailurePolicyForResolvedDependencies)
{
    TopoBuildContext context;
    BindTest(context, "export", 0);
    TopoNodeDefinition producer;
    producer.id = {TopoNodeStage::DATA_PROCESSING, "FailingProducer"};
    producer.mandatory = true;
    producer.creatorFactory = Factory(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; }));
    TopoNodeDefinition normal;
    normal.id = {TopoNodeStage::SUMMARY_GENERATION, "NormalConsumer"};
    normal.mandatory = true;
    normal.creatorFactory = Factory(Action(Success));
    const auto dependency = producer.id;
    normal.dependencyResolver = [dependency](const TopoBuildContext&, const std::vector<TopoNodeId>&)
    { return std::vector<TopoNodeId>{dependency}; };
    auto handler = normal;
    handler.id = {TopoNodeStage::FLOW_CONTROL, "Handler"};
    handler.runOnDependencyFailure = true;
    TopoNodeCollection definitions{{producer.id, producer}, {normal.id, normal}, {handler.id, handler}};
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {normal.id, handler.id}, graph, &definitions));
    const auto normalKey = builder.GetRuntimeNodeKeys().at({normal.id.stage, normal.id.name, "export"});
    const auto handlerKey = builder.GetRuntimeNodeKeys().at({handler.id.stage, handler.id.name, "export"});
    EXPECT_FALSE(graph.at(normalKey).runOnDependencyFailure);
    EXPECT_TRUE(graph.at(handlerKey).runOnDependencyFailure);
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_EQ(executor.GetRuntimeResults().at(normalKey).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(executor.GetRuntimeResults().at(handlerKey).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, RuntimeBindingsPreserveDependencyFailurePolicy)
{
    RuntimeProcessCollection operations;
    operations.emplace(1, Node(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; })));
    std::atomic<int> calls{0};
    auto handler = Node(Action([&](DataInventory&, const Context&) { ++calls; return ANALYSIS_OK; }));
    handler.processDependence = {1};
    handler.runOnDependencyFailure = true;
    operations.emplace(2, handler);
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    ProcessControl executor(operations, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_EQ(calls.load(), 1);
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, BlockedDeviceNodesReleaseSlotWithoutRunningCallbacks)
{
    RuntimeProcessCollection graph;
    graph[1] = Node(Action([](DataInventory&, const Context&) { return ANALYSIS_ERROR; }));
    graph[1].opensDeviceScope = true;
    graph[2] = Node(Action(Success));
    graph[2].processDependence = {1};
    std::atomic<int> markedComplete{0};
    graph[3] = Node(Action([&](DataInventory&, const Context&) { ++markedComplete; return ANALYSIS_OK; }));
    graph[3].processDependence = {1, 2};
    std::atomic<int> nextDevice{0};
    graph[4] = Node(Action([&](DataInventory&, const Context&) { ++nextDevice; return ANALYSIS_OK; }), "device_1");
    // Ensure device_0 acquired its slot first, while device_1 remains independent of its success.
    graph[4].completionDependence = {1};
    graph[4].opensDeviceScope = true;
    graph[5] = Node(Action(Success), "device_1");
    graph[5].completionDependence = {4};
    ProcessControlOptions options;
    options.failureMode = FailureMode::CONTINUE_INDEPENDENT;
    options.maxActiveDevices = 1;
    ProcessControl executor(graph, options);
    EXPECT_FALSE(executor.ExecuteProcess());
    EXPECT_EQ(markedComplete.load(), 0);
    EXPECT_EQ(executor.GetRuntimeResults().at(3).state, RuntimeProcessState::BLOCKED);
    EXPECT_EQ(nextDevice.load(), 1);
    EXPECT_EQ(executor.GetRuntimeResults().at(5).state, RuntimeProcessState::SUCCEEDED);
}

TEST(RuntimeTopology, CompletionDependenciesParticipateInValidation)
{
    RuntimeProcessCollection graph;
    graph[1] = Node(Action(Success));
    graph[1].completionDependence = {2};
    ProcessControl missing(graph);
    EXPECT_FALSE(missing.ExecuteProcess());
    graph[1] = Node(Action(Success));
    graph[1].completionDependence = {2};
    graph[2] = Node(Action(Success));
    graph[2].processDependence = {1};
    ProcessControl cycle(graph);
    EXPECT_FALSE(cycle.ExecuteProcess());
}

TEST(RuntimeTopology, CompilerExpandsCompletionDependenciesAndPrefersSuccessEdges)
{
    TopoBuildContext context;
    BindTest(context, "device_0", 5);
    BindTest(context, "device_1", 15);
    context.scopeContexts["export"] = std::make_shared<TestContext>(0);
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    TopoNodeCollection overlay;
    TopoNodeDefinition producer;
    producer.id = TOPO_NODE(DEVICE_PROCESSING, "Completion.Producer");
    producer.scopeKind = TopoScopeKind::DEVICE;
    producer.creatorFactory = Factory(Action(Success));
    overlay.emplace(producer.id, producer);
    TopoNodeDefinition consumer;
    consumer.id = TOPO_NODE(FLOW_CONTROL, "Completion.Consumer");
    consumer.creatorFactory = Factory(Action(Success));
    consumer.completionDependencies = {producer.id};
    overlay.emplace(consumer.id, consumer);
    RuntimeProcessCollection graph;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {consumer.id}, graph, &overlay));
    const auto key = builder.GetRuntimeNodeKeys().at({consumer.id.stage, consumer.id.name, "export"});
    EXPECT_EQ(graph.at(key).completionDependence.size(), 2U);
    overlay.at(consumer.id).processDependencies = {producer.id};
    ASSERT_TRUE(builder.Build(context, {consumer.id}, graph, &overlay));
    const auto merged = builder.GetRuntimeNodeKeys().at({consumer.id.stage, consumer.id.name, "export"});
    EXPECT_EQ(graph.at(merged).processDependence.size(), 2U);
    EXPECT_TRUE(graph.at(merged).completionDependence.empty());
}

TEST(RuntimeTopology, GlobalStopCancelsNodesEvenWhenTheyAllowDependencyFailure)
{
    class StopContext : public TestContext
    {
    public:
        StopContext() : TestContext(5) {}
        const std::string& GetDfxStopAtName() const override { return name_; }
    private:
        std::string name_ = "stop";
    };
    for (const bool dfx : {false, true})
    {
        SCOPED_TRACE(dfx ? "DFX stop" : "fail-fast");
        RuntimeProcessCollection graph;
        graph[1] = Node(Action([dfx](DataInventory&, const Context&)
            { return dfx ? ANALYSIS_OK : ANALYSIS_ERROR; }));
        if (dfx) { graph[1].context = std::make_shared<StopContext>(); }
        graph[1].processName = "stop";
        graph[2] = Node(Action(Success));
        graph[2].completionDependence = {1};
        std::atomic<int> executedAfterStop{0};
        graph[3] = Node(Action([&](DataInventory&, const Context&) { ++executedAfterStop; return ANALYSIS_OK; }));
        graph[3].processDependence = {2};
        graph[3].runOnDependencyFailure = true;
        graph[4] = Node(Action(Success));
        graph[4].processDependence = {1};
        graph[4].runOnDependencyFailure = true;
        ProcessControlOptions options;
        options.failureMode = dfx ? FailureMode::CONTINUE_INDEPENDENT : FailureMode::STOP_ON_MANDATORY_FAILURE;
        ProcessControl executor(graph, options);
        StartTestLogCapture();
        StartTestPrintCapture();
        const bool success = executor.ExecuteProcess();
        const auto messages = StopTestLogCapture();
        const auto prints = StopTestPrintCapture();
        EXPECT_FALSE(success);
        const auto results = executor.GetRuntimeResults();
        EXPECT_EQ(results.at(2).state, RuntimeProcessState::CANCELLED);
        EXPECT_EQ(results.at(3).state, RuntimeProcessState::CANCELLED);
        EXPECT_EQ(results.at(4).state, RuntimeProcessState::CANCELLED);
        EXPECT_EQ(executedAfterStop.load(), 0);
        if (dfx)
        {
            EXPECT_TRUE(prints.empty());
            EXPECT_THAT(messages, testing::Not(testing::Contains(testing::Field(&TestLogMessage::level, "[ERROR]"))));
        }
        else
        {
            EXPECT_EQ(prints.size(), 1U);
            EXPECT_THAT(messages, testing::Contains(testing::AllOf(
                testing::Field(&TestLogMessage::level, "[ERROR]"),
                testing::Field(&TestLogMessage::message,
                    "Required process not executed: node=device_0/test, state=CANCELLED, "
                    "execution_failures=[device_0/stop(return_code=0x1)]."))));
        }
    }
}
