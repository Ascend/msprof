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

import itertools
import os
import re

from common_func.constant import Constant
from common_func.file_name_manager import get_acl_compiles
from common_func.file_name_manager import get_v5_model_exeom_compiles
from common_func.file_name_manager import get_v5_stars_profile_compiles
from common_func.file_name_manager import get_dbg_file_compiles
from common_func.file_name_manager import get_acl_hash_compiles
from common_func.file_name_manager import get_ai_core_compiles
from common_func.file_name_manager import get_ai_cpu_compiles
from common_func.file_name_manager import get_aicpu_compiles
from common_func.file_name_manager import get_aiv_compiles
from common_func.file_name_manager import get_api_event_compiles
from common_func.file_name_manager import get_biu_compiles
from common_func.file_name_manager import get_capture_stream_info_compiles
from common_func.file_name_manager import get_ctrl_cpu_compiles
from common_func.file_name_manager import get_data_preprocess_compiles
from common_func.file_name_manager import get_ddr_compiles
from common_func.file_name_manager import get_dvpp_compiles
from common_func.file_name_manager import get_ffts_pmu_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_freq_compiles
from common_func.file_name_manager import get_ge_ctx_id_info_compiles
from common_func.file_name_manager import get_ge_fusion_add_info_compiles
from common_func.file_name_manager import get_ge_fusion_op_compiles
from common_func.file_name_manager import get_ge_graph_add_info_compiles
from common_func.file_name_manager import get_ge_hash_compiles
from common_func.file_name_manager import get_ge_host_compiles
from common_func.file_name_manager import get_ge_logic_stream_info_compiles
from common_func.file_name_manager import get_ge_memory_application_info_compiles
from common_func.file_name_manager import get_ge_static_op_mem_compiles
from common_func.file_name_manager import get_ge_model_load_compiles
from common_func.file_name_manager import get_ge_model_time_compiles
from common_func.file_name_manager import get_ge_node_basic_info_compiles
from common_func.file_name_manager import get_ge_session_info_compiles
from common_func.file_name_manager import get_ge_step_info_compiles
from common_func.file_name_manager import get_ge_task_compiles
from common_func.file_name_manager import get_ge_tensor_add_info_compiles
from common_func.file_name_manager import get_ge_tensor_info_compiles
from common_func.file_name_manager import get_hash_data_compiles
from common_func.file_name_manager import get_hbm_compiles
from common_func.file_name_manager import get_hccl_hcom_compiles
from common_func.file_name_manager import get_hccl_info_compiles
from common_func.file_name_manager import get_hccs_compiles
from common_func.file_name_manager import get_helper_model_with_q_compiles
from common_func.file_name_manager import get_host_queue_compiles
from common_func.file_name_manager import get_hwts_compiles
from common_func.file_name_manager import get_hwts_vector_compiles
from common_func.file_name_manager import get_l2_cache_compiles
from common_func.file_name_manager import get_llc_compiles
from common_func.file_name_manager import get_memcpy_info_compact_compiles
from common_func.file_name_manager import get_msproftx_compiles
from common_func.file_name_manager import get_multi_thread_compiles
from common_func.file_name_manager import get_nic_compiles
from common_func.file_name_manager import get_npu_mem_compiles
from common_func.file_name_manager import get_npu_module_mem_compiles
from common_func.file_name_manager import get_npu_op_mem_compiles
from common_func.file_name_manager import get_node_attr_info_compiles
from common_func.file_name_manager import get_parallel_strategy_compiles
from common_func.file_name_manager import get_pcie_compiles
from common_func.file_name_manager import get_pid_cpu_usage_compiles
from common_func.file_name_manager import get_pid_mem_compiles
from common_func.file_name_manager import get_roce_compiles
from common_func.file_name_manager import get_runtime_api_compiles
from common_func.file_name_manager import get_runtime_task_track_compiles
from common_func.file_name_manager import get_soc_log_compiles
from common_func.file_name_manager import get_soc_profiler_compiles
from common_func.file_name_manager import get_sys_cpu_usage_compiles
from common_func.file_name_manager import get_sys_mem_compiles
from common_func.file_name_manager import get_task_track_compact_compiles
from common_func.file_name_manager import get_ts_cpu_compiles
from common_func.file_name_manager import get_ts_track_aiv_compiles
from common_func.file_name_manager import get_ts_track_compiles
from common_func.file_name_manager import get_hccl_op_info_compiles
from common_func.file_name_manager import get_qos_compiles
from common_func.file_name_manager import get_mc2_comm_info_compiles
from common_func.file_name_manager import get_netdev_stats_compiles
from common_func.file_name_manager import get_ub_compiles
from common_func.file_name_manager import get_ccu_mission_compiles
from common_func.file_name_manager import get_ccu_channel_compiles
from common_func.file_name_manager import get_ccu_task_info_compiles
from common_func.file_name_manager import get_ccu_group_info_compiles
from common_func.file_name_manager import get_ccu_wait_signal_info_compiles
from common_func.file_name_manager import get_soc_pmu_compiles
from common_func.file_name_manager import get_lpm_info_compiles
from common_func.file_name_manager import get_host_stream_expand_spec_info_compiles
from common_func.file_name_manager import get_runtime_op_info_compiles
from common_func.file_name_manager import get_dpu_track_compact_compiles
from common_func.file_name_manager import get_dpu_hccl_track_compact_compiles
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from framework.prof_factory_maker import ProfFactoryMaker
from profiling_bean.prof_enum.data_tag import DataTag


