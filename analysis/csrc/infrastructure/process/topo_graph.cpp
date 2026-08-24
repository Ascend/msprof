/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

#include <algorithm>
#include <memory>
#include <new>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Application
{
namespace
{
template <TopoNodeStage Stage, size_t Index>
struct TopoNodeTag
{
};

template <TopoNodeStage Stage, size_t Index>
struct TopoNodeTypeGetter
{
    static std::type_index Get(size_t index)
    {
        return index == Index ? std::type_index(typeid(TopoNodeTag<Stage, Index>))
                              : TopoNodeTypeGetter<Stage, Index - 1>::Get(index);
    }
};

template <TopoNodeStage Stage>
struct TopoNodeTypeGetter<Stage, 0>
{
    static std::type_index Get(size_t) { return std::type_index(typeid(TopoNodeTag<Stage, 0>)); }
};

TopoNodeCollection& Definitions()
{
    static TopoNodeCollection definitions;
    return definitions;
}
}  // namespace

bool TopoNodeRegistry::Register(TopoNodeDefinition definition)
{
    if (definition.id.name.empty() || definition.creatorFactory == nullptr)
    {
        ERROR("Invalid topology node registration.");
        return false;
    }
    if (definition.chipIds.empty())
    {
        definition.chipIds = {CHIP_ID_ALL};
    }
    return Definitions().emplace(definition.id, std::move(definition)).second;
}

const TopoNodeCollection& TopoNodeRegistry::GetDefinitions() { return Definitions(); }

TopoNodeCollection& TopoNodeRegistry::MutableDefinitions() { return Definitions(); }

TopoNodeSequenceRegister::TopoNodeSequenceRegister(TopoNodeId id, std::type_index runtimeType, bool mandatory,
                                                   std::vector<TopoNodeId> dependencies,
                                                   TopoNodeCreatorFactory creatorFactory,
                                                   TopoDependencyResolver dependencyResolver)
{
    TopoNodeDefinition definition;
    definition.id = std::move(id);
    definition.runtimeType = runtimeType;
    definition.mandatory = mandatory;
    definition.processDependencies = std::move(dependencies);
    definition.creatorFactory = std::move(creatorFactory);
    definition.dependencyResolver = std::move(dependencyResolver);
    if (!TopoNodeRegistry::Register(std::move(definition)))
    {
        ERROR("Register topology sequence failed.");
    }
}

TopoNodeDataRegister::TopoNodeDataRegister(TopoNodeId id, std::vector<std::type_index> inputDataTypes)
{
    const auto* definition = TopoNodeRegistry::Find(id);
    if (definition == nullptr)
    {
        ERROR("Register topology data before topology sequence.");
        return;
    }
    TopoNodeRegistry::MutableDefinitions().at(definition->id).inputDataTypes = std::move(inputDataTypes);
}

const TopoNodeDefinition* TopoNodeRegistry::Find(const TopoNodeId& id)
{
    const auto iter = Definitions().find(id);
    return iter == Definitions().end() ? nullptr : &iter->second;
}

const TopoNodeDefinition* TopoNodeRegistry::FindProcessorByName(const std::string& name)
{
    return Find({TopoNodeStage::DATA_PROCESSING, name});
}

const TopoNodeDefinition* TopoNodeRegistry::FindProcessorByRuntimeType(std::type_index runtimeType)
{
    for (const auto& item : Definitions())
    {
        if (item.second.id.stage == TopoNodeStage::DATA_PROCESSING && item.second.runtimeType == runtimeType)
        {
            return &item.second;
        }
    }
    return nullptr;
}

bool TopoGraphBuilder::Build(const TopoBuildContext& context, const std::vector<TopoNodeId>& roots,
                             Infra::ProcessCollection& processes)
{
    if (roots.empty())
    {
        ERROR("Topology roots are empty.");
        return false;
    }
    context_ = &context;
    roots_ = &roots;
    processes_ = &processes;
    nodeKeys_.clear();
    nextSyntheticIndex_.clear();
    visiting_.clear();
    processes.clear();
    for (const auto& root : roots)
    {
        std::type_index key(typeid(void));
        if (!AddNode(root, key))
        {
            processes.clear();
            return false;
        }
    }
    return true;
}

bool TopoGraphBuilder::ResolveDependencies(const TopoNodeDefinition& definition,
                                           std::vector<std::type_index>& dependencies)
{
    std::vector<TopoNodeId> ids = definition.processDependencies;
    if (definition.dependencyResolver != nullptr)
    {
        const auto dynamicDependencies = definition.dependencyResolver(*context_, *roots_);
        ids.insert(ids.end(), dynamicDependencies.begin(), dynamicDependencies.end());
    }
    for (const auto& id : ids)
    {
        std::type_index key(typeid(void));
        if (!AddNode(id, key))
        {
            return false;
        }
        if (std::find(dependencies.begin(), dependencies.end(), key) == dependencies.end())
        {
            dependencies.emplace_back(key);
        }
    }
    return true;
}

bool TopoGraphBuilder::AddNode(const TopoNodeId& id, std::type_index& key)
{
    const auto keyIter = nodeKeys_.find(id);
    if (keyIter != nodeKeys_.end())
    {
        key = keyIter->second;
        return true;
    }
    if (visiting_[id])
    {
        ERROR("Topology contains a cycle at %.", id.name);
        return false;
    }
    const auto* definition = TopoNodeRegistry::Find(id);
    if (definition == nullptr)
    {
        ERROR("Topology contains an unregistered node %.", id.name);
        return false;
    }
    visiting_[id] = true;
    std::vector<std::type_index> dependencies;
    if (!ResolveDependencies(*definition, dependencies))
    {
        return false;
    }
    key = definition->id.stage == TopoNodeStage::DATA_PROCESSING ? definition->runtimeType
                                                                 : AllocateSyntheticKey(definition->id.stage);
    if (key == std::type_index(typeid(void)))
    {
        ERROR("Failed to allocate topology key for %.", definition->id.name);
        return false;
    }
    Infra::RegProcessInfo processInfo;
    processInfo.creator = definition->creatorFactory(*context_);
    processInfo.processDependence = std::move(dependencies);
    processInfo.paramTypes = definition->inputDataTypes;
    processInfo.chipIds = definition->chipIds;
    processInfo.processName = definition->id.name;
    processInfo.mandatory = definition->mandatory;
    if (processInfo.creator == nullptr || !processes_->emplace(key, std::move(processInfo)).second)
    {
        ERROR("Failed to append topology node %.", definition->id.name);
        return false;
    }
    nodeKeys_.emplace(id, key);
    visiting_.erase(id);
    return true;
}

std::type_index TopoGraphBuilder::AllocateSyntheticKey(TopoNodeStage stage)
{
    const size_t index = nextSyntheticIndex_[stage]++;
    if (index >= MAX_TOPO_NODE_COUNT)
    {
        ERROR("Too many topology nodes in stage %.", static_cast<uint8_t>(stage));
        return std::type_index(typeid(void));
    }
    switch (stage)
    {
        case TopoNodeStage::SUMMARY_GENERATION:
            return TopoNodeTypeGetter<TopoNodeStage::SUMMARY_GENERATION, MAX_TOPO_NODE_COUNT - 1>::Get(index);
        case TopoNodeStage::DATABASE_PERSISTENCE:
            return TopoNodeTypeGetter<TopoNodeStage::DATABASE_PERSISTENCE, MAX_TOPO_NODE_COUNT - 1>::Get(index);
        case TopoNodeStage::TIMELINE_EXPORT:
            return TopoNodeTypeGetter<TopoNodeStage::TIMELINE_EXPORT, MAX_TOPO_NODE_COUNT - 1>::Get(index);
        case TopoNodeStage::FLOW_CONTROL:
            return TopoNodeTypeGetter<TopoNodeStage::FLOW_CONTROL, MAX_TOPO_NODE_COUNT - 1>::Get(index);
        default:
            return std::type_index(typeid(void));
    }
}
}  // namespace Application
}  // namespace Analysis
