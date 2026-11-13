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

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/aicpu_processor.h"
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
#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/metric_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/model_name_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_host_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/op_statistic_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/overlap_analysis_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/step_trace_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/task_association_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/unified_pmu_processor.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/system/acc_pmu_processor.h"
#include "analysis/csrc/domain/data_process/system/aicore_freq_processor.h"
#include "analysis/csrc/domain/data_process/system/biu_perf_processor.h"
#include "analysis/csrc/domain/data_process/system/block_detail_processor.h"
#include "analysis/csrc/domain/data_process/system/chip_trans_processor.h"
#include "analysis/csrc/domain/data_process/system/ddr_processor.h"
#include "analysis/csrc/domain/data_process/system/ftrace_generic_event_processor.h"
#include "analysis/csrc/domain/data_process/system/hbm_processor.h"
#include "analysis/csrc/domain/data_process/system/hccs_processor.h"
#include "analysis/csrc/domain/data_process/system/host_platform_processor.h"
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
#define REGISTER_PROCESSOR(Processor, Name, Mandatory, Dependencies)                            \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(Processor), TOPO_NODE(DATA_PROCESSING, Name), Mandatory, \
                                CreateDataProcessorFactory<Processor>(Name), Dependencies, nullptr)

#define REGISTER_PROCESSOR_WITH_DATA(Processor, Name, Mandatory, Dependencies, ...) \
    REGISTER_PROCESSOR(Processor, Name, Mandatory, Dependencies);                   \
    REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DATA_PROCESSING, Name), __VA_ARGS__)

REGISTER_PROCESSOR(HashInitProcessor, PROCESSOR_NAME_HASH, true, TOPO_DEPS());

REGISTER_PROCESSOR(ApiProcessor, PROCESSOR_NAME_API, true, TOPO_DEPS());

REGISTER_PROCESSOR(DPUProcessor, PROCESSOR_NAME_DPU, true, TOPO_DEPS());

REGISTER_PROCESSOR_WITH_DATA(CommunicationInfoProcessor, PROCESSOR_NAME_COMMUNICATION, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH),
                                       TOPO_NODE(DEVICE_PERSISTENCE, "DeviceHcclPersistence")),
                             GeHashMap);

REGISTER_PROCESSOR(CCUMissionProcessor, PROCESSOR_NAME_CCU_MISSION, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "CcuPersistence")));

REGISTER_PROCESSOR_WITH_DATA(ComputeTaskInfoProcessor, PROCESSOR_NAME_COMPUTE_TASK_INFO, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)), GeHashMap);

REGISTER_PROCESSOR(KfcTaskProcessor, PROCESSOR_NAME_KFC_TASK, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "AicpuPersistence")));

REGISTER_PROCESSOR(MsprofTxDeviceProcessor, PROCESSOR_NAME_DEVICE_TX, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "TsTrackPersistence")));

REGISTER_PROCESSOR(MsprofTxHostProcessor, PROCESSOR_NAME_MSTX, true, TOPO_DEPS());

REGISTER_PROCESSOR(StepTraceProcessor, PROCESSOR_NAME_STEP_TRACE, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "TracePersistence")));

REGISTER_PROCESSOR(TaskProcessor, PROCESSOR_NAME_TASK, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "AscendTaskPersistence")));

REGISTER_PROCESSOR(AccPmuProcessor, PROCESSOR_NAME_ACC_PMU, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "AccPmuPersistence")));

REGISTER_PROCESSOR(AicoreFreqProcessor, PROCESSOR_NAME_AICORE_FREQ, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "FreqPersistence"),
                             TOPO_NODE(DEVICE_PERSISTENCE, "LpmInfoPersistence")));

REGISTER_PROCESSOR(ChipTransProcessor, PROCESSOR_NAME_CHIP_TRAINS, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "ChipTransPersistence")));

REGISTER_PROCESSOR(DDRProcessor, PROCESSOR_NAME_DDR, true, TOPO_DEPS());

REGISTER_PROCESSOR(HBMProcessor, PROCESSOR_NAME_HBM, true, TOPO_DEPS());

REGISTER_PROCESSOR(HCCSProcessor, PROCESSOR_NAME_HCCS, true, TOPO_DEPS());

REGISTER_PROCESSOR(NetDevStatsProcessor, PROCESSOR_NAME_NETDEV_STATS, false, TOPO_DEPS());

REGISTER_PROCESSOR(HostCpuUsageProcessor, PROCESSOR_NAME_CPU_USAGE, true, TOPO_DEPS());

REGISTER_PROCESSOR(HostCpuFreqProcessor, PROCESSOR_NAME_CPU_FREQ, true, TOPO_DEPS());

REGISTER_PROCESSOR(HostMemUsageProcessor, PROCESSOR_NAME_MEM_USAGE, true, TOPO_DEPS());

REGISTER_PROCESSOR(HostDiskUsageProcessor, PROCESSOR_NAME_DISK_USAGE, true, TOPO_DEPS());

REGISTER_PROCESSOR(HostNetworkUsageProcessor, PROCESSOR_NAME_NETWORK_USAGE, true, TOPO_DEPS());

REGISTER_PROCESSOR(OSRuntimeApiProcessor, PROCESSOR_NAME_OSRT_API, true, TOPO_DEPS());

