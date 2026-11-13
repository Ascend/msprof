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

#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <unordered_set>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Application
{
namespace
{
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

bool TopoNodeRegistry::SupportsChip(const TopoNodeDefinition& definition, uint32_t chip)
{
    return definition.chipIds.empty() ||
           std::find(definition.chipIds.begin(), definition.chipIds.end(), CHIP_ID_ALL) != definition.chipIds.end() ||
           std::find(definition.chipIds.begin(), definition.chipIds.end(), chip) != definition.chipIds.end();
}

TopoNodeSequenceRegister::TopoNodeSequenceRegister(TopoNodeDefinition definition)
{
    if (!TopoNodeRegistry::Register(std::move(definition)))
    {
        ERROR("Register topology definition failed.");
    }
}

TopoNodeSequenceRegister::TopoNodeSequenceRegister(TopoNodeId id, std::type_index runtimeType, bool mandatory,
                                                   std::vector<TopoNodeId> dependencies,
                                                   TopoNodeCreatorFactory creatorFactory,
                                                   TopoDependencyResolver dependencyResolver,
                                                   bool runOnDependencyFailure)
{
    TopoNodeDefinition definition;
    definition.id = std::move(id);
    definition.runtimeType = runtimeType;
    definition.mandatory = mandatory;
    definition.processDependencies = std::move(dependencies);
    definition.creatorFactory = std::move(creatorFactory);
    definition.dependencyResolver = std::move(dependencyResolver);
    definition.runOnDependencyFailure = runOnDependencyFailure;
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

namespace
{
class TopoGraphCompiler
{
   public:
    TopoGraphCompiler(const TopoBuildContext& context, const std::vector<TopoNodeId>& roots,
                      const TopoNodeCollection* overlay, Infra::RuntimeProcessCollection& processes,
                      std::unordered_map<TopoNodeId, Infra::RuntimeProcessKey, TopoNodeIdHash>& keys)
        : context_(context), roots_(roots), overlay_(overlay), processes_(processes), keys_(keys)
    {
    }

    bool Add(TopoNodeId id, const std::string& parentScope, Infra::RuntimeProcessKey& key)
    {
        const auto* definition = Find(id);
        if (!definition || !ResolveScope(*definition, parentScope, id))
        {
            ERROR("Unknown or ambiguous runtime node %/%.", id.scope, id.name);
            return false;
        }
        // Dependencies may be registered without scope; prefer this request's scoped override after resolution.
        definition = Find(id);
        const auto existing = keys_.find(id);
        if (existing != keys_.end())
        {
            key = existing->second;
            return true;
        }
        if (!visiting_.insert(id).second)
        {
            ERROR("Runtime topology contains a cycle at %/%.", id.scope, id.name);
            return false;
        }
        if (definition->scopeKind == TopoScopeKind::DEVICE)
        {
            const auto chip = context_.scopeChips.find(id.scope);
            if (chip == context_.scopeChips.end() || !TopoNodeRegistry::SupportsChip(*definition, chip->second))
            {
                ERROR("Unsupported chip for runtime node %/%.", id.scope, id.name);
                return false;
            }
        }
        TopoBuildContext instanceContext = context_;
        instanceContext.currentScope = id.scope;
        auto dependencies = definition->processDependencies;
        if (definition->dependencyResolver)
        {
            auto resolved = definition->dependencyResolver(instanceContext, roots_);
            dependencies.insert(dependencies.end(), resolved.begin(), resolved.end());
        }
        auto completionDependencies = definition->completionDependencies;
        if (definition->completionDependencyResolver)
        {
            auto resolved = definition->completionDependencyResolver(instanceContext, roots_);
            completionDependencies.insert(completionDependencies.end(), resolved.begin(), resolved.end());
        }
        if (definition->scopeKind != TopoScopeKind::DEVICE)
        {
            dependencies = ExpandDeviceDependencies(dependencies);
            completionDependencies = ExpandDeviceDependencies(completionDependencies);
        }
        Infra::RuntimeProcessInfo info;
        auto compileDependencies = [&](const std::vector<TopoNodeId>& inputs,
                                       std::vector<Infra::RuntimeProcessKey>& output) -> bool
        {
            for (const auto& dependency : inputs)
            {
                Infra::RuntimeProcessKey dependencyKey = 0;
                if (!Add(dependency, id.scope, dependencyKey))
                {
                    ERROR("Dependency %/% required by %/% failed.", dependency.scope, dependency.name, id.scope,
                          id.name);
                    return false;
                }
                if (std::find(info.processDependence.begin(), info.processDependence.end(), dependencyKey) ==
                        info.processDependence.end() &&
                    std::find(output.begin(), output.end(), dependencyKey) == output.end())
                {
                    output.push_back(dependencyKey);
                }
            }
            return true;
        };
        if (!compileDependencies(dependencies, info.processDependence) ||
            !compileDependencies(completionDependencies, info.completionDependence))
        {
            return false;
        }
        const auto context = context_.scopeContexts.find(id.scope);
        const auto inventory = context_.scopeInventories.find(id.scope);
        if (!definition->creatorFactory || context == context_.scopeContexts.end() ||
            inventory == context_.scopeInventories.end() || !context->second || !inventory->second)
        {
            ERROR("Missing runtime binding for %/%.", id.scope, id.name);
            return false;
        }
        info.creator = definition->creatorFactory(instanceContext);
        if (!info.creator)
        {
            ERROR("Topology creator factory returned an empty creator for %/%, profile path: %.", id.scope, id.name,
                  context_.profPath);
            return false;
        }
        info.context = context->second;
        info.inventory = inventory->second;
        info.paramTypes = definition->inputDataTypes;
        info.chipIds = definition->chipIds;
        info.mandatory = definition->mandatory;
        info.processName = id.name;
        info.scope = id.scope;
        info.resources = definition->resources;
        std::sort(info.resources.begin(), info.resources.end());
        info.resources.erase(std::unique(info.resources.begin(), info.resources.end()), info.resources.end());
        info.opensDeviceScope = definition->opensDeviceScope;
        info.runOnDependencyFailure = definition->runOnDependencyFailure;
        key = ++nextKey_;
        processes_.emplace(key, std::move(info));
        keys_.emplace(id, key);
        visiting_.erase(id);
        return true;
    }

   private:
    std::vector<TopoNodeId> ExpandDeviceDependencies(const std::vector<TopoNodeId>& dependencies) const
    {
        std::vector<TopoNodeId> expanded;
        for (const auto& dependency : dependencies)
        {
            if ((dependency.stage != TopoNodeStage::DEVICE_PROCESSING &&
                 dependency.stage != TopoNodeStage::DEVICE_PERSISTENCE) ||
                !dependency.scope.empty())
            {
                expanded.push_back(dependency);
                continue;
            }
            // An unscoped Device dependency means the applicable instance on each requested device.
            // Chip alternatives and reused whole-device results do not create redundant work.
            for (const auto& scope : context_.scopeChips)
            {
                const auto reuse = context_.scopeReuse.find(scope.first);
                if (reuse != context_.scopeReuse.end() && reuse->second)
                {
                    continue;
                }
                auto instance = dependency;
                instance.scope = scope.first;
                const auto* definition = Find(instance);
                if (!definition || TopoNodeRegistry::SupportsChip(*definition, scope.second))
                {
                    expanded.push_back(std::move(instance));
                }
            }
        }
        return expanded;
    }

    const TopoNodeDefinition* Find(const TopoNodeId& id) const
    {
        TopoNodeId normalized{id.stage, id.name, ""};
        if (overlay_)
        {
            auto found = overlay_->find(id);
            if (found != overlay_->end())
            {
                return &found->second;
            }
            found = overlay_->find(normalized);
            if (found != overlay_->end())
            {
                return &found->second;
            }
        }
        return TopoNodeRegistry::Find(normalized);
    }

    bool ResolveScope(const TopoNodeDefinition& definition, const std::string& parent, TopoNodeId& id) const
    {
        std::string expected;
        switch (definition.scopeKind)
        {
            case TopoScopeKind::PROFILE:
                expected = "profile";
                break;
            case TopoScopeKind::HOST:
                expected = "host";
                break;
            case TopoScopeKind::EXPORT:
                expected = "export";
                break;
            case TopoScopeKind::DEVICE:
                expected = id.scope.empty() ? parent : id.scope;
                if (context_.scopeChips.count(expected) == 0)
                {
                    return false;
                }
                break;
        }
        if (!id.scope.empty() && id.scope != expected)
        {
            return false;
        }
        id.scope = expected;
        return true;
    }
    const TopoBuildContext& context_;
    const std::vector<TopoNodeId>& roots_;
    const TopoNodeCollection* overlay_;
    Infra::RuntimeProcessCollection& processes_;
    std::unordered_map<TopoNodeId, Infra::RuntimeProcessKey, TopoNodeIdHash>& keys_;
    std::unordered_set<TopoNodeId, TopoNodeIdHash> visiting_;
    Infra::RuntimeProcessKey nextKey_ = 0;
};
}  // namespace

bool TopoGraphBuilder::Build(const TopoBuildContext& context, const std::vector<TopoNodeId>& roots,
                             Infra::RuntimeProcessCollection& processes, const TopoNodeCollection* overlay)
{
    processes.clear();
    runtimeNodeKeys_.clear();
    if (roots.empty())
    {
        ERROR("Cannot build runtime topology: roots are empty, profile path: %.", context.profPath);
        return false;
    }
    try
    {
        TopoGraphCompiler compiler(context, roots, overlay, processes, runtimeNodeKeys_);
        for (const auto& root : roots)
        {
            Infra::RuntimeProcessKey key = 0;
            if (!compiler.Add(root, "", key))
            {
                processes.clear();
                runtimeNodeKeys_.clear();
                return false;
            }
        }
    }
    catch (const std::exception& error)
    {
        ERROR("Build runtime graph failed, profile path: %, reason: %.", context.profPath, error.what());
        processes.clear();
        runtimeNodeKeys_.clear();
        return false;
    }
    return true;
}
}  // namespace Application
}  // namespace Analysis
