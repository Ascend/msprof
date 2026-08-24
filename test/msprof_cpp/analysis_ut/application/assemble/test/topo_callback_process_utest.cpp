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

#include <algorithm>
#include <memory>
#include <typeindex>
#include <utility>

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include "analysis/csrc/infrastructure/context/include/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

using namespace Analysis;
using namespace Analysis::Application;
using namespace Analysis::Infra;

namespace
{
struct GraphProcessor
{
};
struct GraphInput
{
};

class TopologyRegistryGuard
{
   public:
    TopologyRegistryGuard() : backup_(TopoNodeRegistry::GetDefinitions()) {}
    ~TopologyRegistryGuard() { TopoNodeRegistry::MutableDefinitions() = backup_; }

   private:
    TopoNodeCollection backup_;
};

TopoNodeCreatorFactory SuccessfulCreator()
{
    return [](const TopoBuildContext&)
    {
        return []() -> std::unique_ptr<Process>
        { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return true; })); };
    };
}
}  // namespace

TEST(TopoCallbackProcessUTest, ShouldPropagateCallbackResult)
{
    DataInventory dataInventory;
    Context context;

    TopoCallbackProcess success([](DataInventory&) { return true; });
    TopoCallbackProcess failure([](DataInventory&) { return false; });
    TopoCallbackProcess empty(nullptr);

    EXPECT_EQ(success.Run(dataInventory, context), ANALYSIS_OK);
    EXPECT_EQ(failure.Run(dataInventory, context), ANALYSIS_ERROR);
    EXPECT_EQ(empty.Run(dataInventory, context), ANALYSIS_ERROR);
}

TEST(TopoNodeRegistrationUTest, ShouldRegisterStaticExportNodesWithInputData)
{
    const auto* summary = TopoNodeRegistry::Find({TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_OP_SUMMARY});
    const auto* db = TopoNodeRegistry::Find({TopoNodeStage::DATABASE_PERSISTENCE, "DB:" + PROCESSOR_NAME_API});
    const auto* timeline = TopoNodeRegistry::Find({TopoNodeStage::TIMELINE_EXPORT, Domain::PROCESS_TASK});

    ASSERT_NE(summary, nullptr);
    ASSERT_NE(db, nullptr);
    ASSERT_NE(timeline, nullptr);
    EXPECT_FALSE(summary->inputDataTypes.empty());
    EXPECT_FALSE(db->inputDataTypes.empty());
    EXPECT_FALSE(timeline->inputDataTypes.empty());
}

TEST(TopoNodeRegistrationUTest, ShouldRegisterNetDevStatsProcessorAsOptional)
{
    const auto* netDevStats = TopoNodeRegistry::Find({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_NETDEV_STATS});
    ASSERT_NE(netDevStats, nullptr);
    EXPECT_FALSE(netDevStats->mandatory);
}

TEST(TopoGraphBuilderUTest, ShouldResolveControlDependenciesUsingAllocatedNodeKeys)
{
    TopologyRegistryGuard guard;
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    definitions.clear();

    TopoNodeDefinition processor;
    processor.id = {TopoNodeStage::DATA_PROCESSING, "processor"};
    processor.runtimeType = typeid(GraphProcessor);
    processor.creatorFactory = SuccessfulCreator();
    TopoNodeDefinition pre;
    pre.id = {TopoNodeStage::FLOW_CONTROL, "pre"};
    pre.creatorFactory = SuccessfulCreator();
    TopoNodeDefinition timeline;
    timeline.id = {TopoNodeStage::TIMELINE_EXPORT, "timeline"};
    timeline.processDependencies = {{TopoNodeStage::FLOW_CONTROL, "pre"}, {TopoNodeStage::DATA_PROCESSING, "processor"}};
    timeline.inputDataTypes = {typeid(GraphInput)};
    timeline.creatorFactory = SuccessfulCreator();
    TopoNodeDefinition post;
    post.id = {TopoNodeStage::FLOW_CONTROL, "post"};
    post.dependencyResolver = [](const TopoBuildContext&, const std::vector<TopoNodeId>& roots)
    {
        std::vector<TopoNodeId> dependencies;
        for (const auto& root : roots)
        {
            if (root.stage == TopoNodeStage::TIMELINE_EXPORT)
            {
                dependencies.push_back(root);
            }
        }
        return dependencies;
    };
    post.creatorFactory = SuccessfulCreator();
    definitions.emplace(processor.id, std::move(processor));
    definitions.emplace(pre.id, std::move(pre));
    definitions.emplace(timeline.id, std::move(timeline));
    definitions.emplace(post.id, std::move(post));

    ProcessCollection processes;
    TopoBuildContext context;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{TopoNodeStage::TIMELINE_EXPORT, "timeline"}, {TopoNodeStage::FLOW_CONTROL, "post"}},
                              processes));
    ASSERT_EQ(processes.size(), 4UL);

    std::type_index timelineKey(typeid(void));
    const RegProcessInfo* postInfo = nullptr;
    for (const auto& process : processes)
    {
        if (process.second.processName == "timeline")
        {
            timelineKey = process.first;
        }
        if (process.second.processName == "post")
        {
            postInfo = &process.second;
        }
    }
    ASSERT_NE(timelineKey, std::type_index(typeid(void)));
    ASSERT_NE(postInfo, nullptr);
    EXPECT_NE(std::find(postInfo->processDependence.begin(), postInfo->processDependence.end(), timelineKey),
              postInfo->processDependence.end());
    EXPECT_EQ(processes.at(timelineKey).paramTypes, std::vector<std::type_index>{typeid(GraphInput)});
}

