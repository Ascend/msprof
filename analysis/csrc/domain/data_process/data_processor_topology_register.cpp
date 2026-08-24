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

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/api_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/dpu_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/fusion_op_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/fusion_task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/hccl_statistic_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/host_task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/kfc_task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/mc2_comm_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/metric_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/model_name_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_host_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/op_statistic_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/overlap_analysis_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/step_trace_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/unified_pmu_processor.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/system/acc_pmu_processor.h"
#include "analysis/csrc/domain/data_process/system/aicore_freq_processor.h"
#include "analysis/csrc/domain/data_process/system/biu_perf_processor.h"
#include "analysis/csrc/domain/data_process/system/block_detail_processor.h"
#include "analysis/csrc/domain/data_process/system/chip_trans_processor.h"
#include "analysis/csrc/domain/data_process/system/ddr_processor.h"
#include "analysis/csrc/domain/data_process/system/hbm_processor.h"
#include "analysis/csrc/domain/data_process/system/hccs_processor.h"
#include "analysis/csrc/domain/data_process/system/host_usage_processor.h"
#include "analysis/csrc/domain/data_process/system/llc_processor.h"
#include "analysis/csrc/domain/data_process/system/low_power_processor.h"
#include "analysis/csrc/domain/data_process/system/netdev_stats_processor.h"
#include "analysis/csrc/domain/data_process/system/npu_mem_processor.h"
#include "analysis/csrc/domain/data_process/system/npu_module_mem_processor.h"
#include "analysis/csrc/domain/data_process/system/npu_op_mem_processor.h"
#include "analysis/csrc/domain/data_process/system/page_fault_processor.h"
#include "analysis/csrc/domain/data_process/system/pcie_processor.h"
#include "analysis/csrc/domain/data_process/system/qos_processor.h"
#include "analysis/csrc/domain/data_process/system/sio_processor.h"
#include "analysis/csrc/domain/data_process/system/soc_bandwidth_processor.h"
#include "analysis/csrc/domain/data_process/system/sys_io_processor.h"
#include "analysis/csrc/domain/data_process/system/ub_processor.h"

namespace Analysis
{
namespace Domain
{
#define REGISTER_PROCESSOR(Processor, Name, Dependencies)                                  \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(Processor), TOPO_NODE(DATA_PROCESSING, Name), true, \
                                CreateDataProcessorFactory<Processor>(Name), Dependencies, nullptr)

#define REGISTER_PROCESSOR_WITH_DATA(Processor, Name, Dependencies, ...) \
    REGISTER_PROCESSOR(Processor, Name, Dependencies);                   \
    REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DATA_PROCESSING, Name), __VA_ARGS__)

#define REGISTER_OPTIONAL_PROCESSOR(Processor, Name, Dependencies)                          \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(Processor), TOPO_NODE(DATA_PROCESSING, Name), false, \
                                CreateDataProcessorFactory<Processor>(Name), Dependencies, nullptr)

REGISTER_PROCESSOR(HashInitProcessor, PROCESSOR_NAME_HASH, TOPO_DEPS());
REGISTER_PROCESSOR(ApiProcessor, PROCESSOR_NAME_API, TOPO_DEPS());
REGISTER_PROCESSOR(DPUProcessor, PROCESSOR_NAME_DPU, TOPO_DEPS());
REGISTER_PROCESSOR_WITH_DATA(CommunicationInfoProcessor, PROCESSOR_NAME_COMMUNICATION,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)), GeHashMap);
REGISTER_PROCESSOR(CCUMissionProcessor, PROCESSOR_NAME_CCU_MISSION, TOPO_DEPS());
REGISTER_PROCESSOR_WITH_DATA(ComputeTaskInfoProcessor, PROCESSOR_NAME_COMPUTE_TASK_INFO,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)), GeHashMap);
REGISTER_PROCESSOR(KfcTaskProcessor, PROCESSOR_NAME_KFC_TASK, TOPO_DEPS());

