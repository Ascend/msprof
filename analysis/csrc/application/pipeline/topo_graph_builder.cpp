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
#include <exception>
#include <map>
#include <memory>
#include <set>

#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/pipeline/unified_pipeline.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
namespace
{
using namespace Infra;
using namespace Domain;
using namespace Utils;
const std::string DATABASE_SUFFIX = ".db";
const std::string EXPORTED_DB_PREFIX = DB_NAME_MSPROF_DB + "_";

TopoNodeId Control(const std::string& name) { return {TopoNodeStage::FLOW_CONTROL, name, "profile"}; }
TopoNodeId HostNode(const std::string& name) { return {TopoNodeStage::HOST_PROCESSING, name, "host"}; }
TopoNodeId DeviceNode(const std::string& name, const std::string& scope)
{
    return {TopoNodeStage::DEVICE_PROCESSING, name, scope};
}

bool CheckDatabase(const std::string& path, bool& hasTables)
{
    hasTables = false;
    if (!File::IsFile(path))
    {
        ERROR("Database file is missing or invalid: %.", path);
        return false;
    }
    // SQLite permits an empty database before the first table is created.
    if (File::Size(path) == 0)
    {
        return true;
    }
    DBRunner runner(path);
    std::vector<std::tuple<std::string>> rows;
    if (!runner.QueryData("PRAGMA quick_check", rows) || rows.size() != 1 || std::get<0>(rows[0]) != "ok")
    {
        ERROR("Database integrity check failed: %.", path);
        return false;
    }
    rows.clear();
    if (!runner.QueryData("SELECT name FROM sqlite_master WHERE type='table' LIMIT 1", rows))
    {
        ERROR("Read database schema failed: %.", path);
        return false;
    }
    hasTables = !rows.empty();
    return true;
}

bool HealthyDatabase(const std::string& path)
{
    bool hasTables = false;
    if (!CheckDatabase(path, hasTables))
    {
        return false;
    }
    if (!hasTables)
    {
        ERROR("Exported database contains no tables: %.", path);
        return false;
    }
    return true;
}

bool ValidateExportDatabases(const std::string& profPath)
{
    for (const auto& path : File::GetFilesWithPrefix(profPath, EXPORTED_DB_PREFIX))
    {
        if (EndsWith(path, DATABASE_SUFFIX) && !HealthyDatabase(path))
        {
            return false;
        }
    }
    return true;
}

bool IntermediateReady(const std::string& path, bool& ready)
{
    const auto sqlite = File::PathJoin({path, "sqlite"});
    ready = false;
    if (!File::Exist(sqlite))
    {
        return true;
    }
    const auto files = File::GetFilesWithPrefix(sqlite, "");
    for (const auto& file : files)
    {
        if (file.size() >= 3 && file.compare(file.size() - 3, 3, ".db") == 0)
        {
            bool hasTables = false;
            if (!CheckDatabase(file, hasTables))
            {
                return false;
            }
            // Optional Python readers may leave empty databases when no records were collected.
            ready = ready || hasTables;
        }
    }
    return true;
}

void Bind(TopoBuildContext& context, const std::string& scope, std::shared_ptr<Infra::Context> value)
{
    context.scopeContexts[scope] = std::move(value);
    context.scopeInventories[scope] = std::make_shared<DataInventory>();
}

void AddCallback(TopoNodeCollection& definitions, const TopoNodeId& id, TopoScopeKind scope,
                 std::vector<TopoNodeId> dependencies, std::function<bool()> action,
                 bool runOnDependencyFailure = false)
{
    TopoNodeDefinition definition;
    definition.id = id;
    definition.scopeKind = scope;
    definition.mandatory = true;
    definition.processDependencies = std::move(dependencies);
    definition.runOnDependencyFailure = runOnDependencyFailure;
    definition.creatorFactory = [action](const TopoBuildContext&)
    {
        return [action]() -> std::unique_ptr<Process>
        { return std::unique_ptr<Process>(new TopoCallbackProcess([action](DataInventory&) { return action(); })); };
    };
    definitions.emplace(id, std::move(definition));
}

ProcessCreator Callback(std::function<bool()> action)
{
    return [action]() -> std::unique_ptr<Process>
    { return std::unique_ptr<Process>(new TopoCallbackProcess([action](DataInventory&) { return action(); })); };
}

ProcessCreator CreateEnvironment(const TopoBuildContext& context)
{
    if (context.profPath.empty())
    {
        ERROR("Cannot bind Environment.Load: profile path is empty.");
        return {};
    }
    const auto path = context.profPath;
    const bool environmentLoaded = context.environmentLoaded;
    return Callback(
        [path, environmentLoaded]
        {
            Host::Cann::HashData::GetInstance().Clear();
            Host::Cann::TypeData::GetInstance().Clear();
            Credential::IdPool::GetInstance().Clear();
            Host::Cann::RTAddInfoCenter::GetInstance().Clear();
            if (environmentLoaded)
            {
                return true;
            }
            Domain::Environment::Context::GetInstance().Clear();
            return Domain::Environment::Context::GetInstance().Load({path});
        });
}

ProcessCreator CreateHost(const TopoBuildContext& context)
{
    const auto pathEntry = context.scopePaths.find(context.currentScope);
    const auto reuseEntry = context.scopeReuse.find(context.currentScope);
    if (pathEntry == context.scopePaths.end() || pathEntry->second.empty() || reuseEntry == context.scopeReuse.end())
    {
        ERROR("Cannot bind HostC: missing path or reuse state for scope %, profile path: %.", context.currentScope,
              context.profPath);
        return {};
    }
    const auto path = pathEntry->second;
    const bool reused = reuseEntry->second;
    return Callback(
        [path, reused]
        {
            if (reused)
            {
                return true;
            }
            const auto sqlite = File::PathJoin({path, "sqlite"});
            if (!File::CreateDir(sqlite))
            {
                return false;
            }
            KernelParserWorker worker(path);
            return worker.Run(false) == ANALYSIS_OK;
        });
}

ProcessCreator CreateDeviceInit(const TopoBuildContext& context)
{
    const auto pathEntry = context.scopePaths.find(context.currentScope);
    const auto reuseEntry = context.scopeReuse.find(context.currentScope);
    const auto chipEntry = context.scopeChips.find(context.currentScope);
    const auto binding = context.scopeContexts.find(context.currentScope);
    if (pathEntry == context.scopePaths.end() || pathEntry->second.empty() || reuseEntry == context.scopeReuse.end() ||
        chipEntry == context.scopeChips.end() || binding == context.scopeContexts.end())
    {
        ERROR("Cannot bind Device.Init: missing path, reuse state, chip or context for scope %, profile path: %.",
              context.currentScope, context.profPath);
        return {};
    }
    const auto device = std::dynamic_pointer_cast<DeviceContext>(binding->second);
    if (!device)
    {
        ERROR("Cannot bind Device.Init: context is not a DeviceContext for scope %, profile path: %.",
              context.currentScope, context.profPath);
        return {};
    }
    const auto path = pathEntry->second;
    const auto chip = chipEntry->second;
    const bool reused = reuseEntry->second;
    return Callback(
        [path, device, chip, reused]
        {
            if (reused)
            {
                return true;
            }
            const auto sqlite = File::PathJoin({path, "sqlite"});
            if (!File::CreateDir(sqlite))
            {
                return false;
            }
            if (!device->Init(path))
            {
                ERROR("Device.Init failed, device path: %, expected chip: %.", path, chip);
                return false;
            }
            if (device->GetChipID() != chip)
            {
                ERROR("Device chip changed after planning, path: %, expected: %, actual: %.", path, chip,
                      device->GetChipID());
                return false;
            }
            return true;
        });
}

TopoNodeDefinition EnvironmentDefinition()
{
    TopoNodeDefinition definition;
    definition.id = TOPO_NODE(FLOW_CONTROL, "Environment.Load");
    definition.scopeKind = TopoScopeKind::PROFILE;
    definition.mandatory = true;
    definition.creatorFactory = CreateEnvironment;
    return definition;
}

TopoNodeDefinition HostDefinition()
{
    TopoNodeDefinition definition;
    definition.id = TOPO_NODE(HOST_PROCESSING, "HostC");
    definition.scopeKind = TopoScopeKind::HOST;
    definition.mandatory = true;
    definition.processDependencies = TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, "Environment.Load"));
    definition.creatorFactory = CreateHost;
    definition.resources = {"host-state"};
    return definition;
}

