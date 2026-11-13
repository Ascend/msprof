/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_GRAPH_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_GRAPH_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analysis/csrc/application/include/export_mode_enum.h"
#include "analysis/csrc/application/timeline/json_process_enum.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_struct.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Application
{
class DBAssembler;
class TimelineManager;

enum class TopoNodeStage : uint8_t
{
    DATA_PROCESSING,       // Domain data processing.
    SUMMARY_GENERATION,    // Application summary generation.
    DATABASE_PERSISTENCE,  // Application database persistence.
    TIMELINE_EXPORT,       // Application timeline export.
    FLOW_CONTROL,          // Topology lifecycle control.
    HOST_PROCESSING,
    DEVICE_PROCESSING,
    DEVICE_PERSISTENCE,  // Device intermediate SQLite persistence.
};

enum class TopoScopeKind
{
    PROFILE,
    HOST,
    DEVICE,
    EXPORT
};

struct TopoNodeId
{
    TopoNodeStage stage;
    std::string name;
    std::string scope;

    bool operator==(const TopoNodeId& other) const
    {
        return stage == other.stage && name == other.name && scope == other.scope;
    }
};

struct TopoNodeIdHash
{
    size_t operator()(const TopoNodeId& id) const
    {
        return std::hash<uint8_t>()(static_cast<uint8_t>(id.stage)) ^ (std::hash<std::string>()(id.name) << 1U) ^
               (std::hash<std::string>()(id.scope) << 2U);
    }
};

struct TopoBuildContext
{
    std::string profPath;
    std::string outputPath;
    std::vector<JsonProcess> timelineProcesses;
    std::shared_ptr<TimelineManager> timelineSession;
    std::shared_ptr<DBAssembler> dbSession;
    std::string currentScope;
    // Init nodes may initialize these owned contexts before business processes receive const bindings.
    std::unordered_map<std::string, std::shared_ptr<Infra::Context>> scopeContexts;
    std::unordered_map<std::string, std::shared_ptr<Infra::DataInventory>> scopeInventories;
    std::unordered_map<std::string, uint32_t> scopeChips;
    std::unordered_map<std::string, std::string> scopePaths;
    std::unordered_map<std::string, bool> scopeReuse;
    std::vector<TopoNodeId> parseRoots;
    // Export selection is separate from the final pipeline entry roots; empty keeps direct Build callers' roots.
    std::vector<TopoNodeId> selectedExportRoots;
    bool strictSelection = false;
    // The export entry may have already loaded the environment before preparing its sessions.
    bool environmentLoaded = false;
};

using TopoNodeCreatorFactory = std::function<Infra::ProcessCreator(const TopoBuildContext&)>;
using TopoDependencyResolver =
    std::function<std::vector<TopoNodeId>(const TopoBuildContext&, const std::vector<TopoNodeId>&)>;

struct TopoNodeDefinition
{
    TopoNodeId id;
    // Unscoped Device dependencies on non-Device nodes expand per requested device and chip.
    std::vector<TopoNodeId> processDependencies;
    std::vector<TopoNodeId> completionDependencies;
    std::vector<std::type_index> inputDataTypes;
    bool mandatory{false};
    std::vector<uint32_t> chipIds;
    TopoNodeCreatorFactory creatorFactory;
    TopoDependencyResolver dependencyResolver;
    TopoDependencyResolver completionDependencyResolver;
    std::type_index runtimeType{typeid(void)};
    TopoScopeKind scopeKind = TopoScopeKind::EXPORT;
    std::vector<std::string> resources;
    bool opensDeviceScope = false;
    bool runOnDependencyFailure = false;
};

using TopoNodeCollection = std::unordered_map<TopoNodeId, TopoNodeDefinition, TopoNodeIdHash>;

class TopoNodeRegistry
{
   public:
    static bool Register(TopoNodeDefinition definition);
    static const TopoNodeCollection& GetDefinitions();
    static const TopoNodeDefinition* Find(const TopoNodeId& id);
    static const TopoNodeDefinition* FindProcessorByName(const std::string& name);
    static const TopoNodeDefinition* FindProcessorByRuntimeType(std::type_index runtimeType);
    static bool SupportsChip(const TopoNodeDefinition& definition, uint32_t chip);
    static TopoNodeCollection& MutableDefinitions();
};

class TopoNodeSequenceRegister
{
   public:
    explicit TopoNodeSequenceRegister(TopoNodeDefinition definition);
    TopoNodeSequenceRegister(TopoNodeId id, std::type_index runtimeType, bool mandatory,
                             std::vector<TopoNodeId> dependencies, TopoNodeCreatorFactory creatorFactory,
                             TopoDependencyResolver dependencyResolver = nullptr, bool runOnDependencyFailure = false);
};

class TopoNodeDataRegister
{
   public:
    TopoNodeDataRegister(TopoNodeId id, std::vector<std::type_index> inputDataTypes);
};

template <typename... T>
struct TopoNodeTypeIndexList
{
    TopoNodeTypeIndexList() : types{typeid(T)...} {}
    std::vector<std::type_index> types;
};

struct PipelineRequest;

class TopoGraphBuilder
{
   public:
    // Extends the caller's selected export roots with Host/Device parsing and lifecycle nodes.
    bool BuildExport(const TopoBuildContext& exportContext, const std::vector<TopoNodeId>& exportRoots,
                     Infra::RuntimeProcessCollection& processes);
    // Discovers Host/Device scopes and builds the full graph required by these export modes.
    bool Build(const std::string& profPath, const std::set<ExportMode>& modes, const std::string& reportsJson,
               Infra::RuntimeProcessCollection& processes);
    // Builds the complete request, including Host/Device parsing and selected exports, without executing nodes.
    bool Build(const PipelineRequest& request, Infra::RuntimeProcessCollection& processes);
    // Compiles caller-owned bindings and roots in one dependency expansion.
    bool Build(const TopoBuildContext& context, const std::vector<TopoNodeId>& roots,
               Infra::RuntimeProcessCollection& processes, const TopoNodeCollection* overlay = nullptr);
    const std::unordered_map<TopoNodeId, Infra::RuntimeProcessKey, TopoNodeIdHash>& GetRuntimeNodeKeys() const
    {
        return runtimeNodeKeys_;
    }

   private:
    std::unordered_map<TopoNodeId, Infra::RuntimeProcessKey, TopoNodeIdHash> runtimeNodeKeys_;
};
}  // namespace Application
}  // namespace Analysis

