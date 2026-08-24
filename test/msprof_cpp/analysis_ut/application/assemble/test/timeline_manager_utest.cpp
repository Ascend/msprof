/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include <algorithm>
#include <memory>
#include <typeindex>
#include <utility>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"

#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/infrastructure/context/include/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
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
    context.profPath = PROF_PATH;
    context.outputPath = RESULT_PATH;
    context.timelineProcesses = selection;
    context.timelineSession = std::make_shared<TimelineManager>(PROF_PATH, RESULT_PATH);
    ProcessCollection processes;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, processes));

    ProcessCollection::const_iterator pre = processes.end();
    ProcessCollection::const_iterator timeline = processes.end();
    const RegProcessInfo* postDump = nullptr;
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
    const std::type_index preKey = pre->first;
    const std::type_index timelineKey = timeline->first;
    EXPECT_NE(std::find(timeline->second.processDependence.begin(), timeline->second.processDependence.end(), preKey),
              timeline->second.processDependence.end());
    EXPECT_NE(std::find(postDump->processDependence.begin(), postDump->processDependence.end(), timelineKey),
              postDump->processDependence.end());
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

TEST(TimelineManagerUTest, ShouldWriteValidEmptyJsonWhenNoAssemblerWritesEvents)
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
    manager.PostDumpJson();

    const std::vector<std::string> files = File::GetOriginData(outputPath, {MSPROF_JSON_FILE}, {});
    ASSERT_EQ(files.size(), 1UL);
    FileReader reader(files.front());
    nlohmann::json content;
    ASSERT_EQ(reader.ReadJson(content), Analysis::ANALYSIS_OK);
    EXPECT_TRUE(content.empty());
    EXPECT_TRUE(File::RemoveDir(basePath, 0));
}
