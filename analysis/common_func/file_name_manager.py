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

import re
from collections import namedtuple

from common_func.constant import Constant
from common_func.empty_class import EmptyClass


class FileNameManagerConstant:
    """
    file name manager constant class
    """
    HOST_START_PATTERN = r"^host_start\.log(\.\d+)?"

    DEV_START_PATTERN = r"^dev_start\.log\.\d+"

    INFO_JSON_PATTERN = r"^info\.json"

    INFO_JSON_FOR_DEVICE_PATTERN = r"^info\.json\.\d+$"

    START_INFO_PATTERN = r"^start_info"

    END_INFO_PATTERN = r"^end_info"

    SAMPLE_JSON_PATTERN = r"^sample\.json"

    ACL_FILE_PATTERN = r"^AclModule\.(acl_model|acl_op|acl_rts|acl_others)\.(\d+)\.slice_\d+"
    ACL_INFER_FILE_PATTERN = r"^AclModule\.(acl_model|acl_op|acl_rts|acl_others)\.(\d+)\.\d+"

    ACL_HASH_FILE_PATTERN = r"^AclModule\.hash_dic\.(\d+)\.slice_\d+"

    AI_CORE_FILE_PATTERN = r"^aicore\.data\.(\d+)\.slice_\d+"
    AI_CORE_INFER_FILE_PATTERN = r"^aicore\.data\.\d+\.(\d+)\.\d+"
    AI_CORE_TRAINING_FILE_PATTERN = r"^aicore\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    AIV_FILE_PATTERN = r"^aiVectorCore\.data\.(\d+)\.slice_\d+"
    AIV_INFER_FILE_PATTERN = r"^aiVectorCore\.data\.\d+\.(\d+)\.\d+"

    AI_CPU_FILE_PATTERN = r"^ai_ctrl_cpu\.data\.(\d+)\.slice_\d+"
    AI_CPU_INFER_FILE_PATTERN = r"^ctrlcpu\.data\.txt\.(\d+)\.\d+"
    AI_CPU_TRAINING_FILE_PATTERN = r"ctrlcpu.data\.txt\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    CTRL_CPU_FILE_PATTERN = r"^ai_ctrl_cpu\.data\.(\d+)\.slice_\d+"
    CTRL_CPU_INFER_FILE_PATTERN = r"^ctrlcpu\.data\.txt\.(\d+)\.\d+"
    CTRL_CPU_TRAINING_FILE_PATTERN = r"ctrlcpu.data\.txt\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    DATA_PREPROCESS_FILE_PATTERN = r"^DATA_PREPROCESS\.{}\.(\d+)\.slice_\d+"
    DATA_PREPROCESS_TRAINING_FILE_PATTERN = r"^DATA_PREPROCESS\.dev\.{}\.(\d+)\.slice_\d+"

    DATA_PREPARATION_DEVICE_QUEUE = r"^Framework\.device_queue\.(\d+)\.slice_\d+"
    DATA_PREPARATION_DATASET_ITERATION = r"^Framework\.dataset_iterator\.(\d+)\.slice_\d+"

    DDR_FILE_PATTERN = r"^ddr\.data\.(\d+)\.slice_\d+"
    DDR_INFER_FILE_PATTERN = r"^ddr\.data\.\d+\.(\d+)\.\d+"
    DDR_TRAINING_FILE_PATTERN = r"^ddr\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    DVPP_FILE_PATTERN = r"^dvpp\.data\.(\d+)\.slice_\d+"
    DVPP_INFER_FILE_PATTERN = r"^dvpp\.data\.\d+\.(\d+)\.\d+"
    DVPP_TRAINING_FILE_PATTERN = r"^dvpp\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    GE_TASK_FILE_PATTERN = r"^Framework\.task_desc_info\.(\d+)\.slice_\d+"
    GE_TASK_SINGLE_FILE_PATTERN = r"^Framework\.single_op_task_info\.(\d+)\.slice_\d+"
    GE_TASK_INFER_FILE_PATTERN = r"^Framework\.task_desc_info\.(\d+)\.\d+"
    GE_TASK_TRAINING_FILE_PATTERN = r"^Framework\.host\.task_desc_info\.(\d+)\.slice_\d+"

    GE_STEP_INFO_PATTERN = r"^Framework\.step_info\.(\d+)\.slice_\d+"

    GE_TENSOR_INFO_PATTERN = r"^Framework\.tensor_data_info\.(\d+)\.slice_\d+"
    GE_TENSOR_INFO_SINGLE_PATTERN = r"^Framework\.single_op_tensor_info\.(\d+)\.slice_\d+"

    GE_SESSION_INFO_PATTERN = r"^Framework\.id_map_info\.(\d+)\.slice_\d+"

    GE_HASH_PATTERN = r"^Framework\.hash_dic\.(\d+)\.slice_\d+"

    GE_MODEL_LOAD_FILE_PATTERN = r"^Framework\.model_load_info_(\d+)\.(\d+)\.slice_\d+"
    GE_MODEL_LOAD_INFER_FILE_PATTERN = r"^Framework\.model_load_info_(\d+).(\d+).\d+"
    GE_MODEL_LOAD_TRAINING_FILE_PATTERN = r"^Framework\.host.model_load_info_(\d+)\.(\d+)\.slice_\d+"

    GE_MODEL_TIME_FILE_PATTERN = r"^Framework\.model_time_info_(\d+)_(\d+)\.(\d+)\.slice_\d+"
    GE_MODEL_TIME_INFER_FILE_PATTERN = r"^Framework\.model_time_info_(\d+)_(\d+).(\d+).\d+"
    GE_MODEL_TIME_TRAINING_FILE_PATTERN = \
        r"^Framework.host.model_time_info_(\d+)_(\d+)\.(\d+)\.slice_\d+"

    GE_FUSION_INFO_PATTERN = r"^Framework\.model_time_info_(\d+)_(\d+)\.(\d+)\.slice_\d+"

    GE_FUSION_OP_PATTERN = r"^Framework\.fusion_op_info_(\d+)\.(\d+)\.slice_\d+"
    GE_HOST_SCH_FILE_PATTERN = r"^Framework\.dynamic_op_execute\.(\d+)\.slice_\d+"

    HBM_FILE_PATTERN = r"^hbm\.data\.(\d+)\.slice_\d+"
    HBM_TRAINING_FILE_PATTERN = r"hbm\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    HCCL_FILE_PATTERN = r"^HCCL\.hcom_all_reduce"
    HCCL_TRAINING_FILE_PATTERN = r"^HCCL\.host\.hcom_all_reduce"

    HCCS_FILE_PATTERN = r"^hccs\.data\.(\d+)\.slice_\d+"
    HCCS_TRAINING_FILE_PATTERN = r"^hccs\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    HWTS_FILE_PATTERN = r"^hwts\.data\.(\d+)\.slice_\d+"
    HWTS_INFER_FILE_PATTERN = r"^hwts\.log\.data\.\d+\.(\d+)\.\d+"
    HWTS_TRAINING_FILE_PATTERN = \
        r"hwts\.log\.data\.\d+\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    HWTS_VECTOR_FILE_PATTERN = r"^hwts\.aiv_data\.(\d+)\.slice_\d+"
    HWTS_VECTOR_INFER_FILE_PATTERN = r"^hwts\.log\.aiv\.data\.\d+\.(\d+)\.\d+"

    L2_CACHE_FILE_PATTERN = r"^l2_cache\.data\.(\d+)\.slice_\d+"
    L2_CACHE_INFER_FILE_PATTERN = r"l2_cache\.data\.d+\.(d+)\.d+"
    L2_CACHE_TRAINING_FILE_PATTERN = \
        r"l2_cache\.data\.\d+\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    LLC_FILE_PATTERN = r"^llc\.data\.(\d+)\.slice_\d+"
    LLC_INFER_FILE_PATTERN = r"^llc\.data\.(\d+)\.\d+"
    LLC_TRAINING_FILE_PATTERN = r"^llc\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    NIC_FILE_PATTERN = r"^nic\.data\.(\d+)\.slice_\d+"
    NIC_TRAINING_FILE_PATTERN = \
        r"^nic\.data\.\d+\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    PCIE_FILE_PATTERN = r"^pcie\.data\.(\d+)\.slice_\d+"
    PCIE_TRAINING_FILE_PATTERN = \
        r"^pcie\.data\.\d+\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    PID_MEM_FILE_PATTERN = r"^(\d+)-Memory\.data\.(\d+)\.slice_\d+"
    HOST_PID_MEM_FILE_PATTERN = r"^(\d+)-Memory\.data\.slice_\d+"
    PID_MEM_INFER_FILE_PATTERN = r"^(\d+)-Memory\.data\.(\d+)\.\d+"
    PID_MEM_TRAINING_FILE_PATTERN = r"^(\d+)-Memory\.data\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    PID_CPU_USAGE_FILE_PATTERN = r"^(\d+)-CpuUsage\.data\.(\d+)\.slice_\d+"
    HOST_PID_CPU_USAGE_FILE_PATTERN = r"^(\d+)-CpuUsage\.data\.slice_\d+"
    PID_CPU_USAGE_INFER_FILE_PATTERN = r"^(\d+)-CpuUsage\.data\.(\d+)\.\d+"
    PID_CPU_USAGE_TRAINING_FILE_PATTERN = \
        r"(\d+)-CpuUsage\.data\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    ROCE_FILE_PATTERN = r"^roce\.data\.(\d+)\.slice_\d+"
    ROCE_TRAINING_FILE_PATTERN = \
        r"roce\.data\.(\d+)\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    RUNTIME_TASK_TRACK_FILE_PATTERN = r"^[r|R]untime\.task_track\.(\d+)\.slice_\d+"
    RUNTIME_TASK_TRACK_INFER_FILE_PATTERN = r"[r|R]untime\.runtime\.(\d+)\.\d+"
    RUNTIME_TASK_TRACK_TRAINING_FILE_PATTERN = r"^[r|R]untime\.host\.runtime\.(\d+)\.slice_\d+"
    TASK_TRACK_COMPACT_FILE_PATTERN = r"^(aging|unaging)\.compact\.task_track\.slice_\d+"

    RUNTIME_API_FILE_PATTERN = r"^[r|R]untime\.api\.(\d+)\.slice_\d+"
    RUNTIME_API_INFER_FILE_PATTERN = r"^[r|R]untime\.runtime\.data\.(\d+)\.\d+"
    MEMCPY_INFO_COMPACT_FILE_PATTERN = r"^(aging|unaging)\.compact\.memcpy_info\.slice_\d+"

    SYS_MEM_FILE_PATTERN = r"^Memory\.data\.(\d+)\.slice_\d+"
    HOST_SYS_MEM_FILE_PATTERN = r"^host_sys_mem\.data\.slice_\d+"
    SYS_MEM_INFER_FILE_PATTERN = r"^Memory\.data\.(\d+)\.\d+"
    SYS_MEM_TRAINING_FILE_PATTERN = \
        r"^Memory\.data\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    SYS_CPU_USAGE_FILE_PATTERN = r"^SystemCpuUsage\.data\.(\d+)\.slice_\d+"
    HOST_SYS_CPU_USAGE_FILE_PATTERN = r"^host_sys_cpu\.data\.slice_\d+"
    SYS_CPU_USAGE_INFER_FILE_PATTERN = r"SystemCpuUsage\.data\.(\d+)\.\d+"
    SYS_CPU_USAGE_TRAINING_FILE_PATTERN = \
        r"^SystemCpuUsage\.data\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    TS_CPU_FILE_PATTERN = r"^tscpu\.data\.(\d+)\.slice_\d+"
    TS_CPU_INFER_FILE_PATTERN = r"^tscpu\.data\.\d+\.(\d+)\.\d+"
    TS_CPU_TRAINING_FILE_PATTERN = \
        r"^tscpu\.data\.(\d+)\.(dev|host)\.profiler_default_tag\.(\d+)\.slice_\d+"

    TS_TRACK_FILE_PATTERN = r"^ts_track\.data\.(\d+)\.slice_\d+"
    TS_TRACK_INFER_FILE_PATTERN = r"ts_track\.data\.\d+\.(\d+)\.\d+"
    TS_TRACK_TRAINING_FILE_PATTERN = \
        r"ts_track\.data\.\d+\.dev\.profiler_default_tag\.(\d+)\.slice_\d+"

    TS_TRACK_AIV_FILE_PATTERN = r"^ts_track\.aiv_data\.(\d+)\.slice_\d+"
    TS_TRACK_AIV_INFER_FILE_PATTERN = r"^ts_track\.aiv\.data\.\d+\.(\d+)\.\d+"

    MATCHED_DEV_ID_INX = -1

    HOST_CPU_USAGE_PATTERN = r"^host_cpu\.data\.slice_\d+"
    HOST_MEM_USAGE_PATTERN = r"^host_mem\.data\.slice_\d+"
    HOST_DISK_USAGE_PATTERN = r"^host_disk\.data\.slice_\d+"
    HOST_SYS_CALL_PATTERN = r"^host_syscall\.data\.slice_\d+"
    HOST_PTHREAD_CALL_PATTERN = r"^host_pthreadcall\.data\.slice_\d+"
    HOST_NETWORK_USAGE_PATTERN = r"^host_network\.data\.slice_\d+"
    HOST_PLATFORM_PATTERN = r"^host_platform_uncore\.bin"

    SOC_LOG_FILE_PATTERN = r"^stars_soc\.data\.(\d+)\.slice_\d+"
    SOC_PROFILE_FILE_PATTERN = r"^stars_soc_profile\.data\.(\d+)\.slice_\d+"
    BLOCK_LOG_FILE_PATTERN = r"^stars_block\.data\.(\d+)\.slice_\d+"
    FFTS_PMU_FILE_PATTERN = r"^ffts_profile\.data\.(\d+)\.slice_\d+"
    HCCL_HCOM_FILE_PATTERN = r"^HCCL\.hcom_([0-9a-zA-Z]+)_(\d+)_(\d+)_(\d+)\.(\d+)\.slice_\d+"

    ALL_FILE_TAG = "all_file.complete"

    MSPROFTX_FILE_PATTERN = r"^(unaging|aging)\.additional\.msproftx\.slice_\d+"
    MSPROF_JSON_FILE_PATTERN = r"^msprof(_\d+)?(_\d+)?(_\d+)?(_\d+)?(_slice_\d+)?.json"
    MSPROFTX_JSON_FILE_PATTERN = r"^msprof_tx_?\d?_?\d?.json"
    MSPROF_JSON_WITHOUT_SLICE_PATTERN = r"^msprof_(\d+)\.json"

    # helper
    MODEL_WITH_Q_FILE_PATTERN = r"^DATA_PREPROCESS\.AICPU_MODEL\.(\d+)\.slice_\d+"

    # biu perf
    BIU_PERF_FILE_PATTERN = r"^instr\.group_\d+_(aic|aiv0|aiv1).\d+.slice_\d+"
    BIU_PERF_FILE_PATTERN_CHIP6 = r"^instr\.biu_perf_group\d+_(aic|aiv0|aiv1)\.\d+.slice_\d+"


    # parallel
    PARALLEL_STRATEGY_FILE_PATTERN = r"^Framework\.parallel_strategy\.(\d+)\.slice_\d+"

    # npu mem
    NPU_MEM_FILE_PATTERN = r"^npu_mem\.data\.(\d+)\.slice_\d+"
    NPU_APP_MEM_FILE_PATTERN = r"^npu_mem\.app\.(\d+)\.slice_\d+"
    NPU_MODULE_MEM_FILE_PATTERN = r"^npu_module_mem\.data\.(\d+)\.slice_\d+"
    NPU_OP_MEM_FILE_PATTERN = r"^(aging|unaging)\.additional\.task_memory_info\.slice_\d+"

    # freq
    FREQ_FILE_PATTERN = r"lpmFreqConv\.data\.(\d+)\.slice_\d+"

    # lpm info
    LPM_INFO_FILE_PATTERN = r"lpmInfoConv\.data\.(\d+)\.slice_\d+"

    # qos
    QOS_DATA_PATTERN = r"^qos\.data\.(\d+)\.slice_\d+"

    # new data struct
    API_EVENT_FILE_PATTERN = r"^(aging|unaging)\.api_event\.data\.slice_\d+"
    HASH_DATA_FILE_PATTERN = r"^(aging|unaging)\.additional\.(hash_dic|type_info_dic)\.slice_\d+"
    HCCL_INFO_FILE_PATTERN = r"^(unaging|aging)\.additional\.hccl_info\.slice_\d+"
    MULTI_THREAD_FILE_PATTERN = r"^(unaging|aging)\.additional\.Multi_Thread\.slice_\d+"
    TENSOR_ADD_INFO_FILE_PATTERN = r"^(unaging|aging)\.additional\.tensor_info\.slice_\d+"
    NODE_BASIC_INFO_FILE_PATTERN = r"^(unaging|aging)\.compact\.node_basic_info\.slice_\d+"
    NODE_ATTR_INFO_FILE_PATTERN = r"^(unaging|aging)\.compact\.node_attr_info\.slice_\d+"
    GRAPH_ADD_INFO_FILE_PATTERN = r"^(unaging|aging)\.additional\.graph_id_map\.slice_\d+"
    FUSION_ADD_INFO_PATTERN = r"^(unaging|aging)\.additional\.fusion_op_info\.slice_\d+"
    MEMORY_APPLICATION_FILE_PATTERN = r"^(unaging|aging)\.additional\.memory_application\.slice_\d+"
    STATIC_OP_MEM_FILE_PATTERN = r"^(unaging|aging)\.additional\.static_op_mem\.slice_\d+"
    CTX_ID_FILE_PATTERN = r"^(unaging|aging)\.additional\.context_id_info\.slice_\d+"
    GE_LOGIC_STREAM_INFO_PATTERN = r"^(unaging|aging)\.additional\.logic_stream_info\.slice_\d+"
    AICPU_FILE_PATTERN = r"^aicpu\.data\.(\d+)\.slice_\d+"
    HCCL_OP_INFO_FILE_PATTERN = r"^(unaging|aging)\.compact\.hccl_op_info\.slice_\d+"

    # dpu
    DPU_TASK_TRACK_FILE_PATTERN = r"^(aging|unaging)\.compact\.dpu_track\.slice_\d+"
    DPU_HCCL_TRACK_FILE_PATTERN = r"^(aging|unaging)\.additional\.dpu_hccl_info\.slice_\d+"

    # V5
    V5_MODEL_EXEOM_PATTERN = r"^unaging\.additional\.model_exeom\.slice_\d+"
    V5_STARS_PROFILE_PATTERN = r"^nano_stars_profile\.data.\d+\.slice_\d+"

    # dbg
    DBG_PATTERN = r".+\.dbg$"

    # mc2
    MC2_COMM_INFO_PATTERN = r"^(unaging|aging)\.additional\.mc2_comm_info\.slice_\d+"

    # capture stream info
    CAPTURE_STREAM_INFO_PATTERN = r"^(unaging|aging)\.compact\.capture_stream_info\.slice_\d+"

    # runtime op info
    RUNTIME_OP_INFO_PATTERN = r"^(unaging|aging)\.additional\.capture_op_info\.slice_\d+"
    RUNTIME_OP_INFO_VAR_PATTERN = r"^unaging\.variable\.capture_op_info\.slice_\d+"

    # netdev stats
    NETDEV_STATS_FILE_PATTERN = r"^netdev_stats\.data\.(\d+)\.slice_\d+"

    # ub
    UB_DATA_PATTERN = r"^ub\.data\.(\d+)\.slice_\d+"

    # ccu
    # ccu(0|1) means die0/die1
    CCU_MISSION_PATTERN = r"^ccu(0|1)\.instr\.(\d+)\.slice_\d+"
    CCU_CHANNEL_PATTERN = r"^ccu(0|1)\.stat\.(\d+)\.slice_\d+"
    CCU_TASK_INFO_PATTERN = r"^(unaging|aging)\.additional\.ccu_task_info\.slice_\d+"
    CCU_GROUP_INFO_PATTERN = r"^(unaging|aging)\.additional\.ccu_group_info\.slice_\d+"
    CCU_WAIT_SIGNAL_INFO_PATTERN = r"^(unaging|aging)\.additional\.ccu_wait_signal_info\.slice_\d+"

    # soc pmu
    SOC_PMU_PATTEN = r"^socpmu\.data\.(\d+)\.slice_\d+"

    # stream expand spec info
    STREAM_EXPAND_SPEC_INFO_PATTERN = r"^(aging|unaging)\.compact\.expand_stream_spec\.slice_\d+"

    def get_file_name_manager_class_name(self: any) -> any:
        """
        get file name manager class name
        """
        return self.__class__.__name__

    def get_file_name_manager_class_member(self: any) -> any:
        """
        get file name manager class member num
        """
        return self.__dict__


