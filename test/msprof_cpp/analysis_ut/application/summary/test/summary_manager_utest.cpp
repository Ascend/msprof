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
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_factory.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string PROF_PATH = "./summary_test/PROF_0";

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
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    RuntimeProcessCollection processes;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, roots, processes));

    const auto summary = std::find_if(
        processes.begin(), processes.end(), [](const RuntimeProcessCollection::value_type& process) -> bool
        { return process.second.processName == PROCESSOR_NAME_NPU_MEM && process.second.paramTypes.size() == 1UL; });
    ASSERT_NE(summary, processes.end());
    EXPECT_EQ(summary->second.processDependence.size(), 1UL);
    EXPECT_EQ(summary->second.paramTypes.size(), 1UL);
}

TEST(SummaryManagerUTest, ShouldSelectAllDeliverablesWhenSelectionIsEmpty)
{
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({}, roots));
    EXPECT_EQ(roots.size(), 14UL);
    std::unordered_set<TopoNodeId, TopoNodeIdHash> uniqueRoots;
    for (const auto& root : roots)
    {
        EXPECT_EQ(root.stage, TopoNodeStage::SUMMARY_GENERATION);
        EXPECT_NE(TopoNodeRegistry::Find(root), nullptr);
        EXPECT_TRUE(uniqueRoots.insert(root).second);
    }
}

TEST(SummaryManagerUTest, ShouldRegisterHbmSummaryForCppExport)
{
    EXPECT_TRUE(SummaryManager::IsDeliverableSupported("hbm"));
    EXPECT_NE(nullptr, SummaryFactory::GetAssemblerByName(PROCESSOR_NAME_HBM, PROF_PATH));
    EXPECT_NE(nullptr, TopoNodeRegistry::Find({TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_HBM}));
    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({}, roots));
    const TopoNodeId hbmRoot{TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_HBM};
    EXPECT_NE(roots.end(), std::find(roots.begin(), roots.end(), hbmRoot));
    roots.clear();
    EXPECT_TRUE(SummaryManager::GetTopologyRoots({"hbm"}, roots));
    EXPECT_EQ(roots, (std::vector<TopoNodeId>{hbmRoot}));
}

TEST(SummaryManagerUTest, ShouldRegisterUbAndLlcReadWriteForCppExport)
{
    for (const auto &selection : std::vector<std::pair<std::string, std::string>>{
             {"ub", PROCESSOR_NAME_UB}, {"llc_read_write", PROCESSOR_NAME_LLC}})
    {
        EXPECT_TRUE(SummaryManager::IsDeliverableSupported(selection.first));
        EXPECT_NE(nullptr, SummaryFactory::GetAssemblerByName(selection.second, PROF_PATH));
        const TopoNodeId root{TopoNodeStage::SUMMARY_GENERATION, selection.second};
        EXPECT_NE(nullptr, TopoNodeRegistry::Find(root));
        std::vector<TopoNodeId> roots;
        ASSERT_TRUE(SummaryManager::GetTopologyRoots({selection.first}, roots));
        EXPECT_EQ(roots, (std::vector<TopoNodeId>{root}));
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

TEST(SummaryManagerUTest, ShouldDeduplicateAicpuFamilyDeliverables)
{
    std::vector<std::string> assemblers;
    ASSERT_TRUE(SummaryManager::GetAssemblerList({"aicpu", "dp", "aicpu_mi", "aicpu"}, assemblers));
    ASSERT_EQ(assemblers.size(), 1UL);
    EXPECT_EQ(assemblers[0], PROCESSOR_NAME_AICPU);

    std::vector<TopoNodeId> roots;
    ASSERT_TRUE(SummaryManager::GetTopologyRoots({"dp", "aicpu_mi"}, roots));
    ASSERT_EQ(roots.size(), 1UL);
    EXPECT_EQ(roots.front(), (TopoNodeId{TopoNodeStage::SUMMARY_GENERATION, PROCESSOR_NAME_AICPU}));
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
