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

#include <cstdint>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/block_detail_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/biu_perf_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/overlap_analysis_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;

namespace
{
const std::string TIMELINE_PREFIX = "TIMELINE:";
const std::string TIMELINE_PRE_DUMP = TIMELINE_PREFIX + "PRE_DUMP";
const std::string TIMELINE_POST_DUMP = TIMELINE_PREFIX + "POST_DUMP";
}  // namespace

#define REGISTER_TIMELINE_NODE(Name, Dependencies)                                    \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(TIMELINE_EXPORT, Name), true, \
                                TimelineManager::CreateTimelineAssembler(Name), Dependencies, nullptr)

#define REGISTER_TIMELINE_NODE_WITH_DATA(Name, Dependencies, ...) \
    REGISTER_TIMELINE_NODE(Name, Dependencies);                   \
    REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(TIMELINE_EXPORT, Name), __VA_ARGS__)

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP), true,
                            TimelineManager::CreateTimelinePreDump(), TOPO_DEPS(),
                            TimelineManager::ResolveTimelinePreDumpDependencies);
REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP), std::vector<MsprofTxHostData>);

REGISTER_TIMELINE_NODE_WITH_DATA(
    PROCESS_TASK,
    TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP), TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
              TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_KFC_TASK),
              TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
              TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_API), TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MEMCPY_INFO)),
    std::unordered_map<uint32_t, uint32_t>, std::vector<TaskInfoData>, std::vector<ApiData>,
    std::vector<MemcpyInfoData>, std::vector<AscendTaskData>, std::vector<KfcTurnData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_ACC_PMU,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_ACC_PMU)),
                                 std::vector<AccPmuData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_API,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_API),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK)),
                                 std::vector<AscendTaskData>, std::vector<ApiData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_DDR,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DDR)),
                                 std::vector<DDRData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_STARS_CHIP_TRANS,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_CHIP_TRAINS),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_PCIE)),
                                 std::vector<PaLinkInfoData>, std::vector<PcieInfoData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_HBM,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HBM)),
                                 std::vector<HbmData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_HCCL,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_KFC_TASK)),
                                 std::vector<CommunicationTaskData>, std::vector<CommunicationOpData>,
                                 std::vector<KfcTaskData>, std::vector<KfcOpData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_CCU,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_CCU_MISSION)),
                                 std::vector<CCUMissionTimelineData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_HCCS,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HCCS)),
                                 std::vector<HccsData>);

#define REGISTER_SIMPLE_TIMELINE_NODE(Name, Processor, DataType) \
    REGISTER_TIMELINE_NODE_WITH_DATA(                            \
        Name, TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP), TOPO_NODE(DATA_PROCESSING, Processor)), DataType)

REGISTER_SIMPLE_TIMELINE_NODE(PROCESSOR_NAME_OSRT_API, PROCESSOR_NAME_OSRT_API, std::vector<OSRuntimeApiData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_NETWORK_USAGE, PROCESSOR_NAME_NETWORK_USAGE, std::vector<NetWorkUsageData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_DISK_USAGE, PROCESSOR_NAME_DISK_USAGE, std::vector<DiskUsageData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_MEMORY_USAGE, PROCESSOR_NAME_MEM_USAGE, std::vector<MemUsageData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_CPU_USAGE, PROCESSOR_NAME_CPU_USAGE, std::vector<CpuUsageData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_MSPROFTX, PROCESSOR_NAME_MSTX, std::vector<MsprofTxHostData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_NPU_MEM, PROCESSOR_NAME_NPU_MEM, std::vector<NpuMemData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_OVERLAP_ANALYSE, PROCESSOR_NAME_OVERLAP_ANALYSIS,
                              std::vector<OverlapAnalysisData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_PCIE, PROCESSOR_NAME_PCIE, std::vector<PCIeData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_SIO, PROCESSOR_NAME_SIO, std::vector<SioData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_STARS_SOC, PROCESSOR_NAME_SOC, std::vector<SocBandwidthData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_LLC, PROCESSOR_NAME_LLC, std::vector<LLcData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_NIC, PROCESSOR_NAME_NIC_TIMELINE, std::vector<NicReceiveSendData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_ROCE, PROCESSOR_NAME_ROCE_TIMELINE, std::vector<RoceReceiveSendData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_QOS, PROCESSOR_NAME_QOS, std::vector<QosData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_BIU_PERF, PROCESSOR_NAME_BIU_PERF, std::vector<BiuPerfData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_UB, PROCESSOR_NAME_UB, std::vector<UbData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_DPU, PROCESSOR_NAME_DPU, std::vector<DPUData>);
REGISTER_SIMPLE_TIMELINE_NODE(PROCESS_FUSION_TASK, PROCESSOR_NAME_FUSION_TASK, std::vector<FusionTaskTimelineData>);

#undef REGISTER_SIMPLE_TIMELINE_NODE

REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_STEP_TRACE,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_STEP_TRACE)),
                                 std::vector<TrainTraceData>, std::vector<AllReduceData>, std::vector<GetNextData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_LOW_POWER,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_AICORE_FREQ),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_LOW_POWER)),
                                 std::vector<LowPowerData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_DEVICE_TX,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DEVICE_TX),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MSTX)),
                                 std::vector<MsprofTxDeviceData>, std::vector<MsprofTxHostData>);
REGISTER_TIMELINE_NODE_WITH_DATA(PROCESS_BLOCK_DETAIL,
                                 TOPO_DEPS(TOPO_NODE(FLOW_CONTROL, TIMELINE_PRE_DUMP),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_BLOCK_DETAIL),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                           TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK)),
                                 std::vector<BlockDetailData>, std::vector<TaskInfoData>, std::vector<AscendTaskData>);

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, TIMELINE_POST_DUMP), true,
                            TimelineManager::CreateTimelinePostDump(), TOPO_DEPS(),
                            TimelineManager::ResolveSelectedTimelineNodes);

#undef REGISTER_TIMELINE_NODE_WITH_DATA
#undef REGISTER_TIMELINE_NODE
}  // namespace Application
}  // namespace Analysis