TopoNodeDefinition DeviceInitDefinition()
{
    TopoNodeDefinition definition;
    definition.id = TOPO_NODE(DEVICE_PROCESSING, "Device.Init");
    definition.scopeKind = TopoScopeKind::DEVICE;
    definition.mandatory = true;
    definition.processDependencies = TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, "Environment.Load"));
    definition.creatorFactory = CreateDeviceInit;
    definition.opensDeviceScope = true;
    return definition;
}

REGISTER_TOPO_NODE_DEFINITION(EnvironmentDefinition());
REGISTER_TOPO_NODE_DEFINITION(HostDefinition());
REGISTER_TOPO_NODE_DEFINITION(DeviceInitDefinition());

bool BindHost(const PipelineRequest& request, TopoBuildContext& context)
{
    bool reused = false;
    if (!IntermediateReady(request.hostPath, reused))
    {
        return false;
    }
    context.scopePaths["host"] = request.hostPath;
    context.scopeReuse["host"] = reused;
    context.parseRoots.push_back(HostNode("HostC"));
    return true;
}

bool ReadChip(const std::string& path, uint32_t& chip)
{
    const auto files = File::GetOriginData(path, {"info.json"}, {"done"});
    if (files.size() != 1)
    {
        ERROR("Expected one Device info.json in %, found %.", path, files.size());
        return false;
    }
    nlohmann::json info;
    FileReader reader(files[0]);
    if (reader.ReadJson(info) != ANALYSIS_OK || !info.contains("platform_version"))
    {
        ERROR("Cannot read Device platform_version from %.", files[0]);
        return false;
    }
    const auto& value = info.at("platform_version");
    if (StrToU32(chip, value.is_string() ? value.get<std::string>() : value.dump()) != ANALYSIS_OK)
    {
        ERROR("Invalid Device platform_version in %: %.", files[0], value.dump());
        return false;
    }
    return true;
}