class FileDispatch:
    """
    dispatch the different files to the correspond parser
    """

    FILES_FILTER_MAP = {
        DataTag.ACL: get_acl_compiles(),
        DataTag.ACL_HASH: get_acl_hash_compiles(),
        DataTag.GE_TASK: get_ge_task_compiles(),
        DataTag.GE_SESSION: get_ge_session_info_compiles(),
        DataTag.GE_TENSOR: get_ge_tensor_info_compiles(),
        DataTag.GE_STEP: get_ge_step_info_compiles(),
        DataTag.GE_MODEL_TIME: get_ge_model_time_compiles(),
        DataTag.GE_FUSION_OP_INFO: get_ge_fusion_op_compiles(),
        DataTag.GE_MODEL_LOAD: get_ge_model_load_compiles(),
        DataTag.GE_HASH: get_ge_hash_compiles(),
        DataTag.GE_HOST: get_ge_host_compiles(),
        DataTag.GE_LOGIC_STREAM_INFO: get_ge_logic_stream_info_compiles(),
        DataTag.RUNTIME_API: get_runtime_api_compiles(),
        DataTag.RUNTIME_TRACK: get_runtime_task_track_compiles(),
        DataTag.TS_TRACK: get_ts_track_compiles(),
        DataTag.HWTS: get_hwts_compiles(),
        DataTag.AI_CORE: get_ai_core_compiles(),
        DataTag.AI_CPU: get_data_preprocess_compiles(Constant.DATA_PROCESS_AI_CPU),
        DataTag.DATA_PROCESS: get_data_preprocess_compiles(Constant.DATA_PROCESS_DP),
        DataTag.STARS_LOG: get_soc_log_compiles(),
        DataTag.FFTS_PMU: get_ffts_pmu_compiles(),
        DataTag.L2CACHE: get_l2_cache_compiles(),
        DataTag.AIV: get_aiv_compiles(),
        DataTag.DVPP: get_dvpp_compiles(),
        DataTag.NIC: get_nic_compiles(),
        DataTag.ROCE: get_roce_compiles(),
        DataTag.TSCPU: get_ts_cpu_compiles(),
        DataTag.CTRLCPU: get_ctrl_cpu_compiles(),
        DataTag.AICPU: get_ai_cpu_compiles(),
        DataTag.LLC: get_llc_compiles(),
        DataTag.DDR: get_ddr_compiles(),
        DataTag.SYS_MEM: get_sys_mem_compiles(),
        DataTag.PID_MEM: get_pid_mem_compiles(),
        DataTag.SYS_USAGE: get_sys_cpu_usage_compiles(),
        DataTag.PID_USAGE: get_pid_cpu_usage_compiles(),
        DataTag.HBM: get_hbm_compiles(),
        DataTag.HCCS: get_hccs_compiles(),
        DataTag.PCIE: get_pcie_compiles(),
        DataTag.TS_TRACK_AIV: get_ts_track_aiv_compiles(),
        DataTag.HWTS_AIV: get_hwts_vector_compiles(),
        DataTag.HCCL: get_hccl_hcom_compiles(),
        DataTag.BIU_PERF: get_biu_compiles(),
        DataTag.MSPROFTX: get_msproftx_compiles(),
        DataTag.HELPER_MODEL_WITH_Q: get_helper_model_with_q_compiles(),
        DataTag.SOC_PROFILER: get_soc_profiler_compiles(),
        DataTag.DATA_QUEUE: get_data_preprocess_compiles(Constant.DATA_QUEUE),
        DataTag.HOST_QUEUE: get_host_queue_compiles(),
        DataTag.PARALLEL_STRATEGY: get_parallel_strategy_compiles(),
        DataTag.NPU_MEM: get_npu_mem_compiles(),
        DataTag.NPU_MODULE_MEM: get_npu_module_mem_compiles(),
        DataTag.MEMORY_OP: get_npu_op_mem_compiles(),
        DataTag.FREQ: get_freq_compiles(),
        DataTag.API_EVENT: get_api_event_compiles(),
        DataTag.HASH_DICT: get_hash_data_compiles(),
        DataTag.TASK_TRACK: get_task_track_compact_compiles(),
        DataTag.MEMCPY_INFO: get_memcpy_info_compact_compiles(),
        DataTag.HCCL_INFO: get_hccl_info_compiles(),
        DataTag.MULTI_THREAD: get_multi_thread_compiles(),
        DataTag.GRAPH_ADD_INFO: get_ge_graph_add_info_compiles(),
        DataTag.TENSOR_ADD_INFO: get_ge_tensor_add_info_compiles(),
        DataTag.NODE_BASIC_INFO: get_ge_node_basic_info_compiles(),
        DataTag.NODE_ATTR_INFO: get_node_attr_info_compiles(),
        DataTag.FUSION_ADD_INFO: get_ge_fusion_add_info_compiles(),
        DataTag.MEMORY_APPLICATION: get_ge_memory_application_info_compiles(),
        DataTag.STATIC_OP_MEM: get_ge_static_op_mem_compiles(),
        DataTag.CTX_ID: get_ge_ctx_id_info_compiles(),
        DataTag.AICPU_ADD_INFO: get_aicpu_compiles(),
        DataTag.HCCL_OP_INFO: get_hccl_op_info_compiles(),
        DataTag.QOS: get_qos_compiles(),
        DataTag.MC2_COMM_INFO: get_mc2_comm_info_compiles(),
        DataTag.CAPTURE_STREAM_INFO: get_capture_stream_info_compiles(),
        DataTag.NETDEV_STATS: get_netdev_stats_compiles(),
        DataTag.UB: get_ub_compiles(),
        DataTag.CCU_MISSION: get_ccu_mission_compiles(),
        DataTag.CCU_CHANNEL: get_ccu_channel_compiles(),
        DataTag.CCU_TASK: get_ccu_task_info_compiles(),
        DataTag.CCU_GROUP: get_ccu_group_info_compiles(),
        DataTag.CCU_WAIT_SIGNAL: get_ccu_wait_signal_info_compiles(),
        DataTag.BIU_PERF_CHIP6: get_biu_compiles(),
        DataTag.SOC_PMU: get_soc_pmu_compiles(),
        DataTag.LPM_INFO: get_lpm_info_compiles(),
        DataTag.STREAM_EXPAND: get_host_stream_expand_spec_info_compiles(),
        DataTag.RUNTIME_OP_INFO: get_runtime_op_info_compiles(),
        DataTag.V5_MODEL_EXEOM: get_v5_model_exeom_compiles(),
        DataTag.V5_STARS_PROFILE: get_v5_stars_profile_compiles(),
        DataTag.DBG_FILE: get_dbg_file_compiles(),
        DataTag.DPU_TASK_TRACK: get_dpu_track_compact_compiles(),
        DataTag.DPU_HCCL_TRACK: get_dpu_hccl_track_compact_compiles(),
    }

    def __init__(self: any, sample_config: dict) -> None:
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = {}
        self._prof_factory_maker = ProfFactoryMaker(sample_config)

    def pick_up_files(self: any) -> None:
        """
        pick up the file list with the data tag
        :return: dict with data tag
        """

        def sort_by_suffix_number(s):
            match = re.search(r'_(\d+)$', s)
            return int(match.group(1)) if match else float('inf')

        _files = sorted(os.listdir(PathManager.get_data_dir(self._project_path)), key=sort_by_suffix_number)
        for (_data_tag, _data_regs), _file in itertools.product(self.FILES_FILTER_MAP.items(), _files):
            if get_file_name_pattern_match(_file, *_data_regs):
                self._file_list.setdefault(_data_tag, []).append(_file)

    def dispatch_parser(self: any) -> None:
        """
        entry of file dispatch for data-parsing
        :return: None
        """
        self.pick_up_files()
        if self._file_list:
            self._prof_factory_maker.create_parser_factory(self._file_list)

    def dispatch_calculator(self: any) -> None:
        """
        entry of file dispatch for data-calculating
        :return: None
        """
        self.pick_up_files()
        if self._file_list:
            self._prof_factory_maker.create_calculator_factory(self._file_list)
