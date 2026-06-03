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
class ExportDataType(Enum):
    """
    data type for summary and timeline.
    """

    INVALID = -1
    MSPROF_TX = 0
    STEP_TRACE = 1
    TASK_TIME = 7
    HBM = 8
    DDR = 9
    PCIE = 10
    HCCS = 11
    NIC = 12
    ROCE = 13
    DVPP = 14
    L2_CACHE = 15
    LLC_READ_WRITE = 16
    LLC_AICPU = 17
    LLC_CTRLCPU = 18
    LLC_BANDWIDTH = 19
    AI_CORE_UTILIZATION = 21
    AI_VECTOR_CORE_UTILIZATION = 22
    HOST_CPU_USAGE = 23
    HOST_MEM_USAGE = 24
    HOST_NETWORK_USAGE = 25
    HOST_DISK_USAGE = 26
    OS_RUNTIME_API = 27
    OS_RUNTIME_STATISTIC = 28
    ACSQ_TASK_STATISTIC = 30
    FFTS_SUB_TASK_TIME = 31
    COMMUNICATION = 32
    CPU_USAGE = 33
    PROCESS_CPU_USAGE = 34
    SYS_MEM = 35
    PROCESS_MEM = 36
    OP_SUMMARY = 37
    OP_STATISTIC = 38
    AICPU = 39
    DP = 40
    FUSION_OP = 41
    CTRL_CPU_PMU_EVENTS = 42
    CTRL_CPU_TOP_FUNCTION = 43
    AI_CPU_PMU_EVENTS = 44
    AI_CPU_TOP_FUNCTION = 45
    TS_CPU_PMU_EVENTS = 46
    TS_CPU_TOP_FUNCTION = 47
    STARS_SOC = 48
    STARS_CHIP_TRANS = 49
    LOW_POWER = 51
    INSTR = 52
    ACC_PMU = 53
    NPU_MEM = 55
    OPERATOR_MEMORY = 57
    MEMORY_RECORD = 58
    EVENT = 59
    API = 60
    COMMUNICATION_STATISTIC = 61
    API_STATISTIC = 62
    NPU_MODULE_MEM = 63
    SIO = 64
    AICPU_MI = 65
    QOS = 66
    STATIC_OP_MEM = 67
    BLOCK_DETAIL = 68
    UB = 69
    CCU_MISSION = 70
    CCU_CHANNEL = 71
    CCU_TASK_INFO = 72
    CCU_GROUP_INFO = 73
    CCU_WAIT_SIGNAL_INFO = 74
    SOC_PMU = 75
    FREQ = 76
    VOLTAGE = 77
    DPU = 78
    FUSION_TASK = 79
    MSPROF = 100
