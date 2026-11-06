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
 */ \
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "device_context.h"

namespace Analysis
{
namespace Domain
{

bool RunSingleDevice(const std::string &devicePath, DeviceContext &context, Infra::DataInventory &inventory,
                     std::string &statistics, const char *stopAt)
{
    statistics.clear();
    const auto expectedChip = context.GetChipID();
    if (!context.Init(devicePath) || (expectedChip != 0 && context.GetChipID() != expectedChip))
    {
        statistics = "Init failed or device metadata changed.";
        return false;
    }
    const auto sqlite = Utils::File::PathJoin({devicePath, Common::SQLITE});
    if (!Utils::File::CreateDir(sqlite))
    {
        statistics = "Create sqlite directory failed.";
        return false;
    }
    if (stopAt != nullptr)
    {
        const std::string stopName(stopAt);
        const auto delimiter = stopName.rfind("::");
        context.SetStopAt(delimiter == std::string::npos ? stopName : stopName.substr(delimiter + 2));
    }
    Application::TopoBuildContext bindings;
    auto scopePath = devicePath;
    while (!scopePath.empty() && scopePath.back() == '/')
    {
        scopePath.pop_back();
    }
    const auto scope = Utils::File::BaseName(scopePath);
    bindings.scopeChips[scope] = context.GetChipID();
    bindings.scopeContexts[scope] = std::shared_ptr<Infra::Context>(&context, [](Infra::Context *) {});
    bindings.scopeInventories[scope] = std::shared_ptr<Infra::DataInventory>(&inventory, [](Infra::DataInventory *) {});
    bindings.scopePaths[scope] = devicePath;
    Application::TopoNodeCollection definitions;
    std::vector<Application::TopoNodeId> roots;
    for (const auto &entry : Application::TopoNodeRegistry::GetDefinitions())
    {
        const auto &definition = entry.second;
        if (definition.scopeKind == Application::TopoScopeKind::DEVICE && !definition.opensDeviceScope &&
            !definition.closesDeviceScope &&
            Application::TopoNodeRegistry::SupportsChip(definition, context.GetChipID()))
        {
            auto id = definition.id;
            id.scope = scope;
            roots.push_back(std::move(id));
        }
    }
    Application::TopoNodeDefinition init;
    // Resolve unscoped Init dependencies without relying on a global lifecycle registration.
    init.id = {Application::TopoNodeStage::DEVICE_PROCESSING, "Device.Init", ""};
    init.scopeKind = Application::TopoScopeKind::DEVICE;
    init.creatorFactory = [](const Application::TopoBuildContext &) -> Infra::ProcessCreator
    {
        return []() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(
                new Application::TopoCallbackProcess([](Infra::DataInventory &) { return true; }));
        };
    };
    definitions.emplace(init.id, std::move(init));
    Infra::RuntimeProcessCollection processes;
    Application::TopoGraphBuilder builder;
    if (!builder.Build(bindings, roots, processes, &definitions))
    {
        statistics = "Build device topology failed.";
        return false;
    }
    Infra::ProcessControlOptions options;
    options.releaseUnusedData = false;  // The legacy caller receives the completed inventory.
    Infra::ProcessControl control(processes, options);
    const bool success = control.ExecuteProcess();
    Infra::RecordProcessStat(control.GetExecuteStat(), devicePath, statistics);
    return success;
}
}  // namespace Domain
}  // namespace Analysis