AiCorePattern = namedtuple('AiCorePattern', ['ai_core', 'ai_core_infer', 'ai_core_training', 'ffts_pmu'])


def get_file_name_pattern_match(file_name: str, *file_pattern_compiles: any) -> any:
    """
    get file name pattern match
    """
    if not isinstance(file_name, str):
        return EmptyClass("not original data")
    tags = [Constant.DONE_TAG, Constant.COMPLETE_TAG, Constant.ZIP_TAG]

    def check_file_name(tag: str):
        return file_name.endswith(tag)

    if any(map(check_file_name, tags)):
        return EmptyClass("not original data")
    for file_pattern_compile in file_pattern_compiles:
        match_res = file_pattern_compile.match(file_name)
        if match_res:
            return match_res
    return EmptyClass("not matchable data")


def get_zip_file_pattern_match(file_name: str, *file_pattern_compiles: any) -> any:
    """
    get zip file pattern match
    """
    if not (isinstance(file_name, str) and file_name.endswith(Constant.ZIP_TAG)):
        return EmptyClass("not original data")
    for file_pattern_compile in file_pattern_compiles:
        match_res = file_pattern_compile.match(file_name)
        if match_res:
            return match_res
    return EmptyClass("not matchable data")


def get_start_info_compiles() -> tuple:
    """
    get the start_info regx compiles
    """
    return (re.compile(FileNameManagerConstant.START_INFO_PATTERN),)


