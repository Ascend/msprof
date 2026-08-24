/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/application/timeline/timeline_manager.h"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"

namespace Analysis
{
namespace Application
{
namespace
{
const std::string PREFIX_CONTEXT = "[";
const std::string SUFFIX_CONTEXT = "]";
const int JSON_FILE_OFFSET = -1;
const std::string TIMELINE_PREFIX = "TIMELINE:";
const std::string TIMELINE_PRE_DUMP = TIMELINE_PREFIX + "PRE_DUMP";
const std::string TIMELINE_POST_DUMP = TIMELINE_PREFIX + "POST_DUMP";

const std::unordered_map<JsonProcess, std::string> JSON_TO_ASSEMBLER_TABLE{
    {JsonProcess::ASCEND, PROCESS_TASK},
    {JsonProcess::ACC_PMU, PROCESS_ACC_PMU},
    {JsonProcess::CANN, PROCESS_API},
    {JsonProcess::DDR, PROCESS_DDR},
    {JsonProcess::STARS_CHIP_TRANS, PROCESS_STARS_CHIP_TRANS},
    {JsonProcess::HBM, PROCESS_HBM},
    {JsonProcess::COMMUNICATION, PROCESS_HCCL},
    {JsonProcess::CCU, PROCESS_CCU},
    {JsonProcess::HCCS, PROCESS_HCCS},
    {JsonProcess::OS_RUNTIME_API, PROCESSOR_NAME_OSRT_API},
    {JsonProcess::NETWORK_USAGE, PROCESS_NETWORK_USAGE},
    {JsonProcess::DISK_USAGE, PROCESS_DISK_USAGE},
    {JsonProcess::MEMORY_USAGE, PROCESS_MEMORY_USAGE},
    {JsonProcess::CPU_USAGE, PROCESS_CPU_USAGE},
    {JsonProcess::MSPROFTX, PROCESS_MSPROFTX},
    {JsonProcess::NPU_MEM, PROCESS_NPU_MEM},
    {JsonProcess::OVERLAP_ANALYSE, PROCESS_OVERLAP_ANALYSE},
    {JsonProcess::PCIE, PROCESS_PCIE},
    {JsonProcess::SIO, PROCESS_SIO},
    {JsonProcess::STARS_SOC, PROCESS_STARS_SOC},
    {JsonProcess::STEP_TRACE, PROCESS_STEP_TRACE},
    {JsonProcess::FREQ, PROCESS_LOW_POWER},
    {JsonProcess::LLC, PROCESS_LLC},
    {JsonProcess::NIC, PROCESS_NIC},
    {JsonProcess::ROCE, PROCESS_ROCE},
    {JsonProcess::QOS, PROCESS_QOS},
    {JsonProcess::DEVICE_TX, PROCESS_DEVICE_TX},
    {JsonProcess::LOW_POWER, PROCESS_LOW_POWER},
    {JsonProcess::BIU_PERF, PROCESS_BIU_PERF},
    {JsonProcess::UB, PROCESS_UB},
    {JsonProcess::BLOCK_DETAIL, PROCESS_BLOCK_DETAIL},
    {JsonProcess::DPU, PROCESS_DPU},
    {JsonProcess::FUSION_TASK, PROCESS_FUSION_TASK},
};

}  // namespace

void TimelineManager::WriteFile(const std::string& filePrefix, FileCategory category)
{
    auto tempFile = filePrefix;
    tempFile.append("_").append(GetTimeStampStr()).append(JSON_SUFFIX);
    auto filePath = File::PathJoin({outputPath_, tempFile});
    DumpTool::WriteToFile(filePath, PREFIX_CONTEXT.c_str(), PREFIX_CONTEXT.size(), category);
    fileType_.emplace(category, filePath);
}

bool TimelineManager::PreDumpJson(const std::vector<JsonProcess>& jsonProcess, DataInventory& dataInventory)
{
    WriteFile(MSPROF_JSON_FILE, FileCategory::MSPROF);
    if (std::find(jsonProcess.begin(), jsonProcess.end(), JsonProcess::STEP_TRACE) != jsonProcess.end())
    {
        WriteFile(STEP_TRACE_FILE, FileCategory::STEP);
    }
    if (std::find(jsonProcess.begin(), jsonProcess.end(), JsonProcess::MSPROFTX) != jsonProcess.end() &&
        dataInventory.GetPtr<std::vector<MsprofTxHostData>>() != nullptr)
    {
        WriteFile(MSPROF_TX_FILE, FileCategory::MSPROF_TX);
    }
    return true;
}

void TimelineManager::PostDumpJson()
{
    for (const auto& it : fileType_)
    {
        // 此处需要覆盖文件末尾的","，实测必须使用in、out、ate三种模式打开文件，才可以实现覆盖写入
        FileWriter writer(it.second, std::ios::in | std::ios::out | std::ios::ate);
        if (File::Size(it.second) == PREFIX_CONTEXT.size())
        {
            writer.WriteText(SUFFIX_CONTEXT);
        }
        else
        {
            writer.WriteTextBack(SUFFIX_CONTEXT, JSON_FILE_OFFSET);
        }
    }
}

std::vector<std::string> TimelineManager::GetAssemblerList(const std::vector<JsonProcess>& jsonProcess)
{
    std::vector<std::string> assemblerList;
    std::set<std::string> assemblerSet;
    for (const auto& jsonEnum : jsonProcess)
    {
        const auto& assemblerName = JSON_TO_ASSEMBLER_TABLE.at(jsonEnum);
        if (assemblerSet.insert(assemblerName).second)
        {
            assemblerList.push_back(assemblerName);
        }
    }
    return assemblerList;
}

bool TimelineManager::GetTopologyRoots(const std::vector<JsonProcess>& jsonProcesses, std::vector<TopoNodeId>& roots)
{
    const size_t rootsSize = roots.size();
    for (const auto& name : GetAssemblerList(jsonProcesses))
    {
        const TopoNodeId id{TopoNodeStage::TIMELINE_EXPORT, name};
        if (TopoNodeRegistry::Find(id) == nullptr)
        {
            ERROR("Timeline execution list node % has no static topology registration.", name);
            roots.resize(rootsSize);
            return false;
        }
        roots.push_back(id);
    }
    const TopoNodeId postDump{TopoNodeStage::FLOW_CONTROL, TIMELINE_POST_DUMP};
    if (TopoNodeRegistry::Find(postDump) == nullptr)
    {
        ERROR("Timeline post dump node has no static topology registration.");
        roots.resize(rootsSize);
        return false;
    }
    roots.push_back(postDump);
    return true;
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
    return [](const TopoBuildContext& context) -> Infra::ProcessCreator
    {
        const auto session = context.timelineSession;
        const std::vector<JsonProcess> processes = context.timelineProcesses;
        if (session == nullptr)
        {
            return Infra::ProcessCreator();
        }
        return [session, processes]() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [session, processes](DataInventory& dataInventory) -> bool
                {
                    INFO("Start exporting timeline!");
                    PRINT_INFO("Start exporting the timeline!");
                    return session->PreDumpJson(processes, dataInventory);
                }));
        };
    };
}

