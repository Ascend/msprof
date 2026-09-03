/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include <unordered_map>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/aicpu_summary_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_op_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/hccl_statistic_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/metric_summary.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/model_name_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/op_statistic_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/page_fault_data.h"

namespace Analysis
{
namespace Application
{
namespace
{
using namespace Analysis::Domain;
using StringMap = std::unordered_map<std::string, std::string>;

}  // namespace

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_OP_SUMMARY), true,
                            SummaryManager::CreateSummaryAssembler(PROCESSOR_OP_SUMMARY),
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_PMU)),
                            nullptr);
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_OP_SUMMARY), std::vector<TaskInfoData>,
                                  std::vector<AscendTaskData>, std::vector<CommunicationOpData>, MetricSummary);

#define REGISTER_SIMPLE_SUMMARY_NODE(Name, DataType, Dependency)                             \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(SUMMARY_GENERATION, Name), true,     \
                                SummaryManager::CreateSummaryAssembler(Name),                \
                                TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, Dependency)), nullptr); \
    REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(SUMMARY_GENERATION, Name), DataType)

REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_COMM_STATISTIC, std::vector<HcclStatisticData>,
                             PROCESSOR_NAME_COMM_STATISTIC);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_OP_STATISTIC, std::vector<OpStatisticData>, PROCESSOR_NAME_OP_STATISTIC);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_NPU_MEM, std::vector<NpuMemData>, PROCESSOR_NAME_NPU_MEM);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_NPU_MODULE_MEM, std::vector<NpuModuleMemData>,
                             PROCESSOR_NAME_NPU_MODULE_MEM);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_API, std::vector<ApiData>, PROCESSOR_NAME_API);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_STEP_TRACE, std::vector<TrainTraceData>, PROCESSOR_NAME_STEP_TRACE);
REGISTER_SIMPLE_SUMMARY_NODE(PROCESSOR_NAME_PAGE_FAULT, std::vector<PageFaultData>, PROCESSOR_NAME_PAGE_FAULT);
REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_NAME_AICPU), true,
                            SummaryManager::CreateSummaryAssembler(PROCESSOR_NAME_AICPU),
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_AICPU)), nullptr);
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_NAME_AICPU), std::vector<AicpuSummaryData>,
                                  std::vector<AicpuDpData>, std::vector<AicpuMiData>);

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_NAME_FUSION_OP), true,
                            SummaryManager::CreateSummaryAssembler(PROCESSOR_NAME_FUSION_OP),
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_FUSION_OP),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MODEL_NAME),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)),
                            nullptr);
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_NAME_FUSION_OP), std::vector<FusionOpInfo>,
                                  std::vector<ModelName>, StringMap);

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_TASK_TIME_SUMMARY), true,
                            SummaryManager::CreateSummaryAssembler(PROCESSOR_TASK_TIME_SUMMARY),
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_HOST_TASK),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK)),
                            nullptr);
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(SUMMARY_GENERATION, PROCESSOR_TASK_TIME_SUMMARY), std::vector<TaskInfoData>,
                                  std::vector<HostTask>, std::vector<AscendTaskData>);

#undef REGISTER_SIMPLE_SUMMARY_NODE

}  // namespace Application
}  // namespace Analysis
