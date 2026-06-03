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

from collections import namedtuple


class TraceViewHeaderConstant:
    """
    trace view header constant class
    """

    TRACE_HEADER_NAME = 'name'
    TRACE_HEADER_PH = 'ph'
    TRACE_HEADER_TS = 'ts'
    TRACE_HEADER_CAT = 'cat'
    TRACE_HEADER_DURATION = 'dur'
    TRACE_HEADER_PID = 'pid'
    TRACE_HEADER_TID = 'tid'
    TRACE_HEADER_BP = 'bp'
    TRACE_HEADER_ID = 'id'
    TRACE_HEADER_ARGS = 'args'
    # column graph format
    COLUMN_GRAPH_HEAD_LEAST = ['name', 'ts', 'pid', 'tid', 'args']  # name, ts, pid, tid, args is required
    # timeline graph format
    TIME_GRAPH_HEAD_LEAST = ['name', 'pid', 'ts', 'dur']  # name, pid, ts, dur is required
    TASK_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur']
    TOP_DOWN_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur', 'args']
    GRPC_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur', 'args', 'cat']

    # don't filter phase
    NOT_FILTER_PHASE = ["M", "s"]

    # meta data head format
    METADATA_HEAD = ["name", "pid", "tid", "args"]

    # pid value when it not set
    DEFAULT_PID_VALUE = 0

    # tid value when it not set
    DEFAULT_TID_VALUE = 0

    # process name
    PROCESS_RUNTIME = "Runtime"
    PROCESS_AI_CORE_UTILIZATION = "AI Core Utilization"
    PROCESS_ACL = "AscendCL"
    PROCESS_AI_CPU = "AI CPU"
    PROCESS_ALL_REDUCE = "All Reduce"
    PROCESS_GE = "GE"
    PROCESS_TASK = "Task Scheduler"
    PROCESS_STEP_TRACE = "Step Trace"
    PROCESS_TRAINING_TRACE = "Training Trace"
    PROCESS_PCIE = "Pcie"
    PROCESS_MSPROFTX = "MsprofTx"
    PROCESS_SUBTASK = 'Subtask Time'
    PROCESS_THREAD_TASK = 'Thread Task Time'
    PROCESS_OVERLAP_ANALYSE = "Overlap Analysis"
    PROCESS_API = "Api"
    PROCESS_EVENT = "Event"
    PROCESS_CPU_USAGE = "CPU Usage"
    PROCESS_MEMORY_USAGE = "Memory Usage"
    PROCESS_NETWORK_USAGE = "Network Usage"
    PROCESS_DISK_USAGE = "Disk Usage"
    PROCESS_OS_RUNTIME_API = "OS Runtime API"
    PROCESS_AI_CORE_FREQ = "AI Core Freq"
    PROCESS_COMMUNICATION = "Communication"
    PROCESS_BLOCK_DETAIL = "Block Detail"
    PROCESS_CCU = "CCU"
    PROCESS_BIU_PERF = "Biu Perf"
    PROCESS_VOLTAGE = "Voltage Info"
    PROCESS_AI_CORE_VOLTAGE = "Aicore Voltage"
    PROCESS_BUS_VOLTAGE = "Bus Voltage"
    PROCESS_DPU = "DPU"
    PROCESS_FUSION_TASK = "Fusion Task"

    # trace general layer
    GENERAL_LAYER_CPU = "CPU"
    GENERAL_LAYER_DPU = "DPU"
    GENERAL_LAYER_NPU = "NPU"

    # trace component layer
    COMPONENT_LAYER_FRAMEWORK = "PID Name"
    COMPONENT_LAYER_CANN = "CANN"
    COMPONENT_LAYER_ASCEND_HW = "Ascend Hardware"
    COMPONENT_LAYER_CPU_USAGE = "CPU Usage"
    COMPONENT_LAYER_MEMORY_USAGE = "Memory Usage"
    COMPONENT_LAYER_NETWORK_USAGE = "Network Usage"
    COMPONENT_LAYER_DISK_USAGE = "Disk Usage"
    COMPONENT_LAYER_OS_RUNTIME_API = "OS Runtime API"
    COMPONENT_LAYER_AICORE_FREQ = "AI Core Freq"
    COMPONENT_LAYER_HCCL = "Communication"
    COMPONENT_LAYER_VOLTAGE = "Voltage Info"
    COMPONENT_LAYER_DPU = "DPU"
    COMPONENT_LAYER_FUSION_TASK = "Fusion Task"

    # filtering msprof timeline trace
    MSPROF_TIMELINE_FILTER_LIST = (PROCESS_ALL_REDUCE, PROCESS_AI_CPU)

    # component_layer_sort
    LAYER_FRAMEWORK_SORT = 6
    LAYER_CANN_SORT = 7
    LAYER_DPU_SORT = 8
    LAYER_CPU_USAGE_SORT = 9
    LAYER_MEMORY_USAGE_SORT = 10
    LAYER_NETWORK_USAGE_SORT = 11
    LAYER_DISK_USAGE_SORT = 12
    LAYER_OS_RUNTIME_API_SORT = 13
    LAYER_ASCEND_HW_SORT = 14
    LAYER_FUSION_TASK_SORT = 15
    LAYER_ASCEND_AICORE_FREQ_SORT = 16
    LAYER_VOLTAGE_SORT = 17
    LAYER_HCCL_SORT = 18
    DEFAULT_LAYER_SORT_START = 19

    # namedtuple configuration of LayerInfo
    LayerInfo = namedtuple('LayerInfo', ['component_layer', 'general_layer', 'sort_index'])

    # 【msprof.json】 timeline layer info map
    LAYER_INFO_MAP = {
        PROCESS_MSPROFTX: LayerInfo(COMPONENT_LAYER_FRAMEWORK, GENERAL_LAYER_CPU, LAYER_FRAMEWORK_SORT),
        PROCESS_ACL: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_GE: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_RUNTIME: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_TASK: LayerInfo(COMPONENT_LAYER_ASCEND_HW, GENERAL_LAYER_NPU, LAYER_ASCEND_HW_SORT),
        PROCESS_FUSION_TASK: LayerInfo(COMPONENT_LAYER_FUSION_TASK, GENERAL_LAYER_NPU, LAYER_FUSION_TASK_SORT),
        PROCESS_STEP_TRACE: LayerInfo(COMPONENT_LAYER_ASCEND_HW, GENERAL_LAYER_NPU, LAYER_ASCEND_HW_SORT),
        PROCESS_API: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_EVENT: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_CPU_USAGE: LayerInfo(COMPONENT_LAYER_CPU_USAGE, GENERAL_LAYER_CPU, LAYER_CPU_USAGE_SORT),
        PROCESS_MEMORY_USAGE: LayerInfo(COMPONENT_LAYER_MEMORY_USAGE, GENERAL_LAYER_CPU, LAYER_MEMORY_USAGE_SORT),
        PROCESS_NETWORK_USAGE: LayerInfo(COMPONENT_LAYER_NETWORK_USAGE, GENERAL_LAYER_CPU, LAYER_NETWORK_USAGE_SORT),
        PROCESS_DISK_USAGE: LayerInfo(COMPONENT_LAYER_DISK_USAGE, GENERAL_LAYER_CPU, LAYER_DISK_USAGE_SORT),
        PROCESS_OS_RUNTIME_API: LayerInfo(COMPONENT_LAYER_OS_RUNTIME_API, GENERAL_LAYER_CPU, LAYER_OS_RUNTIME_API_SORT),
        PROCESS_AI_CORE_FREQ: LayerInfo(COMPONENT_LAYER_AICORE_FREQ, GENERAL_LAYER_NPU, LAYER_ASCEND_AICORE_FREQ_SORT),
        PROCESS_VOLTAGE: LayerInfo(COMPONENT_LAYER_VOLTAGE, GENERAL_LAYER_NPU, LAYER_VOLTAGE_SORT),
        PROCESS_COMMUNICATION: LayerInfo(COMPONENT_LAYER_HCCL, GENERAL_LAYER_NPU, LAYER_HCCL_SORT),
        PROCESS_DPU: LayerInfo(COMPONENT_LAYER_DPU, GENERAL_LAYER_DPU, LAYER_DPU_SORT),
    }

    @classmethod
    def update_layer_info_map(cls: any, process_name: str) -> None:
        """
        update LAYER_INFO_MAP based on process_name
        """
        if process_name is not None and process_name != "":
            cls.COMPONENT_LAYER_FRAMEWORK = process_name
            cls.LAYER_INFO_MAP.update(
                {
                    cls.PROCESS_MSPROFTX: cls.LayerInfo(
                        cls.COMPONENT_LAYER_FRAMEWORK, cls.GENERAL_LAYER_CPU, cls.LAYER_FRAMEWORK_SORT
                    )
                }
            )

    def get_trace_view_header_constant_class_name(self: any) -> any:
        """
        get trace view header constant class name
        """
        return self.__class__.__name__

    def get_trace_view_header_constant_class_member(self: any) -> any:
        """
        get trace view header constant class member num
        """
        return self.__dict__
