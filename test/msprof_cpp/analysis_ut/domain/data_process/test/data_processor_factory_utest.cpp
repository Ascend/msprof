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

#include <set>
#include <string>
#include <typeindex>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/aicpu_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "gtest/gtest.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;

namespace
{
const std::string PROF_PATH = "./data_processor_factory/PROF_0";

class TopologyRegistryGuard
{
   public:
    TopologyRegistryGuard() : backup_(TopoNodeRegistry::GetDefinitions()) {}

    ~TopologyRegistryGuard() { TopoNodeRegistry::MutableDefinitions() = backup_; }

    TopoNodeCollection& Registry() { return TopoNodeRegistry::MutableDefinitions(); }

   private:
    TopoNodeCollection backup_;
};

struct ProcessorA
{
};
struct ProcessorB
{
};
}  // namespace

TEST(DataProcessorFactoryUTest, ShouldRejectEmptyProcessorSet)
{
    RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {}, processes));
    EXPECT_TRUE(processes.empty());
}

TEST(DataProcessorFactoryUTest, ShouldRejectUnknownProcessorAndClearPartialTopology)
{
    RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context, {{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_HASH}}, processes));
    ASSERT_FALSE(processes.empty());

    EXPECT_FALSE(builder.Build(
        context,
        {{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_HASH}, {TopoNodeStage::DATA_PROCESSING, "unknown_processor"}},
        processes));
    EXPECT_TRUE(processes.empty());
}

TEST(DataProcessorFactoryUTest, ShouldBuildSharedDependencyOnlyOnce)
{
    RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    ASSERT_TRUE(builder.Build(context,
                              {{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION},
                               {TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO}},
                              processes));

    EXPECT_EQ(processes.size(), 3UL);
    const auto& keys = builder.GetRuntimeNodeKeys();
    const auto hash = keys.at({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_HASH, "export"});
    const auto communication = keys.at({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION, "export"});
    const auto compute = keys.at({TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO, "export"});
    EXPECT_EQ(processes.at(communication).processDependence, std::vector<RuntimeProcessKey>{hash});
    EXPECT_EQ(processes.at(compute).processDependence, std::vector<RuntimeProcessKey>{hash});
    EXPECT_EQ(processes.at(hash).chipIds, std::vector<uint32_t>{CHIP_ID_ALL});
}

TEST(DataProcessorFactoryUTest, ShouldExposeRegisteredProcessorDefinition)
{
    const auto* definition = TopoNodeRegistry::FindProcessorByName(PROCESSOR_NAME_HASH);
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->runtimeType, std::type_index(typeid(HashInitProcessor)));
    EXPECT_TRUE(static_cast<bool>(definition->creatorFactory));
    EXPECT_EQ(TopoNodeRegistry::FindProcessorByName("unknown_processor"), nullptr);
}

TEST(DataProcessorFactoryUTest, ShouldKeepRegistryNamesCreatorsAndDependenciesConsistent)
{
    const auto& registry = TopoNodeRegistry::GetDefinitions();
    ASSERT_FALSE(registry.empty());
    std::set<std::string> processorNames;

    for (const auto& processor : registry)
    {
        const auto& definition = processor.second;
        if (definition.id.stage != TopoNodeStage::DATA_PROCESSING)
        {
            continue;
        }
        EXPECT_FALSE(definition.id.name.empty());
        EXPECT_TRUE(static_cast<bool>(definition.creatorFactory));
        EXPECT_TRUE(processorNames.insert(definition.id.name).second);
        for (const auto& dependency : definition.processDependencies)
        {
            if (dependency.stage == TopoNodeStage::DEVICE_PROCESSING)
            {
                EXPECT_TRUE(dependency.scope.empty());
                continue;  // This partial UT target links export processors only.
            }
            EXPECT_EQ(dependency.stage, TopoNodeStage::DATA_PROCESSING);
            EXPECT_NE(TopoNodeRegistry::Find(dependency), nullptr);
        }
    }
}