def get_end_info_compiles() -> tuple:
    """
    get the end_info regx compiles
    """
    return (re.compile(FileNameManagerConstant.END_INFO_PATTERN),)


def get_info_json_compiles(device_info_only: bool = False) -> tuple:
    """
    get info json regex compiles
    """
    if device_info_only:
        return (re.compile(FileNameManagerConstant.INFO_JSON_FOR_DEVICE_PATTERN),)
    return (re.compile(FileNameManagerConstant.INFO_JSON_PATTERN),)


def get_host_start_compiles() -> tuple:
    """
    get host start regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_START_PATTERN),)


def get_host_stream_expand_spec_info_compiles() -> tuple:
    """
    get host start regex compiles
    """
    return (re.compile(FileNameManagerConstant.STREAM_EXPAND_SPEC_INFO_PATTERN),)


def get_sample_json_compiles() -> tuple:
    """
    get sample json regex compiles
    """
    return (re.compile(FileNameManagerConstant.SAMPLE_JSON_PATTERN),)


def get_dev_start_compiles() -> tuple:
    """
    get host start regex compiles
    """
    return (re.compile(FileNameManagerConstant.DEV_START_PATTERN),)


def get_acl_compiles() -> tuple:
    """
    get acl regex compiles
    """
    return re.compile(FileNameManagerConstant.ACL_FILE_PATTERN), re.compile(
        FileNameManagerConstant.ACL_INFER_FILE_PATTERN)


def get_acl_hash_compiles() -> tuple:
    """
    get acl hash dict regex compiles
    """
    return (re.compile(FileNameManagerConstant.ACL_HASH_FILE_PATTERN),)


def get_ai_core_compiles() -> tuple:
    """
    get ai core regex compiles
    """
    ai_core_compiles = AiCorePattern(re.compile(FileNameManagerConstant.AI_CORE_FILE_PATTERN),
                                     re.compile(FileNameManagerConstant.AI_CORE_INFER_FILE_PATTERN),
                                     re.compile(FileNameManagerConstant.AI_CORE_TRAINING_FILE_PATTERN),
                                     re.compile(FileNameManagerConstant.FFTS_PMU_FILE_PATTERN))
    return ai_core_compiles


def get_aiv_compiles() -> tuple:
    """
    get ai vector core regex compiles
    """
    return re.compile(FileNameManagerConstant.AIV_FILE_PATTERN), re.compile(
        FileNameManagerConstant.AIV_INFER_FILE_PATTERN)


def get_ai_cpu_compiles() -> tuple:
    """
    get ai cpu regex compiles
    """
    return re.compile(FileNameManagerConstant.AI_CPU_FILE_PATTERN), re.compile(
        FileNameManagerConstant.AI_CPU_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.AI_CPU_TRAINING_FILE_PATTERN)


def get_ctrl_cpu_compiles() -> tuple:
    """
    get ctrl cpu compiles
    """
    return re.compile(FileNameManagerConstant.CTRL_CPU_FILE_PATTERN), re.compile(
        FileNameManagerConstant.CTRL_CPU_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.CTRL_CPU_TRAINING_FILE_PATTERN)


def get_ddr_compiles() -> tuple:
    """
    get ddr regex compiles
    """
    return re.compile(FileNameManagerConstant.DDR_FILE_PATTERN), re.compile(
        FileNameManagerConstant.DDR_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.DDR_TRAINING_FILE_PATTERN)


def get_data_preprocess_compiles(tag: str) -> tuple:
    """
    get dvpp regex compiles
    """
    return re.compile(FileNameManagerConstant.DATA_PREPROCESS_FILE_PATTERN.format(tag)), re.compile(
        FileNameManagerConstant.DATA_PREPROCESS_TRAINING_FILE_PATTERN.format(tag))


def get_host_queue_compiles() -> tuple:
    """
    get host queue compiles
    """
    return re.compile(FileNameManagerConstant.DATA_PREPARATION_DEVICE_QUEUE), re.compile(
        FileNameManagerConstant.DATA_PREPARATION_DATASET_ITERATION)


def get_dvpp_compiles() -> tuple:
    """
    get dvpp regex compiles
    """
    return re.compile(FileNameManagerConstant.DVPP_FILE_PATTERN), re.compile(
        FileNameManagerConstant.DVPP_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.DVPP_TRAINING_FILE_PATTERN)


def get_ge_task_compiles() -> tuple:
    """
    get ge task regex compiles
    """
    ge_task_compiles = (
        re.compile(FileNameManagerConstant.GE_TASK_FILE_PATTERN),
        re.compile(FileNameManagerConstant.GE_TASK_SINGLE_FILE_PATTERN),
        re.compile(FileNameManagerConstant.GE_TASK_INFER_FILE_PATTERN),
        re.compile(FileNameManagerConstant.GE_TASK_TRAINING_FILE_PATTERN)
    )
    return ge_task_compiles


def get_ge_step_info_compiles() -> tuple:
    """
    get ge step info compiles
    """
    return (re.compile(FileNameManagerConstant.GE_STEP_INFO_PATTERN),)


def get_ge_session_info_compiles() -> tuple:
    """
    get ge session info compiles
    """
    return (re.compile(FileNameManagerConstant.GE_SESSION_INFO_PATTERN),)


def get_ge_tensor_info_compiles() -> tuple:
    """
    get ge tensor info compiles
    """
    return re.compile(FileNameManagerConstant.GE_TENSOR_INFO_PATTERN), \
        re.compile(FileNameManagerConstant.GE_TENSOR_INFO_SINGLE_PATTERN)


def get_ge_hash_compiles() -> tuple:
    """
    get ge hash compiles
    """
    return (re.compile(FileNameManagerConstant.GE_HASH_PATTERN),)


def get_ge_model_load_compiles() -> tuple:
    """
    get ge model load compiles
    """
    return re.compile(FileNameManagerConstant.GE_MODEL_LOAD_FILE_PATTERN), re.compile(
        FileNameManagerConstant.GE_MODEL_LOAD_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.GE_MODEL_LOAD_TRAINING_FILE_PATTERN)


def get_ge_model_time_compiles() -> tuple:
    """
    get ge model time compiles
    """
    return re.compile(FileNameManagerConstant.GE_MODEL_TIME_FILE_PATTERN), re.compile(
        FileNameManagerConstant.GE_MODEL_TIME_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.GE_MODEL_TIME_TRAINING_FILE_PATTERN)


def get_ge_fusion_op_compiles() -> tuple:
    """
    get ge fusion op compiles
    """
    return (re.compile(FileNameManagerConstant.GE_FUSION_OP_PATTERN),)


def get_ge_host_compiles() -> tuple:
    """
    get ge host compiles
    """
    return (re.compile(FileNameManagerConstant.GE_HOST_SCH_FILE_PATTERN),)


def get_ge_logic_stream_info_compiles() -> tuple:
    """
    get ge logic stream info files regex compiles
    :return: api data files regex
    """
    return (re.compile(FileNameManagerConstant.GE_LOGIC_STREAM_INFO_PATTERN),)


def get_hbm_compiles() -> tuple:
    """
    get hbm regex compiles
    """
    return re.compile(FileNameManagerConstant.HBM_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HBM_TRAINING_FILE_PATTERN)


def get_hccl_compiles() -> tuple:
    """
    get hccl regex compiles
    """
    return re.compile(FileNameManagerConstant.HCCL_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HCCL_TRAINING_FILE_PATTERN)


def get_hccs_compiles() -> tuple:
    """
    get hccs regex compiles
    """
    return re.compile(FileNameManagerConstant.HCCS_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HCCS_TRAINING_FILE_PATTERN)


def get_hwts_compiles() -> tuple:
    """
    get hwts regex compiles
    """
    return re.compile(FileNameManagerConstant.HWTS_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HWTS_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HWTS_TRAINING_FILE_PATTERN)


def get_hwts_vector_compiles() -> tuple:
    """
    get hwts vector regex compiles
    """
    return re.compile(FileNameManagerConstant.HWTS_VECTOR_FILE_PATTERN), re.compile(
        FileNameManagerConstant.HWTS_VECTOR_INFER_FILE_PATTERN)


def get_l2_cache_compiles() -> tuple:
    """
    get l2 cache regex compiles
    """
    return re.compile(FileNameManagerConstant.L2_CACHE_FILE_PATTERN), re.compile(
        FileNameManagerConstant.L2_CACHE_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.L2_CACHE_TRAINING_FILE_PATTERN)


def get_llc_compiles() -> tuple:
    """
    get llc regex compiles
    """
    return re.compile(FileNameManagerConstant.LLC_FILE_PATTERN), re.compile(
        FileNameManagerConstant.LLC_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.LLC_TRAINING_FILE_PATTERN)


def get_nic_compiles() -> tuple:
    """
    get nic regex compiles
    """
    return re.compile(FileNameManagerConstant.NIC_FILE_PATTERN), re.compile(
        FileNameManagerConstant.NIC_TRAINING_FILE_PATTERN)


def get_pcie_compiles() -> tuple:
    """
    get pcie regex compiles
    """
    return re.compile(FileNameManagerConstant.PCIE_FILE_PATTERN), re.compile(
        FileNameManagerConstant.PCIE_TRAINING_FILE_PATTERN)


def get_pid_mem_compiles() -> tuple:
    """
    get pid memory regex compiles
    """
    pid_mem_compiles = (
        re.compile(FileNameManagerConstant.PID_MEM_FILE_PATTERN),
        re.compile(FileNameManagerConstant.PID_MEM_INFER_FILE_PATTERN),
        re.compile(FileNameManagerConstant.PID_MEM_TRAINING_FILE_PATTERN),
        re.compile(FileNameManagerConstant.HOST_PID_MEM_FILE_PATTERN)
    )
    return pid_mem_compiles


def get_pid_cpu_usage_compiles() -> tuple:
    """
    get pid cpu usage regex compiles
    """
    pid_cpu_usage_compiles = (
        re.compile(FileNameManagerConstant.PID_CPU_USAGE_FILE_PATTERN),
        re.compile(FileNameManagerConstant.PID_CPU_USAGE_INFER_FILE_PATTERN),
        re.compile(FileNameManagerConstant.PID_CPU_USAGE_TRAINING_FILE_PATTERN),
        re.compile(FileNameManagerConstant.HOST_PID_CPU_USAGE_FILE_PATTERN)
    )
    return pid_cpu_usage_compiles


def get_roce_compiles() -> tuple:
    """
    get roce regex compiles
    """
    return re.compile(FileNameManagerConstant.ROCE_FILE_PATTERN), re.compile(
        FileNameManagerConstant.ROCE_TRAINING_FILE_PATTERN)


def get_runtime_task_track_compiles() -> tuple:
    """
    get runtime task track regex compiles
    """
    return re.compile(FileNameManagerConstant.RUNTIME_TASK_TRACK_FILE_PATTERN), re.compile(
        FileNameManagerConstant.RUNTIME_TASK_TRACK_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.RUNTIME_TASK_TRACK_TRAINING_FILE_PATTERN)


def get_runtime_api_compiles() -> tuple:
    """
    get runtime api regex compiles
    """
    return re.compile(FileNameManagerConstant.RUNTIME_API_FILE_PATTERN), re.compile(
        FileNameManagerConstant.RUNTIME_API_INFER_FILE_PATTERN)


def get_sys_mem_compiles() -> tuple:
    """
    get sys mem regex compiles
    """
    sys_mem_compiles = (
        re.compile(FileNameManagerConstant.SYS_MEM_FILE_PATTERN),
        re.compile(FileNameManagerConstant.SYS_MEM_INFER_FILE_PATTERN),
        re.compile(FileNameManagerConstant.SYS_MEM_TRAINING_FILE_PATTERN),
        re.compile(FileNameManagerConstant.HOST_SYS_MEM_FILE_PATTERN)
    )
    return sys_mem_compiles


def get_sys_cpu_usage_compiles() -> tuple:
    """
    get sys cpu usage compiles
    """
    sys_cpu_usage_compiles = (
        re.compile(FileNameManagerConstant.SYS_CPU_USAGE_FILE_PATTERN),
        re.compile(FileNameManagerConstant.SYS_CPU_USAGE_INFER_FILE_PATTERN),
        re.compile(FileNameManagerConstant.SYS_CPU_USAGE_TRAINING_FILE_PATTERN),
        re.compile(FileNameManagerConstant.HOST_SYS_CPU_USAGE_FILE_PATTERN),
    )
    return sys_cpu_usage_compiles


def get_ts_cpu_compiles() -> tuple:
    """
    get ts cpu regex compiles
    """
    return re.compile(FileNameManagerConstant.TS_CPU_FILE_PATTERN), re.compile(
        FileNameManagerConstant.TS_CPU_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.TS_CPU_TRAINING_FILE_PATTERN)


def get_ts_track_compiles() -> tuple:
    """
    get ts track regex compiles
    """
    return re.compile(FileNameManagerConstant.TS_TRACK_FILE_PATTERN), re.compile(
        FileNameManagerConstant.TS_TRACK_INFER_FILE_PATTERN), re.compile(
        FileNameManagerConstant.TS_TRACK_TRAINING_FILE_PATTERN)


def get_ts_track_aiv_compiles() -> tuple:
    """
    get ts track aiv regex compiles
    """
    return re.compile(FileNameManagerConstant.TS_TRACK_AIV_FILE_PATTERN), re.compile(
        FileNameManagerConstant.TS_TRACK_AIV_INFER_FILE_PATTERN)


def get_host_cpu_usage_compiles() -> tuple:
    """
    get host cpu usage regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_CPU_USAGE_PATTERN),)


