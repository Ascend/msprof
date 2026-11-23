/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/application/timeline/timeline_manager.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/include/finish_done_writer.h"
#include "analysis/csrc/application/include/report_map.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"

namespace Analysis
{
namespace Application
{
bool TimelineManager::PreDumpJson()
{
    // File names and the opening bracket are decided when the first event arrives.
    DeliverableConsumer::Instance().BeginTimeline();
    return true;
}

bool TimelineManager::PostDumpJson()
{
    for (const auto& file : DeliverableConsumer::Instance().FinishTimeline())
    {
        if (!FinishDoneWriter::Append(file))
        {
            WARN("Report the timeline deliverable failed, file: %.", file);
        }
    }
    return DeliverableConsumer::Instance().Ok();
}

void TimelineManager::GetTopologyRoots(const std::vector<std::string>& jsonKeys, std::vector<TopoNodeId>& roots)
{
    for (const auto& jsonKey : jsonKeys)
    {
        const std::string nodeKey = GetTimelineNodeKey(jsonKey);
        if (nodeKey.empty())
        {
            ERROR("Timeline report key % is not supported, skip it.", jsonKey);
            continue;
        }
        const TopoNodeId id{TopoNodeStage::TIMELINE_EXPORT, nodeKey};
        if (TopoNodeRegistry::Find(id) == nullptr)
        {
            ERROR("Timeline execution list node % has no static topology registration, skip it.", nodeKey);
            continue;
        }
        roots.push_back(id);
    }
    const TopoNodeId postDump{TopoNodeStage::FLOW_CONTROL, TIMELINE_POST_DUMP};
    roots.push_back(postDump);
}

TopoNodeCreatorFactory TimelineManager::CreateTimelineAssembler(const std::string& name)
{
    return [name](const TopoBuildContext& context)
    {
        const std::string profPath = context.profPath;
        return [name, profPath]() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [name, profPath](DataInventory& dataInventory) -> bool
                {
                    const auto assembler = TimelineFactory::GetAssemblerByName(name);
                    if (assembler == nullptr)
                    {
                        ERROR("% is not defined", name);
                        return false;
                    }
                    return assembler->Run(dataInventory, profPath);
                }));
        };
    };
}

TopoNodeCreatorFactory TimelineManager::CreateTimelinePreDump()
{
    return [](const TopoBuildContext&) -> Infra::ProcessCreator
    {
        return []() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [](DataInventory&) -> bool
                {
                    INFO("Start exporting timeline!");
                    PRINT_INFO("Start exporting the timeline!");
                    return TimelineManager::PreDumpJson();
                }));
        };
    };
}

TopoNodeCreatorFactory TimelineManager::CreateTimelinePostDump()
{
    return [](const TopoBuildContext&) -> Infra::ProcessCreator
    {
        return []() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [](DataInventory&) -> bool
                {
                    if (!TimelineManager::PostDumpJson())
                    {
                        ERROR("Export timeline deliverable failed.");
                        return false;
                    }
                    PRINT_INFO("End exporting timeline output_file. The file is stored in the PROF file.");
                    return true;
                }));
        };
    };
}

std::vector<TopoNodeId> TimelineManager::ResolveSelectedTimelineNodes(const TopoBuildContext& context,
                                                                      const std::vector<TopoNodeId>& roots)
{
    std::vector<TopoNodeId> dependencies;
    const auto& selectedRoots = context.selectedExportRoots.empty() ? roots : context.selectedExportRoots;
    for (const auto& root : selectedRoots)
    {
        if (root.stage == TopoNodeStage::TIMELINE_EXPORT)
        {
            dependencies.push_back(root);
        }
    }
    return dependencies;
}

std::vector<TopoNodeId> TimelineManager::ResolveTimelinePreDumpDependencies(const TopoBuildContext& context,
                                                                            const std::vector<TopoNodeId>& roots)
{
    std::vector<TopoNodeId> dependencies;
    const auto& selectedRoots = context.selectedExportRoots.empty() ? roots : context.selectedExportRoots;
    // Select roots carry their own scope, so only stage and name are meaningful here.
    for (const auto& root : selectedRoots)
    {
        if (root.stage == TopoNodeStage::TIMELINE_EXPORT && root.name == PROCESS_MSPROFTX)
        {
            dependencies.push_back(TopoNodeId{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_MSTX});
            break;
        }
    }
    return dependencies;
}
}  // namespace Application
}  // namespace Analysis