TEST(DataProcessorFactoryUTest, ShouldRegisterAicpuWithDirectDeviceDependencies)
{
    const auto* definition = TopoNodeRegistry::FindProcessorByName(PROCESSOR_NAME_AICPU);
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->processDependencies, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                                         TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                                         TOPO_NODE(DEVICE_PROCESSING, "AicpuPersistence")));
    EXPECT_EQ(definition->inputDataTypes,
              (std::vector<std::type_index>{typeid(std::vector<AscendTaskData>), typeid(std::vector<TaskInfoData>)}));
}

TEST(DataProcessorFactoryUTest, ShouldPreserveOptionalProcessorWithoutNativeDependencies)
{
    const auto* definition = TopoNodeRegistry::FindProcessorByName(PROCESSOR_NAME_NETDEV_STATS);
    ASSERT_NE(definition, nullptr);
    EXPECT_FALSE(definition->mandatory);
    EXPECT_TRUE(definition->processDependencies.empty());
    EXPECT_EQ(definition->scopeKind, TopoScopeKind::EXPORT);
}

TEST(DataProcessorFactoryUTest, ShouldExpressChipAlternativesAsDeviceDependencies)
{
    const auto* definition = TopoNodeRegistry::FindProcessorByName(PROCESSOR_NAME_AICORE_FREQ);
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->processDependencies, TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "FreqPersistence"),
                                                         TOPO_NODE(DEVICE_PROCESSING, "LpmInfoPersistence")));
    EXPECT_TRUE(definition->id.scope.empty());
}

TEST(DataProcessorFactoryUTest, ShouldRejectUnregisteredDependency)
{
    TopologyRegistryGuard guard;
    auto& registry = guard.Registry();
    registry.clear();
    TopoNodeDefinition processor;
    processor.id = {TopoNodeStage::DATA_PROCESSING, "processor_a"};
    processor.runtimeType = typeid(ProcessorA);
    processor.processDependencies = {{TopoNodeStage::DATA_PROCESSING, "missing_processor"}};
    processor.creatorFactory = [](const TopoBuildContext&) { return Infra::ProcessCreator(); };
    registry.emplace(processor.id, std::move(processor));

    RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {{TopoNodeStage::DATA_PROCESSING, "processor_a"}}, processes));
    EXPECT_TRUE(processes.empty());
}

TEST(DataProcessorFactoryUTest, ShouldRejectProcessorDependencyCycle)
{
    TopologyRegistryGuard guard;
    auto& registry = guard.Registry();
    registry.clear();
    TopoNodeDefinition processorA;
    processorA.id = {TopoNodeStage::DATA_PROCESSING, "processor_a"};
    processorA.runtimeType = typeid(ProcessorA);
    processorA.processDependencies = {{TopoNodeStage::DATA_PROCESSING, "processor_b"}};
    processorA.creatorFactory = [](const TopoBuildContext&) { return Infra::ProcessCreator(); };
    TopoNodeDefinition processorB;
    processorB.id = {TopoNodeStage::DATA_PROCESSING, "processor_b"};
    processorB.runtimeType = typeid(ProcessorB);
    processorB.processDependencies = {{TopoNodeStage::DATA_PROCESSING, "processor_a"}};
    processorB.creatorFactory = [](const TopoBuildContext&) { return Infra::ProcessCreator(); };
    registry.emplace(processorA.id, std::move(processorA));
    registry.emplace(processorB.id, std::move(processorB));

    RuntimeProcessCollection processes;
    TopoBuildContext context;
    context.scopeContexts["export"] = std::make_shared<Analysis::Infra::Context>();
    context.scopeInventories["export"] = std::make_shared<DataInventory>();
    context.profPath = PROF_PATH;
    TopoGraphBuilder builder;
    EXPECT_FALSE(builder.Build(context, {{TopoNodeStage::DATA_PROCESSING, "processor_a"}}, processes));
    EXPECT_TRUE(processes.empty());
}
