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

#include "analysis/csrc/application/summary/summary_manager.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/summary/summary_factory.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"

namespace Analysis
{
namespace Application
{
namespace
{
const std::vector<std::string> DATA_ASSEMBLE_LIST{PROCESSOR_OP_SUMMARY,          PROCESSOR_NAME_COMM_STATISTIC,
                                                  PROCESSOR_NAME_OP_STATISTIC,   PROCESSOR_NAME_NPU_MEM,
                                                  PROCESSOR_NAME_NPU_MODULE_MEM, PROCESSOR_NAME_API,
                                                  PROCESSOR_NAME_FUSION_OP,      PROCESSOR_TASK_TIME_SUMMARY,
                                                  PROCESSOR_NAME_STEP_TRACE,     PROCESSOR_NAME_PAGE_FAULT};

const std::unordered_map<std::string, std::string> SUMMARY_DELIVERABLES{
    {"op_summary", PROCESSOR_OP_SUMMARY},
    {"comm_statistic", PROCESSOR_NAME_COMM_STATISTIC},
    {"op_statistic", PROCESSOR_NAME_OP_STATISTIC},
    {"npu_memory", PROCESSOR_NAME_NPU_MEM},
    {"npu_module_memory", PROCESSOR_NAME_NPU_MODULE_MEM},
    {"api_statistic", PROCESSOR_NAME_API},
    {"fusion_op", PROCESSOR_NAME_FUSION_OP},
    {"task_time", PROCESSOR_TASK_TIME_SUMMARY},
    {"step_trace", PROCESSOR_NAME_STEP_TRACE},
    {"page_fault", PROCESSOR_NAME_PAGE_FAULT},
};

}  // namespace

bool SummaryManager::IsDeliverableSupported(const std::string& deliverableName)
{
    return SUMMARY_DELIVERABLES.find(deliverableName) != SUMMARY_DELIVERABLES.end();
}

bool SummaryManager::GetTopologyRoots(const std::vector<std::string>& deliverableNames, std::vector<TopoNodeId>& roots)
{
    const size_t rootsSize = roots.size();
    std::vector<std::string> assemblerNames;
    if (!GetAssemblerList(deliverableNames, assemblerNames))
    {
        return false;
    }
    for (const auto& name : assemblerNames)
    {
        const TopoNodeId id{TopoNodeStage::SUMMARY_GENERATION, name};
        if (TopoNodeRegistry::Find(id) == nullptr)
        {
            ERROR("Summary execution list node % has no static topology registration.", name);
            roots.resize(rootsSize);
            return false;
        }
        roots.push_back(id);
    }
    return true;
}

bool SummaryManager::GetAssemblerList(const std::vector<std::string>& deliverableNames,
                                      std::vector<std::string>& assemblerNames)
{
    assemblerNames.clear();
    if (deliverableNames.empty())
    {
        assemblerNames = DATA_ASSEMBLE_LIST;
        return true;
    }

    std::set<std::string> selectedAssemblers;
    for (const auto& deliverableName : deliverableNames)
    {
        const auto deliverableIter = SUMMARY_DELIVERABLES.find(deliverableName);
        if (deliverableIter == SUMMARY_DELIVERABLES.end())
        {
            ERROR("Summary deliverable % is not supported.", deliverableName);
            return false;
        }
        if (selectedAssemblers.insert(deliverableIter->second).second)
        {
            assemblerNames.emplace_back(deliverableIter->second);
        }
    }
    return true;
}

TopoNodeCreatorFactory SummaryManager::CreateSummaryAssembler(const std::string& name)
{
    return [name](const TopoBuildContext& context)
    {
        const std::string profPath = context.profPath;
        return [name, profPath]() -> std::unique_ptr<Infra::Process>
        {
            return std::unique_ptr<Infra::Process>(new (std::nothrow) TopoCallbackProcess(
                [name, profPath](DataInventory& dataInventory) -> bool
                {
                    const auto assembler = SummaryFactory::GetAssemblerByName(name, profPath);
                    if (assembler == nullptr)
                    {
                        ERROR("% is not defined", name);
                        return false;
                    }
                    return assembler->Run(dataInventory);
                }));
        };
    };
}
}  // namespace Application
}  // namespace Analysis
