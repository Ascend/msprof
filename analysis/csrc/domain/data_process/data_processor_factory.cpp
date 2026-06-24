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

#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"

#include "analysis/csrc/domain/data_process/ai_task/api_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/dpu_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/fusion_op_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/fusion_task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/hccl_statistic_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/host_task_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/kfc_comm_processor.h"
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
#include "analysis/csrc/domain/data_process/system/pcie_processor.h"
#include "analysis/csrc/domain/data_process/system/qos_processor.h"
#include "analysis/csrc/domain/data_process/system/sio_processor.h"
#include "analysis/csrc/domain/data_process/system/soc_bandwidth_processor.h"
#include "analysis/csrc/domain/data_process/system/sys_io_processor.h"
#include "analysis/csrc/domain/data_process/system/ub_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis
{
namespace Domain
{
std::unordered_map<std::string, ProcessorCreator> DataProcessorFactory::processorTable_{
    {PROCESSOR_NAME_API, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, ApiProcessor, profPath); }},
    {PROCESSOR_NAME_DPU, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, DPUProcessor, profPath); }},
    {PROCESSOR_NAME_COMMUNICATION, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, CommunicationInfoProcessor, profPath); }},
    {PROCESSOR_NAME_CCU_MISSION, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, CCUMissionProcessor, profPath); }},
    {PROCESSOR_NAME_COMPUTE_TASK_INFO, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, ComputeTaskInfoProcessor, profPath); }},
    {PROCESSOR_NAME_KFC_TASK, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, KfcTaskProcessor, profPath); }},
    {PROCESSOR_NAME_KFC_COMM, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, KfcCommProcessor, profPath); }},
    {PROCESSOR_NAME_DEVICE_TX, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, MsprofTxDeviceProcessor, profPath); }},
    {PROCESSOR_NAME_MSTX, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, MsprofTxHostProcessor, profPath); }},
    {PROCESSOR_NAME_STEP_TRACE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, StepTraceProcessor, profPath); }},
    {PROCESSOR_NAME_TASK, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, TaskProcessor, profPath); }},
    {PROCESSOR_NAME_ACC_PMU, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, AccPmuProcessor, profPath); }},
    {PROCESSOR_NAME_AICORE_FREQ, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, AicoreFreqProcessor, profPath); }},
    {PROCESSOR_NAME_CHIP_TRAINS, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, ChipTransProcessor, profPath); }},
    {PROCESSOR_NAME_DDR, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, DDRProcessor, profPath); }},
    {PROCESSOR_NAME_HBM, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HBMProcessor, profPath); }},
    {PROCESSOR_NAME_HCCS, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HCCSProcessor, profPath); }},
    {PROCESSOR_NAME_NETDEV_STATS, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NetDevStatsProcessor, profPath); }},
    {PROCESSOR_NAME_CPU_USAGE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HostCpuUsageProcessor, profPath); }},
    {PROCESSOR_NAME_MEM_USAGE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HostMemUsageProcessor, profPath); }},
    {PROCESSOR_NAME_DISK_USAGE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HostDiskUsageProcessor, profPath); }},
    {PROCESSOR_NAME_NETWORK_USAGE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HostNetworkUsageProcessor, profPath); }},
    {PROCESSOR_NAME_OSRT_API, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, OSRuntimeApiProcessor, profPath); }},
    {PROCESSOR_NAME_LLC, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, LLcProcessor, profPath); }},
    {PROCESSOR_NAME_NPU_MEM, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NpuMemProcessor, profPath); }},
    {PROCESSOR_NAME_PCIE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, PCIeProcessor, profPath); }},
    {PROCESSOR_NAME_SIO, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, SioProcessor, profPath); }},
    {PROCESSOR_NAME_SOC, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, SocBandwidthProcessor, profPath); }},
    {PROCESSOR_NAME_NIC_TIMELINE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NicTimelineProcessor, profPath); }},
    {PROCESSOR_NAME_ROCE_TIMELINE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, RoCETimelineProcessor, profPath); }},
    {PROCESSOR_NAME_NIC, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NicProcessor, profPath); }},
    {PROCESSOR_NAME_ROCE, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, RoCEProcessor, profPath); }},
    {PROCESSOR_NAME_QOS, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, QosProcessor, profPath); }},
    {PROCESSOR_MC2_COMM_INFO, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, Mc2CommInfoProcessor, profPath); }},
    {PROCESSOR_PMU, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, MetricProcessor, profPath); }},
    {PROCESSOR_NAME_MEMCPY_INFO, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, MemcpyInfoProcessor, profPath); }},
    {PROCESSOR_NAME_NPU_OP_MEM, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NpuOpMemProcessor, profPath); }},
    {PROCESSOR_NAME_NPU_MODULE_MEM, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, NpuModuleMemProcessor, profPath); }},
    {PROCESSOR_NAME_UNIFIED_PMU, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, UnifiedPmuProcessor, profPath); }},
    {PROCESSOR_NAME_FUSION_OP, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, FusionOpProcessor, profPath); }},
    {PROCESSOR_NAME_FUSION_TASK, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, FusionTaskProcessor, profPath); }},
    {PROCESSOR_NAME_MODEL_NAME, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, ModelNameProcessor, profPath); }},
    {PROCESSOR_NAME_COMM_STATISTIC, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HcclStatisticProcessor, profPath); }},
    {PROCESSOR_NAME_OP_STATISTIC, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, OpStatisticProcessor, profPath); }},
    {PROCESSOR_HOST_TASK, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, HostTaskProcessor, profPath); }},
    {PROCESSOR_NAME_OVERLAP_ANALYSIS, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, OverlapAnalysisProcessor, profPath); }},
    {PROCESSOR_NAME_LOW_POWER, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, LowPowerProcessor, profPath); }},
    {PROCESSOR_NAME_BIU_PERF, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, BiuPerfProcessor, profPath); }},
    {PROCESSOR_NAME_UB, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, UbProcessor, profPath); }},
    {PROCESSOR_NAME_BLOCK_DETAIL, [](const std::string &profPath, std::shared_ptr<DataProcessor> &processor)
     { MAKE_SHARED_RETURN_VOID(processor, BlockDetailProcessor, profPath); }}};

std::shared_ptr<DataProcessor> DataProcessorFactory::GetDataProcessByName(const std::string &profPath,
                                                                          const std::string &processName)
{
    std::shared_ptr<DataProcessor> processor = nullptr;
    auto it = processorTable_.find(processName);
    if (it != processorTable_.end())
    {
        it->second(profPath, processor);
    }
    return processor;
}

}  // namespace Domain
}  // namespace Analysis