def get_host_mem_usage_compiles() -> tuple:
    """
    get host mem usage regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_MEM_USAGE_PATTERN),)


def get_host_disk_usage_compiles() -> tuple:
    """
    get host disk usage regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_DISK_USAGE_PATTERN),)


def get_host_network_usage_compiles() -> tuple:
    """
    get host network usage regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_NETWORK_USAGE_PATTERN),)


def get_host_syscall_compiles() -> tuple:
    """
    get host syscall regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_SYS_CALL_PATTERN),)


def get_host_pthread_call_compiles() -> tuple:
    """
    get host pthread call regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_PTHREAD_CALL_PATTERN),)

def get_host_platform_compiles() -> tuple:
    """
    get host platform regex compiles
    """
    return (re.compile(FileNameManagerConstant.HOST_PLATFORM_PATTERN),)

def get_os_runtime_api_compiles() -> tuple:
    """
    get os runtime api regex compiles
    """
    return re.compile(FileNameManagerConstant.HOST_SYS_CALL_PATTERN), re.compile(
        FileNameManagerConstant.HOST_PTHREAD_CALL_PATTERN)


def get_soc_log_compiles() -> tuple:
    """
    get soc log regex compiles
    :return: soc log compiles
    """
    return (re.compile(FileNameManagerConstant.SOC_LOG_FILE_PATTERN),)


def get_soc_profiler_compiles() -> tuple:
    """
    get stars soc regex compiles
    :return: soc log compiles
    """
    return (re.compile(FileNameManagerConstant.SOC_PROFILE_FILE_PATTERN),)


def get_ffts_pmu_compiles() -> tuple:
    """
    get ffts pmu files regex compiles
    :return: pmu files regex
    """
    return (re.compile(FileNameManagerConstant.FFTS_PMU_FILE_PATTERN),)


def get_hccl_hcom_compiles() -> tuple:
    """
    get hccl files regex compiles
    :return: hccl files regex
    """
    return (re.compile(FileNameManagerConstant.HCCL_HCOM_FILE_PATTERN),)


def get_msproftx_compiles() -> tuple:
    """
    get msproftx files regex compiles
    :return: msproftx files regex
    """
    return (re.compile(FileNameManagerConstant.MSPROFTX_FILE_PATTERN),)


def get_msprof_json_compiles() -> tuple:
    """
    get msprof json files regex compiles
    :return: msprof json files regex
    """
    return (re.compile(FileNameManagerConstant.MSPROF_JSON_FILE_PATTERN),)


def get_msprof_json_without_slice_compiles() -> tuple:
    """
    get msprof json files without slice regex compiles
    :return: msprof json files regex
    """
    return (re.compile(FileNameManagerConstant.MSPROF_JSON_WITHOUT_SLICE_PATTERN),)


def get_msprof_tx_json_compiles() -> tuple:
    """
    get msproftx json files regex compiles
    :return: msproftx json files regex
    """
    return (re.compile(FileNameManagerConstant.MSPROFTX_JSON_FILE_PATTERN),)


def get_helper_model_with_q_compiles() -> tuple:
    """
    get helper model_with_q files regex compiles
    :return: helper model_with_q files regex
    """
    return (re.compile(FileNameManagerConstant.MODEL_WITH_Q_FILE_PATTERN),)


def get_biu_compiles() -> tuple:
    """
    get biu perf files regex compiles
    :return: helper biu perf files regex
    """
    return (re.compile(FileNameManagerConstant.BIU_PERF_FILE_PATTERN),
            re.compile(FileNameManagerConstant.BIU_PERF_FILE_PATTERN_CHIP6))


def get_parallel_strategy_compiles() -> tuple:
    """
    get parallel strategy files regex compiles
    :return: parallel strategy files regex
    """
    return (re.compile(FileNameManagerConstant.PARALLEL_STRATEGY_FILE_PATTERN),)


def get_npu_mem_compiles() -> tuple:
    """
    get npu mem files regex compiles
    :return: npu mem files regex
    """
    return (re.compile(FileNameManagerConstant.NPU_MEM_FILE_PATTERN),
            re.compile(FileNameManagerConstant.NPU_APP_MEM_FILE_PATTERN))


def get_npu_module_mem_compiles() -> tuple:
    """
    get npu module mem files regex compiles
    :return: npu module mem files regex
    """
    return (re.compile(FileNameManagerConstant.NPU_MODULE_MEM_FILE_PATTERN),
            )


def get_npu_op_mem_compiles() -> tuple:
    """
    get npu op mem files regex compiles
    :return: npu op mem files regex
    """
    return (re.compile(FileNameManagerConstant.NPU_OP_MEM_FILE_PATTERN),
            )


def get_freq_compiles() -> tuple:
    """
    get freq files regex compiles
    :return: freq files regex
    """
    return (re.compile(FileNameManagerConstant.FREQ_FILE_PATTERN),
            )


def get_lpm_info_compiles() -> tuple:
    """
    get lpm info && freq files regex compiles
    :return: lpm info && freq files regex
    """
    return (re.compile(FileNameManagerConstant.LPM_INFO_FILE_PATTERN),
            re.compile(FileNameManagerConstant.FREQ_FILE_PATTERN),
            )


def get_api_event_compiles() -> tuple:
    """
    get api and event data files regex compiles
    :return: api and event data files regex
    """
    return (re.compile(FileNameManagerConstant.API_EVENT_FILE_PATTERN),)


def get_hash_data_compiles() -> tuple:
    """
    get hash data  files regex compiles
    :return: hash data files regex
    """
    return (re.compile(FileNameManagerConstant.HASH_DATA_FILE_PATTERN),)


def get_task_track_compact_compiles() -> tuple:
    """
    get runtime task track regex compiles
    """
    return (
        re.compile(FileNameManagerConstant.TASK_TRACK_COMPACT_FILE_PATTERN),
    )


def get_memcpy_info_compact_compiles() -> tuple:
    """
    get runtime memcpy info regex compiles
    """
    return (
        re.compile(FileNameManagerConstant.MEMCPY_INFO_COMPACT_FILE_PATTERN),
    )


def get_hccl_info_compiles() -> tuple:
    """
    get hccl info data files regex compiles
    :return: hccl info data files regex
    """
    return re.compile(FileNameManagerConstant.HCCL_INFO_FILE_PATTERN),


def get_multi_thread_compiles() -> tuple:
    """
    get multiple thread files regex compiles
    :return: multiple thread files regex
    """
    return re.compile(FileNameManagerConstant.MULTI_THREAD_FILE_PATTERN),


def get_ge_graph_add_info_compiles() -> tuple:
    """
    get ge graph add info compiles
    :return: aging ge graph add info files regex
    """
    return (re.compile(FileNameManagerConstant.GRAPH_ADD_INFO_FILE_PATTERN),)


def get_ge_tensor_add_info_compiles() -> tuple:
    """
    get aging ge tensor add info files regex compiles
    :return: aging ge tensor add info files regex
    """
    return (re.compile(FileNameManagerConstant.TENSOR_ADD_INFO_FILE_PATTERN),)


def get_ge_node_basic_info_compiles() -> tuple:
    """
    get ge node basic info files regex compiles
    :return: ge node basic info files regex
    """
    return (re.compile(FileNameManagerConstant.NODE_BASIC_INFO_FILE_PATTERN),)


def get_node_attr_info_compiles() -> tuple:
    """
    get node attr info files regex compiles
    :return: node attr info files regex
    """
    return (re.compile(FileNameManagerConstant.NODE_ATTR_INFO_FILE_PATTERN),)


def get_ge_fusion_add_info_compiles() -> tuple:
    """
    get aging ge fusion add info files regex compiles
    :return: aging ge fusion add info files regex
    """
    return (re.compile(FileNameManagerConstant.FUSION_ADD_INFO_PATTERN),)


def get_ge_memory_application_info_compiles() -> tuple:
    """
    get ge memory application info files regex compiles
    :return: ge memory application info files regex
    """
    return (re.compile(FileNameManagerConstant.MEMORY_APPLICATION_FILE_PATTERN),)


def get_ge_static_op_mem_compiles() -> tuple:
    """
    get ge static op memory files regex compiles
    :return: ge static op memory files regex
    """
    return (re.compile(FileNameManagerConstant.STATIC_OP_MEM_FILE_PATTERN),)


def get_ge_ctx_id_info_compiles() -> tuple:
    """
    get ge ctx id info files regex compiles
    :return: ge ctx id info files regex
    """
    return (re.compile(FileNameManagerConstant.CTX_ID_FILE_PATTERN),)


def get_aicpu_compiles() -> tuple:
    """
    get aicpu files regex compiles
    :return: aicpu files regex
    """
    return (re.compile(FileNameManagerConstant.AICPU_FILE_PATTERN),)


def get_hccl_op_info_compiles() -> tuple:
    """
    get hccl op info data files regex compiles
    :return: hccl op info data files regex
    """
    return (re.compile(FileNameManagerConstant.HCCL_OP_INFO_FILE_PATTERN),)


def get_qos_compiles() -> tuple:
    """
    get qos regex compiles
    :return: qos data files regex
    """
    return (re.compile(FileNameManagerConstant.QOS_DATA_PATTERN),)


def get_mc2_comm_info_compiles() -> tuple:
    """
    get mc2 comm info regex compiles
    :return: mc2 comm info data regex
    """
    return (re.compile(FileNameManagerConstant.MC2_COMM_INFO_PATTERN),)


def get_capture_stream_info_compiles() -> tuple:
    """
    get capture stream info regex compiles
    :return: capture stream info data regex
    """
    return (re.compile(FileNameManagerConstant.CAPTURE_STREAM_INFO_PATTERN),)


def get_netdev_stats_compiles() -> tuple:
    """
    get netdev stats compiles
    """
    return (re.compile(FileNameManagerConstant.NETDEV_STATS_FILE_PATTERN),)


def get_runtime_op_info_compiles() -> tuple:
    """
    get runtime op info regex compiles
    :return: runtime op info data regex
    """
    return (re.compile(FileNameManagerConstant.RUNTIME_OP_INFO_PATTERN),
            re.compile(FileNameManagerConstant.RUNTIME_OP_INFO_VAR_PATTERN))


def get_ub_compiles() -> tuple:
    """
    get ub regex compiles
    :return: ub data files regex
    """
    return (re.compile(FileNameManagerConstant.UB_DATA_PATTERN),)


def get_ccu_mission_compiles() -> tuple:
    """
    get ccu mission regex compiles
    :return: ccu mission data regex
    """
    return (re.compile(FileNameManagerConstant.CCU_MISSION_PATTERN),)


def get_ccu_channel_compiles() -> tuple:
    """
    get ccu channel regex compiles
    :return: ccu channel data regex
    """
    return (re.compile(FileNameManagerConstant.CCU_CHANNEL_PATTERN),)


def get_soc_pmu_compiles() -> tuple:
    """
    get soc pmu regex compiles
    :return: soc pmu data regex
    """
    return (re.compile(FileNameManagerConstant.SOC_PMU_PATTEN),)


def get_ccu_task_info_compiles() -> tuple:
    """
    get ccu task info regex compiles
    :return: ccu task info data regex
    """
    return (re.compile(FileNameManagerConstant. CCU_TASK_INFO_PATTERN),)


def get_ccu_group_info_compiles() -> tuple:
    """
    get ccu group info regex compiles
    :return: ccu group info data regex
    """
    return (re.compile(FileNameManagerConstant. CCU_GROUP_INFO_PATTERN),)


def get_ccu_wait_signal_info_compiles() -> tuple:
    """
    get ccu wait signal info regex compiles
    :return: ccu wait signal info data regex
    """
    return (re.compile(FileNameManagerConstant. CCU_WAIT_SIGNAL_INFO_PATTERN),)


def get_v5_model_exeom_compiles() -> tuple:
    """
    get v5 host info files regex compiles
    :return: v5 host info files regex
    """
    return (re.compile(FileNameManagerConstant.V5_MODEL_EXEOM_PATTERN),)


def get_v5_stars_profile_compiles() -> tuple:
    """
    get v5 device info files regex compiles
    :return: v5 device info files regex
    """
    return (re.compile(FileNameManagerConstant.V5_STARS_PROFILE_PATTERN),)


def get_dbg_file_compiles() -> tuple:
    """
    get v5 host info files regex compiles
    :return: v5 host info files regex
    """
    return (re.compile(FileNameManagerConstant.DBG_PATTERN),)


def get_dpu_track_compact_compiles() -> tuple:
    """
    get dpu task track regex compiles
    """
    return (
        re.compile(FileNameManagerConstant.DPU_TASK_TRACK_FILE_PATTERN),
    )


def get_dpu_hccl_track_compact_compiles() -> tuple:
    """
    get runtime task track regex compiles
    """
    return (
        re.compile(FileNameManagerConstant.DPU_HCCL_TRACK_FILE_PATTERN),
    )