bool BindDevice(const std::string& path, const std::string& scope, TopoBuildContext& context)
{
    uint32_t chip = 0;
    if (!ReadChip(path, chip))
    {
        return false;
    }
    bool reused = false;
    if (!IntermediateReady(path, reused))
    {
        return false;
    }
    Bind(context, scope, DeviceContext::Create(chip));
    context.scopeChips[scope] = chip;
    context.scopePaths[scope] = path;
    context.scopeReuse[scope] = reused;
    return true;
}

bool BindDeviceDefinitions(const TopoBuildContext& context, TopoNodeCollection& definitions)
{
    for (const auto& scope : context.scopeChips)
    {
        for (const auto& item : TopoNodeRegistry::GetDefinitions())
        {
            const auto& registered = item.second;
            if (registered.scopeKind != TopoScopeKind::DEVICE || registered.opensDeviceScope ||
                registered.closesDeviceScope || !TopoNodeRegistry::SupportsChip(registered, scope.second))
            {
                continue;
            }
            auto definition = registered;
            definition.id.scope = scope.first;
            if (!definitions.emplace(definition.id, std::move(definition)).second)
            {
                ERROR("Duplicate Device topology definition, scope: %, node: %, chip: %.", scope.first,
                      registered.id.name, scope.second);
                return false;
            }
        }
    }
    return true;
}

