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

#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ccu_mission_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/dpu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/unified_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/netdev_stats_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_op_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/overlap_analysis_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;

namespace
{
const std::string DB_PREFIX = "DB:";
const std::string DB_STRING_IDS = DB_PREFIX + TABLE_NAME_STRING_IDS;

}  // namespace

#define REGISTER_DB_SAVER(Name, Dependencies)                                                          \
    REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(DATABASE_PERSISTENCE, DB_PREFIX + Name), true, \
                                DBAssembler::CreateSaver(DB_PREFIX + Name), Dependencies, nullptr)

#define REGISTER_DB_SAVER_WITH_DATA(Name, Dependencies, ...) \
    REGISTER_DB_SAVER(Name, Dependencies);                   \
    REGISTER_TOPO_NODE_DEPENDENT_DATA(TOPO_NODE(DATABASE_PERSISTENCE, DB_PREFIX + Name), __VA_ARGS__)

REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_API, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_API)),
                            std::vector<ApiData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_COMMUNICATION,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMMUNICATION)),
                            std::vector<CommunicationOpData>, std::vector<KfcOpData>,
                            std::vector<CommunicationTaskData>, std::vector<KfcTaskData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_ACC_PMU, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_ACC_PMU)),
                            std::vector<AccPmuData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_AICORE_FREQ,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_AICORE_FREQ),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_LOW_POWER)),
                            std::vector<LowPowerData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_DDR, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DDR)),
                            std::vector<DDRData>);
REGISTER_DB_SAVER(PROCESSOR_NAME_ENUM, TOPO_DEPS());
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_HBM, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HBM)),
                            std::vector<HbmData>);
REGISTER_DB_SAVER(PROCESSOR_NAME_HOST_INFO, TOPO_DEPS());
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_HCCS, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_HCCS)),
                            std::vector<HccsData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NETDEV_STATS,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NETDEV_STATS)),
                            std::vector<NetDevStatsEventData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_LLC, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_LLC)),
                            std::vector<LLcData>);
REGISTER_DB_SAVER(PROCESSOR_NAME_META_DATA, TOPO_DEPS());
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_MSTX, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MSTX)),
                            std::vector<MsprofTxHostData>);
REGISTER_DB_SAVER(PROCESSOR_NAME_NPU_INFO, TOPO_DEPS());
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NPU_MEM, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NPU_MEM)),
                            std::vector<NpuMemData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NPU_OP_MEM, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NPU_OP_MEM)),
                            std::vector<NpuOpMemData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NPU_MODULE_MEM,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NPU_MODULE_MEM)),
                            std::vector<NpuModuleMemData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_PCIE, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_PCIE)),
                            std::vector<PCIeData>);
REGISTER_DB_SAVER(PROCESSOR_NAME_SESSION_TIME_INFO, TOPO_DEPS());
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_SOC, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_SOC)),
                            std::vector<SocBandwidthData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NIC, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NIC)),
                            std::vector<NicOriginalData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_ROCE, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_ROCE)),
                            std::vector<RoceOriginalData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_DPU, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DPU)),
                            std::vector<DPUData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_SIO, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_SIO)),
                            std::vector<SioData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_UB, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_UB)),
                            std::vector<UbData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_TASK,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_TASK),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DEVICE_TX)),
                            std::vector<AscendTaskData>, std::vector<MsprofTxDeviceData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_COMPUTE_TASK_INFO,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_COMPUTE_TASK_INFO),
                                      TOPO_NODE(DATA_PROCESSING, PROCESSOR_MC2_COMM_INFO)),
                            std::vector<TaskInfoData>, std::vector<MC2CommInfoData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_MEMCPY_INFO,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MEMCPY_INFO)),
                            std::vector<MemcpyInfoData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_TASK_PMU_INFO,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_UNIFIED_PMU)),
                            std::vector<UnifiedTaskPmu>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_SAMPLE_PMU_TIMELINE,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_UNIFIED_PMU)),
                            std::vector<UnifiedSampleTimelinePmu>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_SAMPLE_PMU_SUMMARY,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_UNIFIED_PMU)),
                            std::vector<UnifiedSampleSummaryPmu>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_CPU_USAGE, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_CPU_USAGE)),
                            std::vector<CpuUsageData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_CPU_FREQ, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_CPU_FREQ)),
                            std::vector<CpuFreqData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_MEM_USAGE, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_MEM_USAGE)),
                            std::vector<MemUsageData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_DISK_USAGE, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_DISK_USAGE)),
                            std::vector<DiskUsageData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_NETWORK_USAGE,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_NETWORK_USAGE)),
                            std::vector<NetWorkUsageData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_OSRT_API, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_OSRT_API)),
                            std::vector<OSRuntimeApiData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_OVERLAP_ANALYSIS,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_OVERLAP_ANALYSIS)),
                            std::vector<OverlapAnalysisData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_QOS, TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_QOS)),
                            std::vector<QosData>);
REGISTER_DB_SAVER_WITH_DATA(PROCESSOR_NAME_CCU_MISSION,
                            TOPO_DEPS(TOPO_NODE(DATA_PROCESSING, PROCESSOR_NAME_CCU_MISSION)),
                            std::vector<CCUMissionTimelineData>);

REGISTER_TOPO_NODE_SEQUENCE(typeid(void), TOPO_NODE(FLOW_CONTROL, DB_STRING_IDS), true,
                            DBAssembler::CreateStringIdsSaver(), TOPO_DEPS(), DBAssembler::ResolveSelectedDBSavers);

#undef REGISTER_DB_SAVER_WITH_DATA
#undef REGISTER_DB_SAVER
}  // namespace Application
}  // namespace Analysis
