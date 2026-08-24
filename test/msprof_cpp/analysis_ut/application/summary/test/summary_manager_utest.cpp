/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string PROF_PATH = "./summary_test/PROF_0";
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

TEST(SummaryManagerUTest, ShouldBuildOnlySelectedDeliverable)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({"npu_memory"}, roots));
    ASSERT_EQ(roots.size(), 1UL);
    EXPECT_EQ(roots.front(), (TopoNodeId{TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_NPU_MEM}));

    TopoBuildContext context;
    context.profPath = PROF_PATH;
    context.outputPath = RESULT_PATH;
    ProcessCollection processes;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, processes));

    const auto summary = std::find_if(
        processes.begin(), processes.end(), [](const ProcessCollection::value_type& process) -> bool
        {
            return process.second.processName == PROCESSOR_NAME_NPU_MEM &&
                   process.second.paramTypes.size() == 1UL;
        });
    ASSERT_NE(summary, processes.end());
    EXPECT_EQ(summary->second.processDependence.size(), 1UL);
    EXPECT_EQ(summary->second.paramTypes.size(), 1UL);
}

TEST(SummaryManagerUTest, ShouldSelectAllDeliverablesWhenSelectionIsEmpty)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({}, roots));
    EXPECT_EQ(roots.size(), 10UL);
    std::unordered_set<TopoNodeId, TopoNodeIdHash> uniqueRoots;
    for (const auto& root : roots)
    {
        EXPECT_EQ(root.stage, TopoNodeStage::SUMMARY_GENERATION);
        EXPECT_NE(TopoNodeRegistry::Find(root), nullptr);
        EXPECT_TRUE(uniqueRoots.insert(root).second);
    }
}

TEST(SummaryManagerUTest, ShouldSelectAndDeduplicateDeliverables)
{
    std::vector<std::string> assemblers;
    ASSERT_TRUE(SummaryManager::GetAssemblerList({"op_summary", "op_summary", "npu_memory"}, assemblers));
    ASSERT_EQ(assemblers.size(), 2UL);
    EXPECT_EQ(assemblers[0], PROCESSOR_OP_SUMMARY);
    EXPECT_EQ(assemblers[1], PROCESSOR_NAME_NPU_MEM);
}

TEST(SummaryManagerUTest, ShouldRejectUnknownDeliverable)
{
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(SummaryManager::GetTopologyRoots({"unknown_deliverable"}, roots));
    EXPECT_TRUE(roots.empty());
}

TEST(SummaryManagerUTest, ShouldRejectExecutionListNodeWithoutRegistration)
{
    TopologyRegistryGuard guard;
    TopoNodeRegistry::MutableDefinitions().erase({TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_NPU_MEM});
    std::vector<TopoNodeId> roots;
    EXPECT_FALSE(SummaryManager::GetTopologyRoots({"npu_memory"}, roots));
    EXPECT_TRUE(roots.empty());
}