bool AddDeviceCompletion(const TopoBuildContext& context, TopoNodeCollection& definitions,
                         const std::vector<TopoNodeId>& selectedRoots, std::vector<TopoNodeId>& done)
{
    RuntimeProcessCollection selected;
    TopoGraphBuilder builder;
    if (!builder.Build(context, selectedRoots, selected, &definitions))
    {
        return false;
    }
    struct DeviceOperations
    {
        std::vector<TopoNodeId> requiredNodes;
        std::vector<TopoNodeId> optionalNodes;
    };
    std::map<std::string, DeviceOperations> operations;
    for (const auto& node : builder.GetRuntimeNodeKeys())
    {
        if (node.first.stage != TopoNodeStage::DEVICE_PROCESSING)
        {
            continue;
        }
        const auto process = selected.find(node.second);
        if (process == selected.end())
        {
            ERROR("Missing selected Device process, scope: %, node: %, key: %.", node.first.scope, node.first.name,
                  node.second);
            return false;
        }
        auto& operation = operations[node.first.scope];
        (process->second.mandatory ? operation.requiredNodes : operation.optionalNodes).push_back(node.first);
    }
    for (auto& scope : operations)
    {
        const auto id = DeviceNode("Device.Ready", scope.first);
        AddCallback(definitions, id, TopoScopeKind::DEVICE, std::move(scope.second.requiredNodes), [] { return true; });
        // Wait for optional work without promoting its failure to a required completion failure.
        auto& ready = definitions.at(id);
        ready.completionDependencies = std::move(scope.second.optionalNodes);
        ready.closesDeviceScope = true;
        done.push_back(id);
    }
    return true;
}

