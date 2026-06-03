# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from abc import ABCMeta
from abc import abstractmethod


class DBNameConstant(metaclass=ABCMeta):
    """
    DB name and DB table names
    """

    # DB name
    DB_ACL_MODULE = "acl_module.db"
    DB_AICORE_OP_SUMMARY = "ai_core_op_summary.db"
    DB_RUNTIME = "runtime.db"
    DB_GE_MODEL_INFO = "ge_model_info.db"
    DB_GE_HOST_INFO = "ge_host_info.db"
    DB_GE_LOGIC_STREAM_INFO = "ge_logic_stream_info.db"
    DB_STREAM_INFO = "stream_info.db"
    DB_AI_CPU = "ai_cpu.db"
    DB_GE_INFO = "ge_info.db"
    DB_HASH = "hash_mapping.db"
    DB_HCCS = "hccs.db"
    DB_HOST_SYS_USAGE_CPU = "cpu_usage.db"
    DB_HOST_SYS_USAGE_MEM = "memory.db"
    DB_HOST_CPU_USAGE = "host_cpu_usage.db"
    DB_HOST_DISK_USAGE = "host_disk_usage.db"
    DB_HOST_MEM_USAGE = "host_mem_usage.db"
    DB_HOST_NETWORK_USAGE = "host_network_usage.db"
    DB_HOST_RUNTIME_API = "host_runtime_api.db"
    DB_HWTS = "hwts.db"
    DB_HWTS_AIV = "hwts_aiv.db"
    DB_HWTS_REC = "hwts-rec.db"
    DB_L2CACHE = "l2cache.db"
    DB_LLC = "llc.db"
    DB_OP_COUNTER = "op_counter.db"
    DB_PERIPHERAL = "peripheral.db"
    DB_RTS_TRACK = "rts_track.db"
    DB_TRACE = "trace.db"
    DB_TIME = "time.db"
    DB_STEP_TRACE = "step_trace.db"
    DB_MEMORY_COPY = "memory_copy.db"
    DB_TASK_PMU = "task_pmu.db"
    DB_NIC_ORIGIN = "nic.db"
    DB_ROCE_ORIGIN = "roce.db"
    DB_NIC_RECEIVE = "nicreceivesend_table.db"
    DB_ROCE_RECEIVE = "rocereceivesend_table.db"
    DB_DDR = "ddr.db"
    DB_PCIE = "pcie.db"
    DB_HBM = "hbm.db"
    DB_QOS = "qos.db"
    DB_HCCL = "hccl.db"
    DB_MSPROFTX = 'msproftx.db'
    DB_GE_HASH = "ge_hash.db"
    DB_SOC_LOG = "soc_log.db"
    DB_ACC_PMU = "acc_pmu.db"
    DB_STARS_SOC = "soc_profiler.db"
    DB_SIO = "sio.db"
    DB_STARS_CHIP_TRANS = "chip_trans.db"
    DB_LOW_POWER = "lowpower.db"
    DB_BIU_PERF = "biu_perf.db"
    DB_CLUSTER_RANK = "cluster_rank.db"
    DB_CLUSTER_STEP_TRACE = 'cluster_step_trace.db'
    DB_CLUSTER_DATA_PREPROCESS = 'data_preprocess.db'
    DB_CLUSTER_HCCL = "cluster_hccl.db"
    DB_PARALLEL = "parallel.db"
    DB_CLUSTER_PARALLEL = "cluster_parallel.db"
    DB_NPU_MEM = "npu_mem.db"
    DB_SYNC_ACL_NPU = "sync_acl_npu.db"
    DB_FREQ = "freq.db"
    DB_API_EVENT = "api_event.db"
    DB_HCCL_INFO = "hccl_info.db"
    DB_MULTI_THREAD = "multi_thread.db"
    DB_TENSOR_ADD_INFO = "tensor_info.db"
    DB_NODE_BASIC_INFO = "node_basic_info.db"
    DB_FUSION_ADD_INFO = "fusion_op_info.db"
    DB_GRAPH_ADD_INFO = "graph_id_map.db"
    DB_CTX_ID = "ctx_id.db"
    DB_STATIC_OP_MEM = "static_op_mem.db"
    DB_MEMORY_APPLICATION = "memory_application.db"
    DB_MEMORY_OP = "task_memory.db"
    DB_ASCEND_TASK = "ascend_task.db"
    DB_NPU_MODULE_MEM = "npu_module_mem.db"
    DB_NODE_ATTR_INFO = "node_attr_info.db"
    DB_STREAM_EXPAND_SPEC = "stream_expand_spec.db"
    DB_NAME_AICORE = "aicore.db"
    DB_NAME_AI_VECTOR_CORE = "ai_vector_core.db"
    DB_NAME_AICPU = "aicpu.db"
    DB_NAME_CTRLCPU = "ctrlcpu.db"
    DB_NAME_TSCPU = "tscpu.db"
    DB_MC2_COMM_INFO = "mc2_comm_info.db"
    DB_KFC_INFO = "kfc_info.db"
    DB_NETDEV_STATS = "netdev_stats.db"
    DB_UB = "ub.db"
    DB_CCU = "ccu.db"
    DB_CCU_ADD_INFO = "ccu_add_info.db"
    DB_SOC_PMU = "soc_pmu.db"
    DB_VOLTAGE = "voltage.db"
    DB_FUSION_TASK = "fusion_task.db"
    DB_DPU = "dpu.db"

    # DB tables
    TABLE_ACL_DATA = "AclData"

    TABLE_AI_CORE_REC = "AiCoreRec"
    TABLE_AI_CPU = "AiCpuData"
    TABLE_AI_CPU_FROM_TS = "AiCpuFromTs"
    TABLE_AI_CPU_DP = "AiCpuDP"
    TABLE_ALL_REDUCE = "all_reduce"
    TABLE_CLUSTER_STEP_TRACE = "ClusterStepTrace_{}"
    TABLE_CLUSTER_ALL_REDUCE = "AllReduce_{}"
    TABLE_EVENT_COUNTER = "EventCounter"
    TABLE_EVENT_COUNT = "EventCount"
    TABLE_GE_LOAD_TABLE = "GELoad"
    TABLE_GE_GRAPH = "ge_graph_data"
    TABLE_GE_INFER = "GEInfer"
    TABLE_GE_STEP_INFO_DATA = "step_info_data"
    TABLE_GE_HOST = "GEHostInfo"
    TABLE_GE_LOGIC_STREAM_INFO = "GeLogicStreamInfo"
    TABLE_CAPTURE_STREAM_INFO = "CaptureStreamInfo"
    TABLE_HASH_ACL = "hash_acl_dict"
    TABLE_HWTS_SYS_RANGE = "hwts_sys_cnt_range"
    TABLE_HWTS_ITER_SYS = "HwtsIter"
    TABLE_HWTS_BATCH = "HwtsBatch"
    TABLE_HWTS_TASK = "HwtsTask"
    TABLE_HWTS_TASK_TIME = "HwtsTaskTime"
    TABLE_LLC_METRIC_DATA = "LLCMetricData"
    TABLE_LLC_DSID = "LLCDsidData"

    # MetricSummary
    DB_METRICS_SUMMARY = "metric_summary.db"
    TABLE_METRIC_SUMMARY = "MetricSummary"
    TABLE_V6_BLOCK_PMU = "V6BlockPmu"
    TABLE_AIV_METRIC_SUMMARY = "AivMetricSummary"

    DB_HCCL_SINGLE_DEVICE = "hccl_single_device.db"

    DB_COMMUNICATION_ANALYZER = "communication_analyzer.db"

    TABLE_HCCL_TASK_SINGLE_DEVICE = "HCCLTaskSingleDevice"
    TABLE_HCCL_OP_REPORT = "HcclOpReport"
    TABLE_HCCL_OP_SINGLE_DEVICE = "HCCLOpSingleDevice"

    TABLE_OP_COUNTER_GE_MERGE = "ge_task_merge"
    TABLE_OP_COUNTER_OP_REPORT = "op_report"
    TABLE_OP_COUNTER_RTS_TASK = "rts_task"
    TABLE_RUNTIME_REPORT_TASK = "ReportTask"
    TABLE_RUNTIME_TASK_TIME = "TaskTime"
    TABLE_RUNTIME_TIMELINE = "TimeLine"
    TABLE_RUNTIME_TRACK = "RuntimeTrack"
    TABLE_TASK_TRACK = "TaskTrack"
    TABLE_RUNTIME_OP_INFO = "RuntimeOpInfo"
    TABLE_MEMCPY_INFO = "MemcpyInfo"
    TABLE_SUMMARY_GE = "ge_summary"
    TABLE_SUMMARY_METRICS = "ai_core_metrics"
    TABLE_SUMMARY_TASK_TIME = "task_time"
    TABLE_SUMMARY_TENSOR = "ge_tensor"
    TABLE_TS_MEMCPY_CALCULATION = "TsMemcpyCalculation"
    TABLE_TIME = 'Time'
    TABLE_TRACE_FILES = 'files'
    TABLE_TRAINING_TRACE = 'training_trace'
    TABLE_GET_NEXT = 'get_next'
    # step trace
    TABLE_STEP_TRACE = "StepTrace"
    TABLE_TS_MEMCPY = "TsMemcpy"
    TABLE_MODEL_WITH_Q = "ModelWithQ"
    TABLE_TASK_TYPE = "TaskType"
    TABLE_STEP_TRACE_DATA = "step_trace_data"
    TABLE_STEP_TIME = "StepTime"
    TABLE_BLOCK_NUM = "TsBlockNum"

    # cpu usage
    TABLE_HOST_CPU_INFO = "CpuInfo"
    TABLE_HOST_CPU_USAGE = "CpuUsage"
    # disk usage
    TABLE_HOST_DISK_USAGE = "DiskUsage"
    # mem usage
    TABLE_HOST_MEM_USAGE = "MemUsage"
    # network usage
    TABLE_HOST_NETWORK_USAGE = "NetworkUsage"
    TABLE_HOST_PROCESS_USAGE = "ProcessUsage"
    # host syscal
    TABLE_HOST_RUNTIME_API = "Syscall"
    # stars
    TABLE_ACSQ_TASK = "AcsqTask"
    TABLE_ACSQ_TASK_TIME = "AcsqTaskTime"
    TABLE_THREAD_TASK = "ThreadTime"
    TABLE_SUBTASK_TIME = "SubtaskTime"
    TABLE_L2CACHE_PARSE = 'L2CacheParse'
    TABLE_L2CACHE_SAMPLE = 'L2CacheSample'
    TABLE_L2CACHE_SUMMARY = 'L2CacheSummary'
    TABLE_BLOCK_LOG = "BlockLog"
    TABLE_SOC_PMU = "SocPmu"
    TABLE_SOC_PMU_SUMMARY = "SocPmuSummary"

    # dvpp
    TABLE_DVPP_ORIGIN = "DvppOriginalData"
    TABLE_DVPP_REPORT = "DvppReportData"
    TABLE_DVPP_TREE = "DvppTreeData"
    # nic
    TABLE_NIC_ORIGIN = 'NicOriginalData'
    TABLE_NIC_REPORT = 'NicReportData'
    TABLE_NIC_RECEIVE = 'NicReceiveSend'
    # roce
    TABLE_ROCE_ORIGIN = 'RoceOriginalData'
    TABLE_ROCE_REPORT = 'RoceReportData'
    TABLE_ROCE_RECEIVE = 'RoceReceiveSend'
    # tscpu
    TABLE_TSCPU_ORIGIN = "TsOriginalData"
    # ddr
    TABLE_DDR_ORIGIN = "DDROriginalData"
    TABLE_DDR_METRIC = "DDRMetricData"
    # sys_mem
    TABLE_SYS_MEM = "sysmem"
    TABLE_PID_MEM = "pidmem"
    # sys_usage
    TABLE_SYS_USAGE = "SysCpuUsage"
    TABLE_PID_USAGE = "ProCpuUsage"
    # pcie
    TABLE_PCIE = "PcieOriginalData"
    # hbm
    TABLE_HBM_ORIGIN = "HBMOriginalData"
    TABLE_HBM_BW = "HBMbwData"
    # qos
    TABLE_QOS_BW = "QosBwData"
    # hccs
    TABLE_HCCS_ORIGIN = 'HCCSOriginalData'
    TABLE_HCCS_EVENTS = 'HCCSEventsData'
    # ub
    TABLE_UB_BW = "UBBwData"

    # llc
    TABLE_LLC_ORIGIN = "LLCOriginalData"
    TABLE_LLC_EVENTS = "LLCEvents"
    TABLE_LLC_METRICS = "LLCMetrics"
    TABLE_MINI_LLC_METRICS = "LLCMetricData"
    TABLE_LLC_CAPACITY = "LLCCapacity"
    TABLE_LLC_BANDWIDTH = "LLCBandwidth"
    # aicpu/ctrl_cpu
    TABLE_CPU_ORIGIN = "OriginalData"

    # tscpu
    TABLE_TS_CPU_EVENT = "EventCount"
    TABLE_TS_CPU_HOT_INS = "HotIns"

    # msproftx
    TABLE_MSPROFTX = "MsprofTx"
    TABLE_MSPROFTX_EX = "MsprofTxEx"

    # stars
    TABLE_PCIE_DATA = "PcieData"
    TABLE_ACC_PMU_DATA = "AccPmu"
    TABLE_ACC_PMU_ORIGIN_DATA = "AccPmuOrigin"
    TABLE_LPE_DATA = 'Lpe'
    TABLE_LPS_DATA = "Lps"
    TABLE_SOC_DATA = "InterSoc"
    TABLE_STARS_DATA = "StarsMessage"
    TABLE_FFTS_LOG = "FftsLog"
    TABLE_STARS_PA_LINK = "PaLinkInfo"
    TABLE_STARS_PCIE = "PcieInfo"
    TABLE_STARS_PCIE_V6 = "PcieInfoV6"
    TABLE_LOWPOWER = "LowPower"
    TABLE_SIO = "Sio"

    # ge
    TABLE_GE_TASK = "TaskInfo"
    TABLE_GE_SESSION = "SessionInfo"
    TABLE_GE_HASH = "GeHashInfo"
    TABLE_TYPE_HASH = "TypeHashInfo"

    # ge model
    TABLE_GE_FUSION_OP_INFO = "GeFusionOpInfo"
    TABLE_MODEL_NAME = "ModelName"

    # biu perf
    TABLE_FLOW_MONITOR = "MonitorFlow"
    TABLE_CYCLES_MONITOR = "MonitorCycles"
    TABLE_BIU_FLOW = "BiuFlow"
    TABLE_BIU_CYCLES = "BiuCycles"
    TABLE_BIU_DATA = "OriBiuData"
    TABLE_BIU_INSTR_STATUS = "BiuInstrStatus"

    # cluster
    TABLE_CLUSTER_RANK = "ClusterRank"
    TABLE_DATA_QUEUE = "DataQueue"
    TABLE_HCCL_OPERATOR_EXE = "HcclOperatorExe"
    TABLE_PARALLEL_STRATEGY = "ParallelStrategy"
    TABLE_HCCL_OPERATOR_OVERLAP = "HcclOperatorOverlap"
    TABLE_COMPUTATION_TIME = "ComputationTime"
    TABLE_CLUSTER_DATA_PARALLEL = "ClusterDataParallel"
    TABLE_CLUSTER_MODEL_PARALLEL = "ClusterModelParallel"
    TABLE_CLUSTER_PIPELINE_PARALLEL = "ClusterPipelineParallel"
    TABLE_CLUSTER_PARALLEL_STRATEGY = "ClusterParallelStrategy"
    TABLE_HOST_QUEUE = "HostQueue"

    # ccu
    TABLE_CCU_MISSION = "OriginMission"
    TABLE_CCU_CHANNEL = "OriginChannel"
    TABLE_CCU_TASK_INFO = "CCUTaskInfo"
    TABLE_CCU_WAIT_SIGNAL_INFO = "CCUWaitSignalInfo"
    TABLE_CCU_GROUP_INFO = "CCUGroupInfo"

    # npu mem of process and device
    TABLE_NPU_MEM = "NpuMem"

    # npu module mem
    TABLE_NPU_MODULE_MEM = "NpuModuleMem"

    # npu operator mem
    TABLE_NPU_OP_MEM_RAW = "NpuOpMemRaw"
    TABLE_NPU_OP_MEM = "NpuOpMem"
    TABLE_NPU_OP_MEM_REC = "NpuOpMemRec"

    # sync_acl_npu
    TABLE_TORCH_TO_ACL = "TorchAclRelation"
    TABLE_TORCH_TO_NPU = "TorchNpuRelation"

    # freq
    TABLE_FREQ_PARSE = "FreqParse"

    # voltage
    TABLE_AIC_VOLTAGE = "AicVoltage"
    TABLE_BUS_VOLTAGE = "BusVoltage"

    # new struct
    TABLE_API_DATA = 'ApiData'
    TABLE_EVENT_DATA = 'EventData'
    TABLE_HCCL_INFO_DATA = 'HcclInfoData'
    TABLE_MULTI_THREAD_DATA = 'MultiThreadData'
    TABLE_TENSOR_ADD_INFO = "TensorInfoV2"
    TABLE_NODE_BASIC_INFO = "NodeBasicInfo"
    TABLE_FUSION_ADD_INFO = "FusionOPInfo"
    TABLE_GRAPH_ADD_INFO = "GraphIdMap"
    TABLE_CTX_ID_ADD_INFO = "CtxId"
    TABLE_CTX_ID = "CtxId"
    TABLE_STATIC_OP_MEM = "StaticOpMem"
    TABLE_MEMORY_APPLICATION = "MemoryApplication"
    TABLE_HCCL_OP = "HCCLOP"
    TABLE_HCCL_TASK = "HCCLTask"
    TABLE_HOST_TASK = "HostTask"
    TABLE_ASCEND_TASK = "AscendTask"
    TABLE_NODE_ATTR_INFO = "NodeAttrInfo"
    TABLE_HCCL_OP_INFO = "HcclOpInfo"
    TABLE_STREAM_EXPAND_SPEC = "StreamExpandSpec"

    # dpu
    TABLE_DPU_TASK_TRACK = "DPUTaskTrack"
    TABLE_DPU_HCCL_TRACK = 'DPUHcclTrack'

    # v5
    TABLE_V5_TASK = "V5Task"

    # flip
    TABLE_HOST_TASK_FLIP = "HostTaskFlip"
    TABLE_DEVICE_TASK_FLIP = "DeviceTaskFlip"

    # communication analyzer time, bandwidth, matrix
    TABLE_COMM_ANALYZER_TIME = "CommAnalyzerTime"
    TABLE_COMM_ANALYZER_BAND = "CommAnalyzerBandwidth"
    TABLE_COMM_ANALYZER_MATRIX = "CommAnalyzerMatrix"

    # mc2
    TABLE_MC2_COMM_INFO = "Mc2CommInfo"
    TABLE_KFC_INFO = "KfcInfo"
    TABLE_KFC_COMM_TURN = "KfcCommTurn"
    TABLE_KFC_COMPUTE_TURN = "KfcComputeTurn"
    TABLE_KFC_OP = "KfcOP"
    TABLE_KFC_TASK = "KfcTask"
    TABLE_DEVICE_HCCL_OP_INFO = "DeviceHcclOpInfo"
    TABLE_AICPU_TASK_FLIP = "AicpuTaskFlip"
    TABLE_AICPU_MASTER_STREAM_HCCL_TASK = "AicpuMasterStreamHcclTask"

    # netdev stats
    TABLE_NETDEV_STATS_ORIGIN = 'NetDevStatsOriginalData'

    # fusion task
    TABLE_FUSION_TASK = "FusionTask"

    @abstractmethod
    def get_db_name(self: any) -> str:
        """
        get db name
        :return: db name
        """

    @abstractmethod
    def get_table_name(self: any) -> str:
        """
        get table name
        :return: table name
        """
