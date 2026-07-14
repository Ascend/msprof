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

#include <atomic>

#include "analysis/csrc/application/summary/summary_factory.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{
namespace Application
{
namespace
{
using namespace Utils;
const uint8_t PROCESSOR_POOL_NUM = 5;
const std::vector<std::string> DATA_ASSEMBLE_LIST{PROCESSOR_OP_SUMMARY,          PROCESSOR_NAME_COMM_STATISTIC,
                                                  PROCESSOR_NAME_OP_STATISTIC,   PROCESSOR_NAME_NPU_MEM,
                                                  PROCESSOR_NAME_NPU_MODULE_MEM, PROCESSOR_NAME_API,
                                                  PROCESSOR_NAME_FUSION_OP,      PROCESSOR_TASK_TIME_SUMMARY,
                                                  PROCESSOR_NAME_STEP_TRACE,     PROCESSOR_NAME_PAGE_FAULT};

const std::set<std::string> SUMMARY_DATA_PROCESS_LIST{PROCESSOR_NAME_COMMUNICATION,
                                                      PROCESSOR_NAME_COMPUTE_TASK_INFO,
                                                      PROCESSOR_NAME_TASK,
                                                      PROCESSOR_PMU,
                                                      PROCESSOR_NAME_COMM_STATISTIC,
                                                      PROCESSOR_NAME_OP_STATISTIC,
                                                      PROCESSOR_NAME_NPU_MEM,
                                                      PROCESSOR_NAME_NPU_MODULE_MEM,
                                                      PROCESSOR_NAME_API,
                                                      PROCESSOR_NAME_FUSION_OP,
                                                      PROCESSOR_NAME_MODEL_NAME,
                                                      PROCESSOR_NAME_STEP_TRACE,
                                                      PROCESSOR_HOST_TASK,
                                                      PROCESSOR_NAME_PAGE_FAULT};
}  // namespace

bool SummaryManager::Run(DataInventory& dataInventory)
{
    INFO("Start exporting summary!");
    PRINT_INFO("Start exporting the summary!");
    bool runFlag = ProcessSummary(dataInventory);
    if (!runFlag)
    {
        ERROR("The unified summary process failed to be executed.");
        PRINT_ERROR(
            "The unified summary process failed to be executed. "
            "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    PRINT_INFO("End exporting summary output_file. The file is stored in the PROF file.");
    return true;
}

bool SummaryManager::ProcessSummary(Analysis::Infra::DataInventory& dataInventory)
{
    Analysis::Utils::ThreadPool pool(PROCESSOR_POOL_NUM);
    pool.Start();
    std::atomic<bool> retFlag(true);
    for (const auto& name : DATA_ASSEMBLE_LIST)
    {
        pool.AddTask(
            [this, &name, &retFlag, &dataInventory]()
            {
                auto assembler = SummaryFactory::GetAssemblerByName(name, profPath_);
                if (assembler == nullptr)
                {
                    ERROR("% is not defined", name);
                    retFlag = false;
                    return;
                }
                retFlag = assembler->Run(dataInventory) && retFlag;
            });
    }
    pool.WaitAllTasks();
    pool.Stop();
    if (!retFlag)
    {
        ERROR("The % for summary assemble failed to be executed.", profPath_);
        PRINT_ERROR(
            "The % for summary assemble failed to be executed. "
            "Please check msprof_analysis_log in outputPath for more info.",
            profPath_);
        return false;
    }
    return true;
}

const std::set<std::string>& SummaryManager::GetProcessList() { return SUMMARY_DATA_PROCESS_LIST; }
}  // namespace Application
}  // namespace Analysis