bool PlanPipeline(const PipelineRequest& request, TopoBuildContext& context, TopoNodeCollection& definitions,
                  std::vector<TopoNodeId>& roots)
{
    definitions.clear();
    Bind(context, "profile", std::make_shared<Infra::Context>());
    Bind(context, "host", std::make_shared<Infra::Context>());
    Bind(context, "export", std::make_shared<Infra::Context>());
    if ((request.flags & PIPELINE_CANN_TRACE) != 0)
    {
        if (!File::CheckDir(request.hostPath))
        {
            return false;
        }
        if (!BindHost(request, context))
        {
            return false;
        }
    }
    if ((request.flags & PIPELINE_DEVICE_DATA) != 0)
    {
        const auto directory = request.devicePath.empty() ? request.profPath : File::ParentPath(request.devicePath);
        auto devices = GetDeviceDirectories(directory);
        std::sort(devices.begin(), devices.end());
        if (devices.empty())
        {
            ERROR(
                "No Device directories found for pipeline, search path: %, requested device path: %, profile path: %.",
                directory, request.devicePath, request.profPath);
            return false;
        }
        for (const auto& device : devices)
        {
            const auto scope = File::BaseName(device);
            if (!BindDevice(device, scope, context))
            {
                return false;
            }
        }
    }
    if (!BindDeviceDefinitions(context, definitions))
    {
        return false;
    }
    const bool exporting = (request.flags & (PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY)) != 0;
    std::vector<TopoNodeId> done{Control("Environment.Load")};
    done.insert(done.end(), context.parseRoots.begin(), context.parseRoots.end());
    for (const auto& scope : context.scopeChips)
    {
        if (context.scopeReuse.at(scope.first))
        {
            done.push_back(DeviceNode("Device.Init", scope.first));
            continue;
        }
        if (exporting)
        {
            continue;
        }
        bool supported = false;
        for (const auto& item : definitions)
        {
            if (item.first.scope == scope.first && item.second.scopeKind == TopoScopeKind::DEVICE)
            {
                supported = true;
                done.push_back(item.first);
            }
        }
        if (!supported)
        {
            ERROR("No supported Device operations, scope: %, chip: %, device path: %.", scope.first, scope.second,
                  context.scopePaths.at(scope.first));
            return false;
        }
    }
    if (exporting)
    {
        if (!ValidateExportDatabases(request.profPath))
        {
            return false;
        }
        const auto init = TopoNodeId{TopoNodeStage::FLOW_CONTROL, "Export.Init", "export"};
        const auto output = File::PathJoin({request.profPath, Common::OUTPUT_PATH});
        AddCallback(definitions, init, TopoScopeKind::EXPORT, {Control("Environment.Load")},
                    [output] { return File::CreateDir(output); });
        for (auto& id : roots)
        {
            id.scope = "export";
        }
        // Copy immutable export definitions into this request's overlay; shared processors remain singletons in export
        // scope.
        for (const auto& item : TopoNodeRegistry::GetDefinitions())
        {
            if (item.second.scopeKind != TopoScopeKind::EXPORT)
            {
                continue;
            }
            auto definition = item.second;
            definition.id.scope = "export";
            definition.processDependencies.push_back(init);
            if (definition.id.stage == TopoNodeStage::DATA_PROCESSING)
            {
                definition.processDependencies.insert(definition.processDependencies.end(), context.parseRoots.begin(),
                                                      context.parseRoots.end());
            }
            // DeliverableConsumer serializes writes per output file. Let independent
            // assemblers and savers prepare data concurrently; dependencies order finalization.
            definitions.emplace(definition.id, std::move(definition));
        }
    }
    auto selectedRoots = roots;
    selectedRoots.insert(selectedRoots.end(), done.begin(), done.end());
    if (!AddDeviceCompletion(context, definitions, selectedRoots, done))
    {
        return false;
    }
    if (exporting)
    {
        done.insert(done.end(), roots.begin(), roots.end());
        const auto finalize = TopoNodeId{TopoNodeStage::FLOW_CONTROL, "Export.Finalize", "export"};
        AddCallback(definitions, finalize, TopoScopeKind::EXPORT, done, [] { return true; });
        done = {finalize};
    }
    const auto final = Control("Pipeline.Done");
    // Observe completion after dependency failures; ProcessControl still checks every required node's outcome.
    AddCallback(definitions, final, TopoScopeKind::PROFILE, done, [] { return true; }, true);
    roots.push_back(final);
    return true;
}

}  // namespace

bool PlanUnifiedPipeline(const PipelineRequest& request, TopoBuildContext& context, TopoNodeCollection& definitions,
                         std::vector<TopoNodeId>& roots)
{
    context = TopoBuildContext{};
    definitions.clear();
    roots.clear();
    if (!IsValidPipelineFlags(request.flags))
    {
        ERROR("Invalid pipeline flags (%), flags: %, supported mask: %, profile path: %.",
              request.flags == 0 ? "empty" : "unsupported bits", request.flags, PIPELINE_ALL, request.profPath);
        return false;
    }
    if (!File::CheckDir(request.profPath))
    {
        return false;
    }
    context.profPath = request.profPath;
    context.strictSelection = !request.reportsJson.empty();
    std::set<ExportMode> modes;
    if ((request.flags & PIPELINE_DB) != 0)
    {
        modes.insert(ExportMode::DB);
    }
    if ((request.flags & PIPELINE_TIMELINE) != 0)
    {
        modes.insert(ExportMode::TIMELINE);
    }
    if ((request.flags & PIPELINE_SUMMARY) != 0)
    {
        modes.insert(ExportMode::SUMMARY);
    }
    if (!modes.empty())
    {
        ExportManager manager(request.profPath, request.reportsJson);
        if (!manager.Plan(modes, context, roots))
        {
            return false;
        }
    }
    return PlanPipeline(request, context, definitions, roots);
}