TEST(TopoGraphBuilderUTest, ShouldClearProcessesWhenRootIsUnregistered)
{
    TopologyRegistryGuard guard;
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    definitions.clear();

    ProcessCollection processes;

    TopoBuildContext context;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {{TopoNodeStage::SUMMARY_GENERATION, "missing"}}, processes));
    EXPECT_TRUE(processes.empty());
}

TEST(TopoGraphBuilderUTest, ShouldCreateSharedDependencyOnlyOnce)
{
    TopologyRegistryGuard guard;
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    definitions.clear();

    TopoNodeDefinition processor;
    processor.id = {TopoNodeStage::DATA_PROCESSING, "processor"};
    processor.runtimeType = typeid(GraphProcessor);
    processor.creatorFactory = SuccessfulCreator();
    TopoNodeDefinition summary;
    summary.id = {TopoNodeStage::SUMMARY_GENERATION, "summary"};
    summary.processDependencies = {{TopoNodeStage::DATA_PROCESSING, "processor"}};
    summary.creatorFactory = SuccessfulCreator();
    TopoNodeDefinition timeline;
    timeline.id = {TopoNodeStage::TIMELINE_EXPORT, "timeline"};
    timeline.processDependencies = {{TopoNodeStage::DATA_PROCESSING, "processor"}};
    timeline.creatorFactory = SuccessfulCreator();
    definitions.emplace(processor.id, std::move(processor));
    definitions.emplace(summary.id, std::move(summary));
    definitions.emplace(timeline.id, std::move(timeline));

    ProcessCollection processes;
    TopoBuildContext context;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{TopoNodeStage::SUMMARY_GENERATION, "summary"}, {TopoNodeStage::TIMELINE_EXPORT, "timeline"}},
                              processes));
    EXPECT_EQ(processes.size(), 3UL);
    EXPECT_EQ(processes.count(typeid(GraphProcessor)), 1UL);
}

TEST(TopoGraphBuilderUTest, ShouldContinueWhenOptionalNodeFails)
{
    TopologyRegistryGuard guard;
    auto& definitions = TopoNodeRegistry::MutableDefinitions();
    definitions.clear();

    TopoNodeDefinition optional;
    optional.id = {TopoNodeStage::DATA_PROCESSING, "optional"};
    optional.runtimeType = typeid(GraphProcessor);
    optional.mandatory = false;
    optional.chipIds = {CHIP_ID_ALL};
    optional.creatorFactory = [](const TopoBuildContext&)
    {
        return []() -> std::unique_ptr<Process>
        { return std::unique_ptr<Process>(new TopoCallbackProcess([](DataInventory&) { return false; })); };
    };

    const std::shared_ptr<bool> requiredRan(new bool(false));
    TopoNodeDefinition required;
    required.id = {TopoNodeStage::SUMMARY_GENERATION, "required"};
    required.mandatory = true;
    required.chipIds = {CHIP_ID_ALL};
    required.creatorFactory = [requiredRan](const TopoBuildContext&)
    {
        return [requiredRan]() -> std::unique_ptr<Process>
        {
            return std::unique_ptr<Process>(new TopoCallbackProcess(
                [requiredRan](DataInventory&) { *requiredRan = true; return true; }));
        };
    };
    definitions.emplace(optional.id, std::move(optional));
    definitions.emplace(required.id, std::move(required));

    ProcessCollection processes;
    TopoBuildContext buildContext;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(buildContext, {{TopoNodeStage::DATA_PROCESSING, "optional"}, {TopoNodeStage::SUMMARY_GENERATION, "required"}},
                              processes));

    DataInventory dataInventory;
    Context processContext;
    ProcessControl processControl(processes);
    EXPECT_TRUE(processControl.ExecuteProcess(dataInventory, processContext));
    EXPECT_TRUE(*requiredRan);
}
