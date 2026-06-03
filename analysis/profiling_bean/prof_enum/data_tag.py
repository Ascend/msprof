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

from enum import Enum
from enum import unique


@unique
class DataTag(Enum):
    """
    Define the tag for profiling data
    """
    ACL = 0
    ACL_HASH = 1
    GE_TASK = 2
    GE_MODEL_TIME = 3
    GE_MODEL_LOAD = 4
    GE_STEP = 5
    RUNTIME_API = 6
    RUNTIME_TRACK = 7
    TS_TRACK = 8
    HWTS = 9
    AI_CORE = 10
    AI_CPU = 11
    TRAINING_TRACE = 12
    DATA_PROCESS = 13
    STARS_LOG = 14
    FFTS_PMU = 15
    AIV = 16
    DVPP = 17
    NIC = 18
    ROCE = 19
    TSCPU = 20
    CTRLCPU = 21
    AICPU = 22
    DDR = 23
    SYS_MEM = 24
    PID_MEM = 25
    SYS_USAGE = 26
    PID_USAGE = 27
    PCIE = 28
    LLC = 29
    HBM = 30
    HCCS = 31
    TS_TRACK_AIV = 32
    L2CACHE = 33
    HWTS_AIV = 34
    HCCL = 35
    MSPROFTX = 36
    GE_TENSOR = 37
    GE_FUSION_OP_INFO = 38
    GE_SESSION = 39
    GE_HASH = 40
    GE_HOST = 41
    SOC_PROFILER = 42
    HELPER_MODEL_WITH_Q = 43
    BIU_PERF = 44
    DATA_QUEUE = 45
    HOST_QUEUE = 46
    PARALLEL_STRATEGY = 47
    NPU_MEM = 50
    FREQ = 52
    HASH_DICT = 53
    API_EVENT = 54
    TASK_TRACK = 56
    MEMCPY_INFO = 57
    HCCL_INFO = 58
    MULTI_THREAD = 59
    GRAPH_ADD_INFO = 60
    TENSOR_ADD_INFO = 61
    NODE_BASIC_INFO = 62
    FUSION_ADD_INFO = 63
    MEMORY_APPLICATION = 64
    CTX_ID = 65
    GE_LOGIC_STREAM_INFO = 66
    MEMORY_OP = 67
    V5_MODEL_EXEOM = 68
    V5_STARS_PROFILE = 69
    DBG_FILE = 70
    NPU_MODULE_MEM = 71
    AICPU_ADD_INFO = 72
    NODE_ATTR_INFO = 73
    HCCL_OP_INFO = 74
    QOS = 75
    STATIC_OP_MEM = 76
    MC2_COMM_INFO = 77
    CAPTURE_STREAM_INFO = 78
    NETDEV_STATS = 79
    UB = 80
    CCU_MISSION = 81
    CCU_CHANNEL = 82
    CCU_TASK = 83
    CCU_WAIT_SIGNAL = 84
    CCU_GROUP = 85
    BIU_PERF_CHIP6 = 86
    SOC_PMU = 87
    LPM_INFO = 88
    STREAM_EXPAND = 89
    RUNTIME_OP_INFO = 90
    DPU_HCCL_TRACK = 91
    DPU_TASK_TRACK = 92


@unique
class AclApiTag(Enum):
    """
    Define the tag for acl api type
    """
    ACL_OP = 1
    ACL_MODEL = 2
    ACL_RTS = 3
    ACL_OTHERS = 4
    ACL_NN = 5
    ACL_ASCENDC = 6
    HOST_HCCL = 7
    ACL_DVPP = 9
    ACL_GRAPH = 10
    ACL_ATB = 11


@unique
class ModuleName(Enum):
    """
    Define the module name for module id
    """
    SLOG = 0
    IDEDD = 1
    SCC = 2
    HCCL = 3
    FMK = 4
    CCU = 5
    DVPP = 6
    RUNTIME = 7
    CCE = 8
    HDC = 9
    DRV = 10
    NET = 11
    DEVMM = 22
    KERNEL = 23
    LIBMEDIA = 24
    CCECPU = 25
    ROS = 27
    HCCP = 28
    ROCE = 29
    TEFUSION = 30
    PROFILING = 31
    DP = 32
    APP = 33
    TS = 34
    TSDUMP = 35
    AICPU = 36
    LP = 37
    TDT = 38
    FE = 39
    MD = 40
    MB = 41
    ME = 42
    IMU = 43
    IMP = 44
    GE = 45
    CAMERA = 47
    ASCENDCL = 48
    TEEOS = 49
    ISP = 50
    SIS = 51
    HSM = 52
    DSS = 53
    PROCMGR = 54
    BBOX = 55
    AIVECTOR = 56
    TBE = 57
    FV = 58
    TUNE = 60
    HSS = 61
    FFTS = 62
    OP = 63
    UDF = 64
    HICAID = 65
    TSYNC = 66
    AUDIO = 67
    TPRT = 68
    ASCENDCKERNEL = 69
    ASYS = 70
    ATRACE = 71
    RTC = 72
    SYSMONITOR = 73
    AML = 74
    ADETECT = 75
    MBUFF = 76
    CUSTOM = 77