bool TopoGraphBuilder::Build(const PipelineRequest& request, RuntimeProcessCollection& processes)
{
    processes.clear();
    runtimeNodeKeys_.clear();
    try
    {
        TopoBuildContext context;
        TopoNodeCollection definitions;
        std::vector<TopoNodeId> roots;
        return PlanUnifiedPipeline(request, context, definitions, roots) &&
               Build(context, roots, processes, &definitions);
    }
    catch (const std::exception& error)
    {
        ERROR("Build pipeline topology failed, profile path: %, flags: %, reason: %.", request.profPath, request.flags,
              error.what());
        processes.clear();
        runtimeNodeKeys_.clear();
        return false;
    }
}

bool TopoGraphBuilder::Build(const std::string& profPath, const std::set<ExportMode>& modes,
                             const std::string& reportsJson, RuntimeProcessCollection& processes)
{
    processes.clear();
    runtimeNodeKeys_.clear();
    try
    {
        if (modes.empty())
        {
            ERROR("Export modes are empty, profile path: %.", profPath);
            return false;
        }
        ExportManager manager(profPath, reportsJson);
        TopoBuildContext context;
        std::vector<TopoNodeId> roots;
        return manager.Plan(modes, context, roots) && BuildExport(context, roots, processes);
    }
    catch (const std::exception& error)
    {
        ERROR("Build export topology failed, profile path: %, reports path: %, reason: %.", profPath, reportsJson,
              error.what());
        processes.clear();
        runtimeNodeKeys_.clear();
        return false;
    }
}

bool TopoGraphBuilder::BuildExport(const TopoBuildContext& exportContext, const std::vector<TopoNodeId>& exportRoots,
                                   RuntimeProcessCollection& processes)
{
    processes.clear();
    runtimeNodeKeys_.clear();
    try
    {
        PipelineRequest request;
        request.profPath = exportContext.profPath;
        if (!File::CheckDir(request.profPath))
        {
            return false;
        }
        // Empty export roots are valid when an existing DB makes the requested DB export a no-op.
        if (exportRoots.empty())
        {
            if (!ExportManager::HasExportedMsprofDB(request.profPath))
            {
                ERROR("No export roots were selected and no exported DB exists, profile path: %.", request.profPath);
                return false;
            }
            if (!ValidateExportDatabases(request.profPath))
            {
                return false;
            }
            // This entry infers parse flags only to serve export consumers; none remain after DB skip.
            INFO("No export roots remain; skip parsing and export, profile path: %.", request.profPath);
            return true;
        }
        for (const auto& root : exportRoots)
        {
            switch (root.stage)
            {
                case TopoNodeStage::DATABASE_PERSISTENCE:
                    request.flags |= PIPELINE_DB;
                    break;
                case TopoNodeStage::TIMELINE_EXPORT:
                    request.flags |= PIPELINE_TIMELINE;
                    break;
                case TopoNodeStage::SUMMARY_GENERATION:
                    request.flags |= PIPELINE_SUMMARY;
                    break;
                default:
                    break;
            }
        }
        if (request.flags == 0)
        {
            ERROR("The selected roots contain no export target, root count: %, profile path: %.", exportRoots.size(),
                  request.profPath);
            return false;
        }
        request.hostPath = File::PathJoin({request.profPath, "host"});
        if (File::CheckDir(request.hostPath, false))
        {
            request.flags |= PIPELINE_CANN_TRACE;
        }
        if (!GetDeviceDirectories(request.profPath).empty())
        {
            request.flags |= PIPELINE_DEVICE_DATA;
        }
        TopoBuildContext context = exportContext;
        TopoNodeCollection definitions;
        std::vector<TopoNodeId> roots = exportRoots;
        return PlanPipeline(request, context, definitions, roots) && Build(context, roots, processes, &definitions);
    }
    catch (const std::exception& error)
    {
        ERROR("Build export topology failed, profile path: %, reason: %.", exportContext.profPath, error.what());
        processes.clear();
        runtimeNodeKeys_.clear();
        return false;
    }
}

}  // namespace Application
}  // namespace Analysis
