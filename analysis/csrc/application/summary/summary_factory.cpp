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

#include "analysis/csrc/application/summary/summary_factory.h"

#include "analysis/csrc/application/summary/api_statistic_assembler.h"
#include "analysis/csrc/application/summary/comm_statistic_assembler.h"
#include "analysis/csrc/application/summary/fusion_op_assembler.h"
#include "analysis/csrc/application/summary/npu_memory_assembler.h"
#include "analysis/csrc/application/summary/npu_module_mem_assembler.h"
#include "analysis/csrc/application/summary/op_statistic_assembler.h"
#include "analysis/csrc/application/summary/op_summary_assembler.h"
#include "analysis/csrc/application/summary/page_fault_assembler.h"
#include "analysis/csrc/application/summary/summary_step_trace_assembler.h"
#include "analysis/csrc/application/summary/task_time_assembler.h"

namespace Analysis
{
namespace Application
{
using namespace Utils;
std::unordered_map<std::string, AssemblerCreator> SummaryFactory::assemblerTable_{
    {PROCESSOR_OP_SUMMARY, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, OpSummaryAssembler, PROCESSOR_OP_SUMMARY, profPath); }},
    {PROCESSOR_NAME_NPU_MEM, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, NpuMemoryAssembler, PROCESSOR_NAME_NPU_MEM, profPath); }},
    {PROCESSOR_NAME_NPU_MODULE_MEM, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, NpuModuleMemAssembler, PROCESSOR_NAME_NPU_MODULE_MEM, profPath); }},
    {PROCESSOR_NAME_API, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, ApiStatisticAssembler, PROCESSOR_NAME_API, profPath); }},
    {PROCESSOR_NAME_FUSION_OP, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, FusionOpAssembler, PROCESSOR_NAME_FUSION_OP, profPath); }},
    {PROCESSOR_TASK_TIME_SUMMARY, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, TaskTimeAssembler, PROCESSOR_TASK_TIME_SUMMARY, profPath); }},
    {PROCESSOR_NAME_STEP_TRACE, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, SummaryStepTraceAssembler, PROCESSOR_NAME_STEP_TRACE, profPath); }},
    {PROCESSOR_NAME_COMM_STATISTIC, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, CommStatisticAssembler, PROCESSOR_NAME_COMM_STATISTIC, profPath); }},
    {PROCESSOR_NAME_OP_STATISTIC, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, OpStatisticAssembler, PROCESSOR_NAME_OP_STATISTIC, profPath); }},
    {PROCESSOR_NAME_PAGE_FAULT, [](const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)
     { MAKE_SHARED0_NO_OPERATION(assembler, PageFaultAssembler, PROCESSOR_NAME_PAGE_FAULT, profPath); }},
};

std::shared_ptr<SummaryAssembler> SummaryFactory::GetAssemblerByName(const std::string& processName,
                                                                     const std::string& profPath)
{
    std::shared_ptr<SummaryAssembler> assembler{nullptr};
    auto it = assemblerTable_.find(processName);
    if (it != assemblerTable_.end())
    {
        it->second(profPath, assembler);
    }
    return assembler;
}
}  // namespace Application
}  // namespace Analysis