REGISTER_PROCESSOR_WITH_DATA(HostPlatformProcessor, PROCESSOR_NAME_HOST_PLATFORM, true, TOPO_DEPS(),
                             std::vector<NumaLevelsHierarchyData>, std::vector<NumaMetricsData>,
                             std::vector<NumaScalingValuesData>, std::vector<NumaTitlesNamesData>,
                             std::vector<HostCoreThreadData>, std::vector<HostCoreProcessData>,
                             std::vector<HostCoreMetricDescData>, std::vector<HostCoreMetricData>);

REGISTER_PROCESSOR(LLcProcessor, PROCESSOR_NAME_LLC, true, TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "LlcPersistence")));

REGISTER_PROCESSOR(NpuMemProcessor, PROCESSOR_NAME_NPU_MEM, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "NpuMemPersistence")));

REGISTER_PROCESSOR(PCIeProcessor, PROCESSOR_NAME_PCIE, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "PciePersistence")));

REGISTER_PROCESSOR(SioProcessor, PROCESSOR_NAME_SIO, true, TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "SioPersistence")));

REGISTER_PROCESSOR(SocBandwidthProcessor, PROCESSOR_NAME_SOC, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "InterSocPersistence")));

REGISTER_PROCESSOR(PageFaultProcessor, PROCESSOR_NAME_PAGE_FAULT, true, TOPO_DEPS());

REGISTER_PROCESSOR_WITH_DATA(AicpuProcessor, PROCESSOR_NAME_AICPU, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                       TOPO_NODE(DEVICE_PERSISTENCE, "AicpuPersistence")),
                             std::vector<AscendTaskData>, std::vector<TaskInfoData>);

REGISTER_PROCESSOR(NicTimelineProcessor, PROCESSOR_NAME_NIC_TIMELINE, true, TOPO_DEPS());

REGISTER_PROCESSOR(RoCETimelineProcessor, PROCESSOR_NAME_ROCE_TIMELINE, true, TOPO_DEPS());

REGISTER_PROCESSOR(NicProcessor, PROCESSOR_NAME_NIC, true, TOPO_DEPS());

REGISTER_PROCESSOR(RoCEProcessor, PROCESSOR_NAME_ROCE, true, TOPO_DEPS());

REGISTER_PROCESSOR(QosProcessor, PROCESSOR_NAME_QOS, true, TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "QosPersistence")));

REGISTER_PROCESSOR(MetricProcessor, PROCESSOR_PMU, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "MetricSummaryPersistence")));

REGISTER_PROCESSOR(MemcpyInfoProcessor, PROCESSOR_NAME_MEMCPY_INFO, true, TOPO_DEPS());
REGISTER_PROCESSOR(FtraceGenericEventProcessor, PROCESSOR_NAME_FTRACE_GENERIC_EVENT, true, TOPO_DEPS());

REGISTER_PROCESSOR_WITH_DATA(NpuOpMemProcessor, PROCESSOR_NAME_NPU_OP_MEM, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HASH)), GeHashMap);

REGISTER_PROCESSOR(NpuModuleMemProcessor, PROCESSOR_NAME_NPU_MODULE_MEM, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "NpuModuleMemPersistence")));

REGISTER_PROCESSOR(UnifiedPmuProcessor, PROCESSOR_NAME_UNIFIED_PMU, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "MetricSummaryPersistence")));

REGISTER_PROCESSOR(FusionOpProcessor, PROCESSOR_NAME_FUSION_OP, true, TOPO_DEPS());

REGISTER_PROCESSOR(FusionTaskProcessor, PROCESSOR_NAME_FUSION_TASK, true, TOPO_DEPS());

REGISTER_PROCESSOR(ModelNameProcessor, PROCESSOR_NAME_MODEL_NAME, true, TOPO_DEPS());

REGISTER_PROCESSOR(HcclStatisticProcessor, PROCESSOR_NAME_COMM_STATISTIC, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "DeviceHcclPersistence")));

REGISTER_PROCESSOR_WITH_DATA(TaskAssociationProcessor, PROCESSOR_NAME_TASK_ASSOCIATION, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK)),
                             std::vector<TaskInfoData>, std::vector<AscendTaskData>);

REGISTER_PROCESSOR_WITH_DATA(OpStatisticProcessor, PROCESSOR_NAME_OP_STATISTIC, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK_ASSOCIATION)),
                             AssociatedTaskCollection);

REGISTER_PROCESSOR(HostTaskProcessor, PROCESSOR_HOST_TASK, true, TOPO_DEPS());

REGISTER_PROCESSOR_WITH_DATA(OverlapAnalysisProcessor, PROCESSOR_NAME_OVERLAP_ANALYSIS, true,
                             TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                       TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION)),
                             std::vector<AscendTaskData>, std::vector<TaskInfoData>, std::vector<CommunicationOpData>);

REGISTER_PROCESSOR(LowPowerProcessor, PROCESSOR_NAME_LOW_POWER, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "LowPowerPersistence")));

REGISTER_PROCESSOR(BiuPerfProcessor, PROCESSOR_NAME_BIU_PERF, true, TOPO_DEPS());

REGISTER_PROCESSOR(UbProcessor, PROCESSOR_NAME_UB, true, TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "UbPersistence")));

REGISTER_PROCESSOR(BlockDetailProcessor, PROCESSOR_NAME_BLOCK_DETAIL, true,
                   TOPO_DEPS(TOPO_NODE(DEVICE_PERSISTENCE, "MetricSummaryPersistence")));

#undef REGISTER_PROCESSOR_WITH_DATA
#undef REGISTER_PROCESSOR
}  // namespace Domain
}  // namespace Analysis