/**
 *
 * TOPO_NODE(stage, name)
 * @param stage 节点所属业务阶段，取值为 TopoNodeStage 枚举值。
 *              例如 DATA_PROCESSING、SUMMARY_GENERATION、
 *              DATABASE_PERSISTENCE、TIMELINE_EXPORT、FLOW_CONTROL。
 * @param name  阶段内稳定且唯一的业务节点名称。相同 stage 与 name
 *              共同组成 TopoNodeId，必须与依赖节点及数据注册中的节点标识保持一致。
 *
 * TOPO_DEPS(...)
 * @param ...   零个或多个由 TOPO_NODE(...) 构造的前置依赖节点。
 *              调度器会先执行全部依赖节点，再执行当前节点；依赖节点必须已注册，
 *              且依赖关系不能形成环。无依赖时使用 TOPO_DEPS()。
 *
 * REGISTER_TOPO_NODE_SEQUENCE(RuntimeType, NodeId, Mandatory, CreatorFactory,
 *                             Dependencies, Resolver[, RunOnDependencyFailure])
 * @param RuntimeType    DATA_PROCESSING 阶段节点对应具体处理器类型，
 *                       使用 typeid(ProcessorType)，供处理器工厂查找；其余阶段使用 typeid(void)。
 *                       所有节点实例均由拓扑构建器分配运行时 key。
 * @param NodeId         当前节点标识，使用 TOPO_NODE(stage, name) 构造。
 * @param Mandatory      是否为必需节点。true 表示节点执行失败会导致本次流程失败；
 *                       false 表示该节点失败后流程可继续执行。
 * @param CreatorFactory 节点创建工厂，接收 TopoBuildContext 并返回 ProcessCreator。
 *                       工厂或其创建的 Process 为空时，节点构建失败。
 * @param Dependencies   静态依赖列表，使用 TOPO_DEPS(...) 构造。
 * @param Resolver       可选动态依赖解析器；根据 TopoBuildContext 和根节点列表
 *                       补充依赖。没有动态依赖时传入 nullptr。
 * @param RunOnDependencyFailure 可省略，默认 false。显式传 true 时，等待全部前驱结束后执行，
 *                       不因前驱失败而阻塞；不覆盖 DFX 或 fail-fast 的全局停止。
 *
 * REGISTER_TOPO_NODE_DEPENDENT_DATA(NodeId, ...)
 * @param NodeId 当前节点标识，必须与 REGISTER_TOPO_NODE_SEQUENCE 的 NodeId 完全一致。
 * @param ...    节点从 DataInventory 读取的输入数据类型，例如
 *               std::vector<TaskInfoData>。类型必须与生产节点写入的类型一致。
 *               该宏必须定义在同一节点的 REGISTER_TOPO_NODE_SEQUENCE 之后。
 *
 * 示例：注册一个时间线导出节点。它依赖时间线预处理和任务数据处理，
 * 并声明运行时从 DataInventory 读取的输入类型。
 *
 * REGISTER_TOPO_NODE_SEQUENCE(
 *     typeid(void),
 *     TOPO_NODE(TIMELINE_EXPORT, PROCESS_TASK),
 *     true,
 *     TimelineManager::CreateTimelineAssembler(PROCESS_TASK),
 *     TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
 *               TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK)),
 *     nullptr);
 *
 * REGISTER_TOPO_NODE_DEPENDENT_DATA(
 *     TOPO_NODE(TIMELINE_EXPORT, PROCESS_TASK),
 *     std::vector<TaskInfoData>,
 *     std::vector<AscendTaskData>);
 */