REGISTER_PROCESSOR(MsprofTxDeviceProcessor, PROCESSOR_NAME_DEVICE_TX, TOPO_DEPS());
REGISTER_PROCESSOR(MsprofTxHostProcessor, PROCESSOR_NAME_MSTX, TOPO_DEPS());
REGISTER_PROCESSOR(StepTraceProcessor, PROCESSOR_NAME_STEP_TRACE, TOPO_DEPS());
REGISTER_PROCESSOR(TaskProcessor, PROCESSOR_NAME_TASK, TOPO_DEPS());
REGISTER_PROCESSOR(AccPmuProcessor, PROCESSOR_NAME_ACC_PMU, TOPO_DEPS());
REGISTER_PROCESSOR(AicoreFreqProcessor, PROCESSOR_NAME_AICORE_FREQ, TOPO_DEPS());
REGISTER_PROCESSOR(ChipTransProcessor, PROCESSOR_NAME_CHIP_TRAINS, TOPO_DEPS());
REGISTER_PROCESSOR(DDRProcessor, PROCESSOR_NAME_DDR, TOPO_DEPS());
REGISTER_PROCESSOR(HBMProcessor, PROCESSOR_NAME_HBM, TOPO_DEPS());
REGISTER_PROCESSOR(HCCSProcessor, PROCESSOR_NAME_HCCS, TOPO_DEPS());
REGISTER_OPTIONAL_PROCESSOR(NetDevStatsProcessor, PROCESSOR_NAME_NETDEV_STATS, TOPO_DEPS());
REGISTER_PROCESSOR(HostCpuUsageProcessor, PROCESSOR_NAME_CPU_USAGE, TOPO_DEPS());
REGISTER_PROCESSOR(HostMemUsageProcessor, PROCESSOR_NAME_MEM_USAGE, TOPO_DEPS());
REGISTER_PROCESSOR(HostDiskUsageProcessor, PROCESSOR_NAME_DISK_USAGE, TOPO_DEPS());
REGISTER_PROCESSOR(HostNetworkUsageProcessor, PROCESSOR_NAME_NETWORK_USAGE, TOPO_DEPS());
REGISTER_PROCESSOR(OSRuntimeApiProcessor, PROCESSOR_NAME_OSRT_API, TOPO_DEPS());
REGISTER_PROCESSOR(LLcProcessor, PROCESSOR_NAME_LLC, TOPO_DEPS());
REGISTER_PROCESSOR(NpuMemProcessor, PROCESSOR_NAME_NPU_MEM, TOPO_DEPS());
REGISTER_PROCESSOR(PCIeProcessor, PROCESSOR_NAME_PCIE, TOPO_DEPS());
REGISTER_PROCESSOR(SioProcessor, PROCESSOR_NAME_SIO, TOPO_DEPS());
REGISTER_PROCESSOR(SocBandwidthProcessor, PROCESSOR_NAME_SOC, TOPO_DEPS());
REGISTER_PROCESSOR(PageFaultProcessor, PROCESSOR_NAME_PAGE_FAULT, TOPO_DEPS());
REGISTER_PROCESSOR(NicTimelineProcessor, PROCESSOR_NAME_NIC_TIMELINE, TOPO_DEPS());
REGISTER_PROCESSOR(RoCETimelineProcessor, PROCESSOR_NAME_ROCE_TIMELINE, TOPO_DEPS());
REGISTER_PROCESSOR(NicProcessor, PROCESSOR_NAME_NIC, TOPO_DEPS());
REGISTER_PROCESSOR(RoCEProcessor, PROCESSOR_NAME_ROCE, TOPO_DEPS());
REGISTER_PROCESSOR(QosProcessor, PROCESSOR_NAME_QOS, TOPO_DEPS());
REGISTER_PROCESSOR(Mc2CommInfoProcessor, PROCESSOR_MC2_COMM_INFO, TOPO_DEPS());
REGISTER_PROCESSOR(MetricProcessor, PROCESSOR_PMU, TOPO_DEPS());
REGISTER_PROCESSOR(MemcpyInfoProcessor, PROCESSOR_NAME_MEMCPY_INFO, TOPO_DEPS());
REGISTER_PROCESSOR_WITH_DATA(NpuOpMemProcessor, PROCESSOR_NAME_NPU_OP_MEM,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)), GeHashMap);
REGISTER_PROCESSOR(NpuModuleMemProcessor, PROCESSOR_NAME_NPU_MODULE_MEM, TOPO_DEPS());
REGISTER_PROCESSOR(UnifiedPmuProcessor, PROCESSOR_NAME_UNIFIED_PMU, TOPO_DEPS());
REGISTER_PROCESSOR(FusionOpProcessor, PROCESSOR_NAME_FUSION_OP, TOPO_DEPS());
REGISTER_PROCESSOR(FusionTaskProcessor, PROCESSOR_NAME_FUSION_TASK, TOPO_DEPS());
REGISTER_PROCESSOR(ModelNameProcessor, PROCESSOR_NAME_MODEL_NAME, TOPO_DEPS());
REGISTER_PROCESSOR(HcclStatisticProcessor, PROCESSOR_NAME_COMM_STATISTIC, TOPO_DEPS());
REGISTER_PROCESSOR(OpStatisticProcessor, PROCESSOR_NAME_OP_STATISTIC, TOPO_DEPS());
REGISTER_PROCESSOR(HostTaskProcessor, PROCESSOR_HOST_TASK, TOPO_DEPS());
REGISTER_PROCESSOR_WITH_DATA(OverlapAnalysisProcessor, PROCESSOR_NAME_OVERLAP_ANALYSIS,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_MC2_COMM_INFO)),
                             std::vector<AscendTaskData>, std::vector<TaskInfoData>, std::vector<CommunicationOpData>,
                             std::vector<MC2CommInfoData>);
REGISTER_PROCESSOR(LowPowerProcessor, PROCESSOR_NAME_LOW_POWER, TOPO_DEPS());
REGISTER_PROCESSOR(BiuPerfProcessor, PROCESSOR_NAME_BIU_PERF, TOPO_DEPS());
REGISTER_PROCESSOR(UbProcessor, PROCESSOR_NAME_UB, TOPO_DEPS());
REGISTER_PROCESSOR(BlockDetailProcessor, PROCESSOR_NAME_BLOCK_DETAIL, TOPO_DEPS());

#undef REGISTER_PROCESSOR_WITH_DATA
#undef REGISTER_OPTIONAL_PROCESSOR
#undef REGISTER_PROCESSOR

}  // namespace Domain
}  // namespace Analysis
