# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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


class TableFields:
    ACC_PMU = ["accId", "readBwLevel", "writeBwLevel", "readOstLevel", "writeOstLevel", "timestampNs", "deviceId"]
    AICORE_FREQ = ["deviceId", "timestampNs", "freq", "dieId"]
    CANN_API = ["startNs", "endNs", "type", "globalTid", "connectionId", "name"]
    COMMUNICATION_OP = [
        "opName",
        "startNs",
        "endNs",
        "connectionId",
        "groupName",
        "opId",
        "relay",
        "retry",
        "dataType",
        "algType",
        "count",
        "opType",
        "deviceId",
        "rankSize",
    ]
    COMMUNICATION_TASK_INFO = [
        "name",
        "globalTaskId",
        "taskType",
        "planeId",
        "groupName",
        "notifyId",
        "rdmaType",
        "srcRank",
        "dstRank",
        "transportType",
        "size",
        "dataType",
        "linkType",
        "opId",
        "isMaster",
        "bandwidth",
    ]
    COMPUTE_TASK_INFO = [
        "name",
        "globalTaskId",
        "blockNum",
        "mixBlockNum",
        "taskType",
        "opType",
        "inputFormats",
        "inputDataTypes",
        "inputShapes",
        "outputFormats",
        "outputDataTypes",
        "outputShapes",
        "attrInfo",
        "opState",
        "hf32Eligible",
        "gridDim",
        "blockDim",
    ]
    CONNECTION_IDS = ["id", "connectionId"]
    CPU_USAGE = ["timestampNs", "cpuId", "usage"]
    ENUM_API_TYPE = ["id", "name"]
    ENUM_HCCL_DATA_TYPE = ["id", "name"]
    ENUM_HCCL_LINK_TYPE = ["id", "name"]
    ENUM_HCCL_RDMA_TYPE = ["id", "name"]
    ENUM_HCCL_TRANSPORT_TYPE = ["id", "name"]
    ENUM_MEMCPY_OPERATION = ["id", "name"]
    ENUM_MODULE = ["id", "name"]
    ENUM_MSTX_EVENT_TYPE = ["id", "name"]
    HBM = ["deviceId", "timestampNs", "bandwidth", "hbmId", "type"]
    HCCS = ["deviceId", "timestampNs", "txThroughput", "rxThroughput"]
    HOST_INFO = ["hostUid", "hostName"]
    HOST_MEM_USAGE = ["timestampNs", "usage"]
    LLC = ["deviceId", "llcId", "timestampNs", "hitRate", "throughput", "mode"]
    MEMCPY_INFO = ["globalTaskId", "size", "memcpyOperation"]
    MEMORY_RECORD = [
        "component",
        "timestamp",
        "totalAllocated",
        "totalReserved",
        "totalActive",
        "streamPtr",
        "deviceId",
    ]
    META_DATA = ["name", "value"]
    NETDEV_STATS = [
        "deviceId",
        "timestampNs",
        "macTxPfcPkt",
        "macRxPfcPkt",
        "macTxByte",
        "macTxBandwidth",
        "macRxByte",
        "macRxBandwidth",
        "macTxBadByte",
        "macRxBadByte",
        "roceTxPkt",
        "roceRxPkt",
        "roceTxErrPkt",
        "roceRxErrPkt",
        "roceTxCnpPkt",
        "roceRxCnpPkt",
        "roceNewPktRty",
        "nicTxByte",
        "nicTxBandwidth",
        "nicRxByte",
        "nicRxBandwidth",
    ]
    NIC = [
        "deviceId",
        "timestampNs",
        "bandwidth",
        "rxPacketRate",
        "rxByteRate",
        "rxPackets",
        "rxBytes",
        "rxErrors",
        "rxDropped",
        "txPacketRate",
        "txByteRate",
        "txPackets",
        "txBytes",
        "txErrors",
        "txDropped",
        "funcId",
    ]
    NPU_INFO = ["id", "name"]
    NPU_MEM = ["type", "ddr", "hbm", "timestampNs", "deviceId"]
    NPU_MODULE_MEM = ["moduleId", "timestampNs", "totalReserved", "deviceId"]
    NPU_OP_MEM = [
        "operatorName",
        "addr",
        "type",
        "size",
        "timestampNs",
        "globalTid",
        "totalAllocate",
        "totalReserve",
        "component",
        "deviceId",
    ]
    OP_MEMORY = [
        "name",
        "size",
        "allocationTime",
        "releaseTime",
        "activeReleaseTime",
        "duration",
        "activeDuration",
        "allocationTotalAllocated",
        "allocationTotalReserved",
        "allocationTotalActive",
        "releaseTotalAllocated",
        "releaseTotalReserved",
        "releaseTotalActive",
        "streamPtr",
        "deviceId",
    ]
    PCIE = [
        "deviceId",
        "timestampNs",
        "txPostMin",
        "txPostMax",
        "txPostAvg",
        "txNonpostMin",
        "txNonpostMax",
        "txNonpostAvg",
        "txCplMin",
        "txCplMax",
        "txCplAvg",
        "txNonpostLatencyMin",
        "txNonpostLatencyMax",
        "txNonpostLatencyAvg",
        "rxPostMin",
        "rxPostMax",
        "rxPostAvg",
        "rxNonpostMin",
        "rxNonpostMax",
        "rxNonpostAvg",
        "rxCplMin",
        "rxCplMax",
        "rxCplAvg",
    ]
    PYTORCH_API = [
        "startNs",
        "endNs",
        "globalTid",
        "connectionId",
        "name",
        "sequenceNumber",
        "fwdThreadId",
        "inputDtypes",
        "inputShapes",
        "callchainId",
        "type",
    ]
    PYTORCH_CALLCHAINS = ["id", "stack", "stackDepth"]
    QOS = ["deviceId", "eventName", "bandwidth", "timestampNs"]
    ROCE = [
        "deviceId",
        "timestampNs",
        "bandwidth",
        "rxPacketRate",
        "rxByteRate",
        "rxPackets",
        "rxBytes",
        "rxErrors",
        "rxDropped",
        "txPacketRate",
        "txByteRate",
        "txPackets",
        "txBytes",
        "txErrors",
        "txDropped",
        "funcId",
    ]
    SESSION_TIME_INFO = ["startTimeNs", "endTimeNs"]
    SOC_BANDWIDTH_LEVEL = ["l2BufferBwLevel", "mataBwLevel", "timestampNs", "deviceId"]
    STEP_TIME = ["id", "startNs", "endNs"]
    STRING_IDS = ["id", "value"]
    TASK = [
        "startNs",
        "endNs",
        "deviceId",
        "connectionId",
        "globalTaskId",
        "globalPid",
        "taskType",
        "contextId",
        "streamId",
        "taskId",
        "modelId",
    ]
    TASK_PMU_INFO = ["globalTaskId", "name", "value"]
    RANK_DEVICE_MAP = ["rankId", "deviceId"]
    ClusterCommunicationBandwidth = [
        "step",
        "rank_id",
        "hccl_op_name",
        "group_name",
        "band_type",
        "transit_size",
        "transit_time",
        "bandwidth",
        "large_packet_ratio",
        "package_size",
        "count",
        "total_duration",
    ]
    ClusterCommunicationMatrix = [
        "step",
        "hccl_op_name",
        "group_name",
        "src_rank",
        "dst_rank",
        "transport_type",
        "op_name",
        "transit_size",
        "transit_time",
        "bandwidth",
    ]
    ClusterCommunicationTime = [
        "step",
        "rank_id",
        "hccl_op_name",
        "group_name",
        "start_timestamp",
        "elapsed_time",
        "transit_time",
        "wait_time",
        "synchronization_time",
        "idle_time",
        "synchronization_time_ratio",
        "wait_time_ratio",
    ]
    ClusterStepTraceTime = [
        "step",
        "type",
        "index",
        "computing",
        "communication_not_overlapped",
        "overlapped",
        "communication",
        "free",
        "stage",
        "bubble",
        "communication_not_overlapped_and_exclude_receive",
        "preparing",
        "dp_index",
        "pp_index",
        "tp_index",
    ]
    CommunicationGroupMapping = ["type", "rank_set", "group_name", "group_id", "pg_name"]
    HostInfo = ["hostUid", "hostName"]
    RankDeviceMap = ["rankId", "deviceId", "hostUid", "profilePath"]
    SAMPLE_PMU_SUMMARY = ["deviceId", "metric", "value", "coreId", "coreType"]
    SAMPLE_PMU_TIMELINE = ["deviceId", "timestampNs", "totalCycle", "usage", "freq", "coreId", "coreType"]
    ENUM_OVERLAY_ANALYSIS_TYPE = ["id", "name"]
    OVERLAP_ANALYSIS = ["id", "deviceId", "startNs", "endNs", "type"]
    SIO = ["deviceId", "name", "timestampNs", "rxReq", "rxRsp", "rxSnp", "rxDat", "txReq", "txRsp", "txSnp", "txDat"]

    @classmethod
    def get_fields(cls, name):
        """
        Retrieve the list of field names with the specified name.
        :param name: The name of the field list.
        :return: The corresponding list of field names, returning None if the name does not exist.
        """
        if hasattr(cls, name):
            return getattr(cls, name)
        else:
            return None