#define TOPO_NODE_REGISTER_CONCAT_INNER(left, right) left##right
#define TOPO_NODE_REGISTER_CONCAT(left, right) TOPO_NODE_REGISTER_CONCAT_INNER(left, right)

#define TOPO_NODE(stage, name) (Analysis::Application::TopoNodeId{Analysis::Application::TopoNodeStage::stage, name})
#define TOPO_DEPS(...) (std::vector<Analysis::Application::TopoNodeId>{__VA_ARGS__})

// Registers a full definition, including scope, resources and device slot lifetime.
// The definition must contain only request-independent metadata; factories bind request state at Build time.
#define REGISTER_TOPO_NODE_DEFINITION(Definition) REGISTER_TOPO_NODE_DEFINITION_IMPL(Definition, __LINE__)
#define REGISTER_TOPO_NODE_DEFINITION_IMPL(Definition, line)                                                     \
    static Analysis::Application::TopoNodeSequenceRegister TOPO_NODE_REGISTER_CONCAT(topoNodeDefinitionRegister, \
                                                                                     line)(Definition)

#define REGISTER_TOPO_NODE_SEQUENCE(RuntimeType, NodeId, Mandatory, CreatorFactory, Dependencies, ...)       \
    REGISTER_TOPO_NODE_SEQUENCE_IMPL(RuntimeType, NodeId, Mandatory, CreatorFactory, Dependencies, __LINE__, \
                                     __VA_ARGS__)

#define REGISTER_TOPO_NODE_SEQUENCE_IMPL(RuntimeType, NodeId, Mandatory, CreatorFactory, Dependencies, line, ...)     \
    static Analysis::Application::TopoNodeSequenceRegister TOPO_NODE_REGISTER_CONCAT(topoNodeSequenceRegister, line)( \
        NodeId, RuntimeType, Mandatory, Dependencies, CreatorFactory, __VA_ARGS__)

#define REGISTER_TOPO_NODE_DEPENDENT_DATA(NodeId, ...) \
    REGISTER_TOPO_NODE_DEPENDENT_DATA_IMPL(NodeId, __LINE__, __VA_ARGS__)

#define REGISTER_TOPO_NODE_DEPENDENT_DATA_IMPL(NodeId, line, ...)                                                  \
    static Analysis::Application::TopoNodeTypeIndexList<__VA_ARGS__> TOPO_NODE_REGISTER_CONCAT(topoNodeDataHelper, \
                                                                                               line);              \
    static Analysis::Application::TopoNodeDataRegister TOPO_NODE_REGISTER_CONCAT(topoNodeDataRegister, line)(      \
        NodeId, std::move(TOPO_NODE_REGISTER_CONCAT(topoNodeDataHelper, line).types))

#endif  // ANALYSIS_INFRASTRUCTURE_PROCESS_TOPO_GRAPH_H