TopoNodeCreatorFactory TimelineManager::CreateTimelinePostDump()
{
    return [](const TopoBuildContext& context) -> Infra::ProcessCreator
    {
        const auto session = context.timelineSession;
        if (session == nullptr)
        {
            return Infra::ProcessCreator();
        }
        return [session]() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [session](DataInventory&) -> bool
                {
                    session->PostDumpJson();
                    PRINT_INFO("End exporting timeline output_file. The file is stored in the PROF file.");
                    return true;
                }));
        };
    };
}

std::vector<TopoNodeId> TimelineManager::ResolveSelectedTimelineNodes(const TopoBuildContext&,
                                                                      const std::vector<TopoNodeId>& roots)
{
    std::vector<TopoNodeId> dependencies;
    for (const auto& root : roots)
    {
        if (root.stage == TopoNodeStage::TIMELINE_EXPORT)
        {
            dependencies.push_back(root);
        }
    }
    return dependencies;
}

std::vector<TopoNodeId> TimelineManager::ResolveTimelinePreDumpDependencies(const TopoBuildContext& context,
                                                                            const std::vector<TopoNodeId>&)
{
    std::vector<TopoNodeId> dependencies;
    if (std::find(context.timelineProcesses.begin(), context.timelineProcesses.end(), JsonProcess::MSPROFTX) !=
        context.timelineProcesses.end())
    {
        dependencies.push_back(TopoNodeId{TopoNodeStage::DATA_PROCESSING, PROCESSOR_NAME_MSTX});
    }
    return dependencies;
}
}  // namespace Application
}  // namespace Analysis
