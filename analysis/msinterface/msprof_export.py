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
import json
import logging
import multiprocessing
import os
import shutil

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.common import call_sys_exit
from common_func.common import error
from common_func.common import init_log
from common_func.common import print_info
from common_func.common import warn
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import check_file_readable
from common_func.file_name_manager import get_msprof_json_without_slice_compiles
from common_func.host_data_check_manager import HostDataCheckManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ge_logic_stream_singleton import GeLogicStreamSingleton
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import run_in_subprocess
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import analyze_collect_data
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import prepare_for_parse
from common_func.msprof_common import get_valid_sub_path
from common_func.msprof_exception import ProfException
from common_func.msvp_common import check_dir_writable
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from common_func.cpp_enable_scene import ExportTimelineScene, ExportSummaryScene
from common_func.system_data_check_manager import SystemDataCheckManager
from common_func.utils import Utils
from framework.file_dispatch import FileDispatch
from framework.load_info_manager import LoadInfoManager
from msinterface.msprof_c_interface import export_unified_db
from msinterface.msprof_c_interface import dump_device_data
from msinterface.msprof_c_interface import export_timeline
from msinterface.msprof_c_interface import export_summary
from msinterface.msprof_data_storage import MsprofDataStorage
from msinterface.msprof_export_data import MsProfExportDataUtils
from msinterface.msprof_output_summary import MsprofOutputSummary
from msinterface.msprof_timeline import MsprofTimeline
from msmodel.compact_info.task_track_model import TaskTrackModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.prof_enum.export_data_type import ExportDataType
from tuning.cluster.cluster_tuning_facade import ClusterTuningFacade
from tuning.cluster_tuning import ClusterTuning


class ExportCommand:
    """
    The class for handle export command.
    """

    FILE_NAME = os.path.basename(__file__)
    EXPORT_HANDLE_MAP = {
        MsProfCommonConstant.TIMELINE: [
            {
                'export_type': ExportDataType.STEP_TRACE,
                'handler': AiStackDataCheckManager.contain_training_trace_data_or_step,
            },
            {'export_type': ExportDataType.API, 'handler': AiStackDataCheckManager.contain_api_data},
            {'export_type': ExportDataType.TASK_TIME, 'handler': AiStackDataCheckManager.contain_core_cpu_reduce_data},
            {'export_type': ExportDataType.HBM, 'handler': SystemDataCheckManager.contain_hbm_data},
            {'export_type': ExportDataType.DDR, 'handler': SystemDataCheckManager.contain_ddr_data},
            {'export_type': ExportDataType.PCIE, 'handler': SystemDataCheckManager.contain_pcie_data},
            {'export_type': ExportDataType.HCCS, 'handler': SystemDataCheckManager.contain_hccs_data},
            {'export_type': ExportDataType.NIC, 'handler': SystemDataCheckManager.contain_nic_data},
            {'export_type': ExportDataType.ROCE, 'handler': SystemDataCheckManager.contain_roce_data},
            {'export_type': ExportDataType.LLC_READ_WRITE, 'handler': SystemDataCheckManager.contain_read_write_data},
            {'export_type': ExportDataType.LLC_AICPU, 'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_CTRLCPU, 'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_BANDWIDTH, 'handler': SystemDataCheckManager.contain_llc_bandwidth_data},
            {'export_type': ExportDataType.NPU_MEM, 'handler': SystemDataCheckManager.contain_npu_mem_data},
            {
                'export_type': ExportDataType.AI_CORE_UTILIZATION,
                'handler': AiStackDataCheckManager.contain_ai_core_sample_based,
            },
            {'export_type': ExportDataType.HOST_CPU_USAGE, 'handler': HostDataCheckManager.contain_host_cpuusage_data},
            {'export_type': ExportDataType.HOST_MEM_USAGE, 'handler': HostDataCheckManager.contain_host_mem_usage_data},
            {
                'export_type': ExportDataType.HOST_NETWORK_USAGE,
                'handler': HostDataCheckManager.contain_host_network_usage_data,
            },
            {
                'export_type': ExportDataType.HOST_DISK_USAGE,
                'handler': HostDataCheckManager.contain_host_disk_usage_data,
            },
            {'export_type': ExportDataType.OS_RUNTIME_API, 'handler': HostDataCheckManager.contain_runtime_api_data},
            {
                'export_type': ExportDataType.FFTS_SUB_TASK_TIME,
                'handler': AiStackDataCheckManager.contain_sub_task_data,
            },
            {'export_type': ExportDataType.COMMUNICATION, 'handler': AiStackDataCheckManager.contain_hccl_hcom_data},
            {'export_type': ExportDataType.MSPROF_TX, 'handler': AiStackDataCheckManager.contain_msproftx_data},
            {
                'export_type': ExportDataType.STARS_SOC,
                'handler': AiStackDataCheckManager.contain_stars_soc_profiler_data,
            },
            {
                'export_type': ExportDataType.STARS_CHIP_TRANS,
                'handler': AiStackDataCheckManager.contain_stars_chip_trans_data,
            },
            {'export_type': ExportDataType.LOW_POWER, 'handler': AiStackDataCheckManager.contain_stars_low_power_data},
            {'export_type': ExportDataType.INSTR, 'handler': AiStackDataCheckManager.contain_biu_perf_data},
            {'export_type': ExportDataType.ACC_PMU, 'handler': AiStackDataCheckManager.contain_acc_pmu_data},
            {'export_type': ExportDataType.SIO, 'handler': AiStackDataCheckManager.contain_sio_data},
            {'export_type': ExportDataType.QOS, 'handler': SystemDataCheckManager.contain_qos_data},
            {'export_type': ExportDataType.BLOCK_DETAIL, 'handler': AiStackDataCheckManager.contain_block_log_data},
            {'export_type': ExportDataType.UB, 'handler': AiStackDataCheckManager.contain_ub_data},
            {'export_type': ExportDataType.CCU_MISSION, 'handler': AiStackDataCheckManager.contain_ccu_mission_data},
            {'export_type': ExportDataType.VOLTAGE, 'handler': SystemDataCheckManager.contain_lpm_data},
            {'export_type': ExportDataType.DPU, 'handler': AiStackDataCheckManager.contain_dpu_data},
            {'export_type': ExportDataType.FUSION_TASK, 'handler': AiStackDataCheckManager.contain_fusion_task_data},
            {'export_type': ExportDataType.FREQ, 'handler': lambda result_dir, device_id=None: True},
            {'export_type': ExportDataType.MSPROF, 'handler': lambda result_dir, device_id=None: True},
        ],
        MsProfCommonConstant.SUMMARY: [
            {'export_type': ExportDataType.TASK_TIME, 'handler': AiStackDataCheckManager.contain_task_time_task},
            {'export_type': ExportDataType.L2_CACHE, 'handler': AiStackDataCheckManager.contain_l2_cache_data},
            {
                'export_type': ExportDataType.STEP_TRACE,
                'handler': AiStackDataCheckManager.contain_step_trace_summary_data,
            },
            {'export_type': ExportDataType.OP_SUMMARY, 'handler': AiStackDataCheckManager.contain_op_summary_data},
            {'export_type': ExportDataType.OP_STATISTIC, 'handler': AiStackDataCheckManager.contain_op_statistic_data},
            {'export_type': ExportDataType.AICPU, 'handler': AiStackDataCheckManager.contain_dp_aicpu_data},
            {'export_type': ExportDataType.DP, 'handler': AiStackDataCheckManager.contain_data_preprocess_dp_data},
            {'export_type': ExportDataType.CPU_USAGE, 'handler': SystemDataCheckManager.contain_cpu_usage_data},
            {
                'export_type': ExportDataType.PROCESS_CPU_USAGE,
                'handler': SystemDataCheckManager.contain_pid_cpu_usage_data,
            },
            {'export_type': ExportDataType.SYS_MEM, 'handler': SystemDataCheckManager.contains_sys_memory_data},
            {'export_type': ExportDataType.PROCESS_MEM, 'handler': SystemDataCheckManager.contains_pid_memory_data},
            {'export_type': ExportDataType.HBM, 'handler': SystemDataCheckManager.contain_hbm_data},
            {'export_type': ExportDataType.DDR, 'handler': SystemDataCheckManager.contain_ddr_data},
            {'export_type': ExportDataType.PCIE, 'handler': SystemDataCheckManager.contain_pcie_data},
            {'export_type': ExportDataType.HCCS, 'handler': SystemDataCheckManager.contain_hccs_data},
            {'export_type': ExportDataType.DVPP, 'handler': SystemDataCheckManager.contain_dvpp_data},
            {'export_type': ExportDataType.NIC, 'handler': SystemDataCheckManager.contain_nic_data},
            {'export_type': ExportDataType.ROCE, 'handler': SystemDataCheckManager.contain_roce_data},
            {'export_type': ExportDataType.LLC_READ_WRITE, 'handler': SystemDataCheckManager.contain_read_write_data},
            {'export_type': ExportDataType.LLC_AICPU, 'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_CTRLCPU, 'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_BANDWIDTH, 'handler': SystemDataCheckManager.contain_llc_bandwidth_data},
            {'export_type': ExportDataType.AI_CPU_TOP_FUNCTION, 'handler': SystemDataCheckManager.contain_ai_cpu_data},
            {'export_type': ExportDataType.AI_CPU_PMU_EVENTS, 'handler': SystemDataCheckManager.contain_ai_cpu_data},
            {
                'export_type': ExportDataType.CTRL_CPU_TOP_FUNCTION,
                'handler': SystemDataCheckManager.contain_ctrl_cpu_data,
            },
            {
                'export_type': ExportDataType.CTRL_CPU_PMU_EVENTS,
                'handler': SystemDataCheckManager.contain_ctrl_cpu_data,
            },
            {'export_type': ExportDataType.TS_CPU_TOP_FUNCTION, 'handler': SystemDataCheckManager.contain_ts_cpu_data},
            {'export_type': ExportDataType.TS_CPU_PMU_EVENTS, 'handler': SystemDataCheckManager.contain_ts_cpu_data},
            {'export_type': ExportDataType.NPU_MEM, 'handler': SystemDataCheckManager.contain_npu_mem_summary_data},
            {'export_type': ExportDataType.FUSION_OP, 'handler': AiStackDataCheckManager.contain_fusion_op_data},
            {
                'export_type': ExportDataType.AI_CORE_UTILIZATION,
                'handler': AiStackDataCheckManager.contain_ai_core_sample_based,
            },
            {
                'export_type': ExportDataType.AI_VECTOR_CORE_UTILIZATION,
                'handler': AiStackDataCheckManager.contain_aiv_core_sample_based,
            },
            {
                'export_type': ExportDataType.OS_RUNTIME_STATISTIC,
                'handler': HostDataCheckManager.contain_runtime_api_data,
            },
            {'export_type': ExportDataType.MSPROF_TX, 'handler': AiStackDataCheckManager.contain_msproftx_data},
            {'export_type': ExportDataType.HOST_CPU_USAGE, 'handler': HostDataCheckManager.contain_host_cpuusage_data},
            {'export_type': ExportDataType.HOST_MEM_USAGE, 'handler': HostDataCheckManager.contain_host_mem_usage_data},
            {
                'export_type': ExportDataType.HOST_NETWORK_USAGE,
                'handler': HostDataCheckManager.contain_host_network_usage_data,
            },
            {
                'export_type': ExportDataType.HOST_DISK_USAGE,
                'handler': HostDataCheckManager.contain_host_disk_usage_data,
            },
            {'export_type': ExportDataType.OPERATOR_MEMORY, 'handler': AiStackDataCheckManager.contain_op_mem_data},
            {'export_type': ExportDataType.MEMORY_RECORD, 'handler': AiStackDataCheckManager.contain_mem_rec_data},
            {
                'export_type': ExportDataType.COMMUNICATION_STATISTIC,
                'handler': AiStackDataCheckManager.contain_hccl_statistic_data,
            },
            {
                'export_type': ExportDataType.API_STATISTIC,
                'handler': AiStackDataCheckManager.contain_api_statistic_data,
            },
            {
                'export_type': ExportDataType.NPU_MODULE_MEM,
                'handler': AiStackDataCheckManager.contain_npu_module_mem_data,
            },
            {'export_type': ExportDataType.AICPU_MI, 'handler': AiStackDataCheckManager.contain_dp_aicpu_data},
            {
                'export_type': ExportDataType.STATIC_OP_MEM,
                'handler': AiStackDataCheckManager.contain_static_op_mem_data,
            },
            {'export_type': ExportDataType.UB, 'handler': AiStackDataCheckManager.contain_ub_data},
            {'export_type': ExportDataType.CCU_MISSION, 'handler': AiStackDataCheckManager.contain_ccu_mission_data},
            {'export_type': ExportDataType.CCU_CHANNEL, 'handler': SystemDataCheckManager.contain_ccu_channel_data},
            {'export_type': ExportDataType.SOC_PMU, 'handler': SystemDataCheckManager.contain_soc_pmu_data},
        ],
    }
    MODEL_ID = "model_id"
    INPUT_MODEL_ID = "input_model_id"

    def __init__(self: any, command_type: str, args: any) -> None:
        self.command_type = command_type
        self.collection_path = os.path.realpath(args.collection_path)
        self.iteration_id = getattr(args, "iteration_id", NumberConstant.DEFAULT_ITER_ID)
        if self.iteration_id is None:
            self.iteration_id = NumberConstant.DEFAULT_ITER_ID
        self.iteration_count = getattr(args, "iteration_count", NumberConstant.DEFAULT_ITER_COUNT)
        self.sample_config = {}
        self.export_format = getattr(args, "export_format", None)
        self.list_map = {
            'export_type_list': [],
            'devices_list': '',
            'model_id': getattr(args, self.MODEL_ID, None),
            'input_model_id': getattr(args, self.MODEL_ID, None) is not None,
        }
        self.iteration_range = None
        self._cluster_params = {'is_cluster_scene': False, 'cluster_path': []}
        self.clear_mode = getattr(args, "clear_mode", False)
        self.is_data_analyzed = False
        self.reports_path = getattr(args, "reports_path", "")

    @staticmethod
    def get_model_id_set(result_dir: str, db_name: str, table_name: str) -> any:
        """
        get model id set
        :param result_dir: result dir
        :param db_name: db name
        :param table_name: table name
        :return: model_ids_set
        """
        db_path = PathManager.get_db_path(result_dir, db_name)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, table_name):
            logging.warning(
                "Can not get model id from framework data, maybe framework data is not collected,"
                " try to export data with no framework data."
            )
            DBManager.destroy_db_connect(conn, curs)
            return set()

        sql = "select model_id from {0}".format(table_name)  # nosec B608
        model_ids = DBManager.fetch_all_data(curs, sql)
        model_ids_set = {model_id[0] for model_id in model_ids}
        DBManager.destroy_db_connect(conn, curs)

        return model_ids_set

    @staticmethod
    def _is_host_export(result_dir: str) -> bool:
        return result_dir.endswith("host")

    @staticmethod
    def _check_flip_data(host_flip: list, device_flip: list) -> None:
        if len(host_flip) != len(device_flip):
            logging.warning(
                "Different flip numbers, %d host flips and %d device flips.", len(host_flip), len(device_flip)
            )
        for host_f, device_f in zip(host_flip, device_flip):
            if host_f.flip_num != device_f.flip_num:
                logging.warning("The flip is not consistent between host and device")

    @staticmethod
    def _process_init(result_dir, export_mode):
        init_log(result_dir)
        LoadInfoManager.load_info(result_dir)
        ProfilingScene().set_mode(export_mode)

    @staticmethod
    def _get_min_model_id(model_match_set):
        if model_match_set:
            return min(model_match_set)
        message = (
            "The model_id obtained from the GE doesn't overlap that in the step_trace. The reported data "
            "may be lost and the profiling will stop. Check whether the reported data is correct."
        )
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)

    @staticmethod
    def _get_sorted_json_file(output_path: str) -> str:
        res_file = []
        for file in os.listdir(output_path):
            for pattern in get_msprof_json_without_slice_compiles():
                match = pattern.match(file)
                if match:
                    timestamp = int(match.group(1))
                    res_file.append((timestamp, os.path.join(output_path, file)))
        if res_file:
            res_file.sort(key=lambda x: x[0], reverse=True)
            return res_file[0][1]
        return ''

    def process(self: any) -> None:
        """
        handle export command
        :return: None
        """
        check_path_valid(self.collection_path, False)
        if self.reports_path:
            check_file_readable(self.reports_path)
        self._process_sub_dirs()
        if self._cluster_params.get('is_cluster_scene', False):
            self._show_cluster_tuning()

    def _check_all_report(self, result_dir: str) -> None:
        """
        check all report
        """
        if ProfilingScene().is_graph_export():
            if ProfilingScene().is_operator():
                # 单算子不能按子图导
                error(self.FILE_NAME, "Please do not set 'model-id' and 'iteration-id' in single op mode.")
                call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)
            return
        if not (ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version()):
            # 不支持唯一ID, 不支持基于唯一ID的全导和按step导出
            if ProfilingScene().is_step_export():
                # 按step导出必须支持唯一ID
                error(self.FILE_NAME, "Do not support iteration export. Please upgrade package and driver.")
                call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)
            # 图模式按子图导, 单算子全导
            export_mode = ExportMode.ALL_EXPORT if ProfilingScene().is_operator() else ExportMode.GRAPH_EXPORT
            ProfilingScene().set_mode(export_mode)
            self.sample_config[StrConstant.EXPORT_MODE] = export_mode
            return
        # 支持唯一ID, 对flip进行校验
        if ProfilingScene().is_step_export():
            print_info(self.FILE_NAME, "Step data will be exported, path is: %s." % result_dir)
        else:
            print_info(self.FILE_NAME, "All collected data will be exported, path is: %s." % result_dir)
        host_flip_da_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_RTS_TRACK)
        device_flip_da_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_STEP_TRACE)
        if not os.path.exists(host_flip_da_path) or not os.path.exists(device_flip_da_path):
            return
        with TaskTrackModel(result_dir, [DBNameConstant.TABLE_HOST_TASK_FLIP]) as host_flip_model:
            host_flip = host_flip_model.get_all_data(DBNameConstant.TABLE_HOST_TASK_FLIP, TaskFlip)
        with TsTrackModel(
            result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]
        ) as device_flip_model:
            device_flip = device_flip_model.get_task_flip_data()
        self._check_flip_data(host_flip, device_flip)

    def _set_default_model_id(self, result_dir, model_match_set, ge_data_set):
        conn, curs = DBManager.check_connect_db(result_dir, DBNameConstant.DB_STEP_TRACE)
        if not (conn and curs):
            return self._get_min_model_id(model_match_set)
        sql = "select model_id, max(index_id) from {} group by model_id".format(DBNameConstant.TABLE_STEP_TRACE_DATA)  # nosec B608
        model_and_index = list(filter(lambda x: x[0] in model_match_set, DBManager.fetch_all_data(curs, sql)))

        if not ge_data_set:
            trace_data = DBManager.fetch_all_data(
                curs,
                "select model_id, iter_id from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA),  # nosec B608
                dto_class=StepTraceDto,
            )
            result = dict()
            for data in trace_data:
                model_id_times = result.get(data.model_id, 0)
                result[data.model_id] = model_id_times + 1
            model_and_index = list(zip(result.keys(), result.values()))
        model_and_index.sort(key=lambda x: x[1])
        DBManager.destroy_db_connect(conn, curs)
        return model_and_index.pop()[0] if model_and_index else self._get_min_model_id(model_match_set)

    def _add_export_type(self: any, result_dir: str) -> None:
        self.list_map['export_type_list'] = []
        export_map = self.EXPORT_HANDLE_MAP.get(self.command_type, [])
        devices = InfoConfReader().get_device_list()
        device = int(devices[0]) if devices else 0
        for item in export_map:
            if item['handler'](result_dir, device):
                self.list_map.get('export_type_list').append(item)
        # msprof will always be exported
        if self.command_type == MsProfCommonConstant.TIMELINE and len(self.list_map.get('export_type_list')) == 1:
            self.list_map.get('export_type_list').pop()

    def _is_iteration_range_valid(self, project_path):
        with TsTrackModel(project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]) as _trace:
            iter_range = _trace.get_index_range_with_model(self.list_map.get(self.MODEL_ID))
        if not iter_range:
            error(self.FILE_NAME, "The step export is not supported.")
            return False
        min_iter = iter_range[0]
        max_iter = iter_range[1]
        if self.iteration_id < min_iter or self.iteration_id + self.iteration_count - 1 > max_iter:
            error(
                self.FILE_NAME,
                f'The exported iteration {self.iteration_id}-{self.iteration_id + self.iteration_count - 1} '
                f'is invalid for model id {self.list_map.get(self.MODEL_ID)}. '
                f'Please enter a valid iteration within {min_iter}-{max_iter}.',
            )
            return False

        if (
            self.iteration_count < NumberConstant.DEFAULT_ITER_ID
            or self.iteration_count > IterationRange.MAX_ITERATION_COUNT
        ):
            error(
                self.FILE_NAME,
                f'The iteration count {self.iteration_count} is invalid '
                f'for model id {self.list_map.get(self.MODEL_ID)}. '
                f'Must be greater than 0 and less than or equal to '
                f'{IterationRange.MAX_ITERATION_COUNT}. Please enter a valid iteration count.',
            )
            return False
        return True

    def _check_index_id(self: any, project_path: str) -> None:
        """
        check index id
        :param project_path: path to get profiling scene
        :return: void
        """
        ProfilingScene().init(project_path)
        if ProfilingScene().is_all_export():
            if self.iteration_count > NumberConstant.DEFAULT_ITER_COUNT:
                warn(
                    self.FILE_NAME,
                    f'Param of "iteration-count" is {self.iteration_count}, but it is unnecessary in all export mode.',
                )
                self.iteration_count = NumberConstant.DEFAULT_ITER_COUNT
            return

        if not self._is_iteration_range_valid(project_path):
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

    def _analyse_sample_config(self: any, result_dir: str) -> None:
        self.sample_config = ConfigMgr.read_sample_config(result_dir)
        self.sample_config[StrConstant.EXPORT_MODE] = ProfilingScene().get_mode()

    def _analyse_data(self: any, result_dir: str) -> None:
        is_data_analyzed = FileManager.is_analyzed_data(result_dir)
        if not is_data_analyzed:
            analyze_collect_data(result_dir, self.sample_config)
        else:
            print_info(
                self.FILE_NAME, 'The data in "%s" has been analyzed. Parsing phase will be skipped.' % result_dir
            )
            logging.warning(
                "File all_file.complete already exists. All data will be exported from db in sqlite. Path is %s ",
                result_dir,
            )
            self.is_data_analyzed = True

    def _check_model_id(self: any, result_dir: str) -> None:
        """
        check model id
        :param result_dir: initialize profiling_scene
        :return: void
        """
        profiling_scene = ProfilingScene()
        profiling_scene.init(result_dir)

        if profiling_scene.is_operator():
            self.list_map[self.MODEL_ID] = Constant.GE_OP_MODEL_ID
            return

        model_ids_ge = ExportCommand.get_model_id_set(
            result_dir, DBNameConstant.DB_GE_INFO, DBNameConstant.TABLE_GE_TASK
        )

        model_ids_step = ExportCommand.get_model_id_set(
            result_dir, DBNameConstant.DB_STEP_TRACE, DBNameConstant.TABLE_STEP_TRACE_DATA
        )

        model_ids_hccl = ExportCommand.get_model_id_set(
            result_dir, DBNameConstant.DB_HCCL, DBNameConstant.TABLE_HCCL_TASK
        )

        model_match_union = model_ids_ge
        if model_ids_hccl:
            model_match_union = model_ids_hccl | model_ids_ge
        model_match_set = model_ids_step
        if model_ids_ge and model_ids_step:
            not_match_set = model_ids_ge - model_ids_step
            if not_match_set:
                logging.warning("step trace data miss model id.")
                logging.debug("step trace data miss %s.", not_match_set)
            model_match_set = model_match_union & model_ids_step
        else:
            logging.warning("ge step info data miss model id.")

        if not self.list_map.get(self.INPUT_MODEL_ID):
            self.list_map[self.MODEL_ID] = self._set_default_model_id(result_dir, model_match_set, model_ids_ge)
            if Utils.is_single_op_graph_mix(result_dir):
                self.list_map[self.MODEL_ID] = Constant.GE_OP_MODEL_ID
            return

        if profiling_scene.is_graph_export() and self.list_map.get(self.MODEL_ID) not in model_match_set:
            message = (
                f"The model id {self.list_map.get(self.MODEL_ID)} is invalid. "  # nosec B608
                f"Must select from {model_match_set}. Please enter a valid model id."  # nosec B608
            )
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)

    def _set_iteration_info(self, result_dir):
        self.iteration_range = IterationRange(
            model_id=self.list_map.get('model_id'), iteration_id=self.iteration_id, iteration_count=self.iteration_count
        )
        MsprofTimeline().set_iteration_info(result_dir, self.iteration_range)

    def _update_device_list(self):
        device_lst = InfoConfReader().get_device_list()
        self.list_map.update({'devices_list': device_lst})

    def _get_sample_json(self, result_dir) -> dict:
        if not self._is_host_export(result_dir):
            device_lst = self.list_map.get('devices_list')
            device = device_lst[0] if device_lst else None
            sample_json = {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: result_dir,
                StrConstant.PARAM_DEVICE_ID: device,
                StrConstant.PARAM_ITER_ID: self.iteration_range,
            }
        else:
            sample_json = {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: result_dir,
            }
        sample_json[StrConstant.EXPORT_MODE] = ProfilingScene().get_mode()
        return sample_json

    def _has_data_to_export(self):
        if not self.list_map.get('export_type_list'):
            return False
        return True

    def _handle_export_data(self: any, params: dict) -> None:
        result = json.loads(MsProfExportDataUtils.export_data(params))
        if result.get('status', NumberConstant.EXCEPTION) == NumberConstant.SKIP:
            return
        if result.get('status', NumberConstant.EXCEPTION) != NumberConstant.SUCCESS:
            if result.get('status', NumberConstant.EXCEPTION) == NumberConstant.ERROR:
                error(self.FILE_NAME, result.get('info', ""))
            else:
                logging.warning(result.get('info', ""))

    def _print_export_info(self: any, params: dict, data: str) -> None:
        export_info = 'The {0} {1} data has been exported to "{2}".'.format(
            params.get(StrConstant.PARAM_DATA_TYPE), self.command_type, data
        )

        if params.get(StrConstant.PARAM_DEVICE_ID) is not None:
            if params.get(StrConstant.PARAM_DEVICE_ID) == str(NumberConstant.HOST_ID):
                export_info = 'The {0} {1} data of host for iteration {2} has been exported to "{3}".'.format(
                    params.get(StrConstant.PARAM_DATA_TYPE),
                    self.command_type,
                    params.get(StrConstant.PARAM_ITER_ID),
                    data,
                )
            else:
                export_info = 'The {0} {1} data of device {2} for iteration {3} has been exported to "{4}".'.format(
                    params.get(StrConstant.PARAM_DATA_TYPE),
                    self.command_type,
                    params.get(StrConstant.PARAM_DEVICE_ID),
                    params.get(StrConstant.PARAM_ITER_ID),
                    data,
                )
        print_info(self.FILE_NAME, export_info)

    def _clear_dir(self, rm_list: list) -> None:
        if not self.clear_mode:
            return
        for result_dir in rm_list:
            if not os.path.exists(result_dir):
                return
            sqlite_path = PathManager.get_sql_dir(result_dir)
            if os.path.exists(sqlite_path):
                check_dir_writable(sqlite_path)
                shutil.rmtree(sqlite_path)

    def _multiprocessing_handle_export_data(self: any, event, result_dir, export_mode):
        ExportCommand._process_init(result_dir, export_mode)
        try:
            self._handle_export_output_data(event, result_dir)
        except ProfException as err:
            if err.message:
                err.callback(MsProfCommonConstant.COMMON_FILE_NAME, err.message)
            else:
                warn(
                    MsProfCommonConstant.COMMON_FILE_NAME,
                    'Analysis data in "%s" failed. Maybe the data is incomplete.' % result_dir,
                )

    def _handle_export_output_data(self: any, event, result_dir):
        if not self.list_map.get('devices_list', []):
            self._export_data(event, None, result_dir)
            return
        for device_id in self.list_map.get('devices_list', []):
            self._export_data(event, int(device_id), result_dir)

    def _handle_export(self: any, result_dir: str) -> None:
        processes = []
        export_mode = ProfilingScene().get_mode()
        try:
            for event in self.list_map.get('export_type_list', []):
                if self.command_type == MsProfCommonConstant.TIMELINE:
                    self._handle_export_output_data(event, result_dir)
                    continue
                process = multiprocessing.Process(
                    target=self._multiprocessing_handle_export_data, args=(event, result_dir, export_mode)
                )
                process.start()
                processes.append(process)
            for process in processes:
                process.join()
        except ProfException as err:
            if err.message:
                err.callback(MsProfCommonConstant.COMMON_FILE_NAME, err.message)
            else:
                warn(
                    MsProfCommonConstant.COMMON_FILE_NAME,
                    'Analysis data in "%s" failed. Maybe the data is incomplete.' % result_dir,
                )

    def _export_data(self: any, event: dict, device_id: str, result_dir: str) -> None:
        export_data_type = event.get('export_type', ExportDataType.INVALID).name.lower()
        params = {
            StrConstant.PARAM_DATA_TYPE: export_data_type,
            StrConstant.PARAM_RESULT_DIR: result_dir,
            StrConstant.PARAM_DEVICE_ID: device_id,
            StrConstant.PARAM_EXPORT_TYPE: self.command_type,
            StrConstant.PARAM_ITER_ID: self.iteration_range,
            StrConstant.PARAM_EXPORT_FORMAT: self.export_format,
            StrConstant.PARAM_MODEL_ID: self.list_map.get("model_id"),
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: self.command_type,
        }

        self._handle_export_data(params)

    def _show_cluster_tuning(self) -> None:
        if self.command_type != MsProfCommonConstant.SUMMARY:
            return

        # 在该代码段屏蔽所有的log打印内容（对打屏内容不做处理）
        logging.disable(level=logging.CRITICAL)
        ClusterTuning(self._cluster_params.get('cluster_path')).run()
        params = {
            "collection_path": self.collection_path,
            "model_id": 0,
            "npu_id": -1,
            "iteration_id": self.iteration_id,
        }
        try:
            ClusterTuningFacade(params).process()
        except ProfException:
            warn(self.FILE_NAME, 'Cluster Tuning did not complete!')
        finally:
            # 解禁打屏限制
            logging.disable(logging.NOTSET)

    def _update_cluster_params(self: any, sub_path: str, is_cluster: bool) -> None:
        if is_cluster:
            self._cluster_params['is_cluster_scene'] = True
            self._cluster_params.setdefault('cluster_path', []).append(sub_path)

    def _process_sub_dirs(self: any, sub_path: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if sub_path:
            collect_path = os.path.join(self.collection_path, sub_path)
        sub_dirs = sorted(get_path_dir(collect_path), reverse=True)
        path_table = {StrConstant.HOST_PATH: "", StrConstant.DEVICE_PATH: []}
        for sub_dir in sub_dirs:  # result_dir
            sub_path = get_valid_sub_path(collect_path, sub_dir, False)
            if DataCheckManager.contain_info_json_data(sub_path):
                self._update_cluster_params(sub_path, is_cluster)
                # 统一收集合法路径 后续统一处理
                if sub_dir == StrConstant.HOST_PATH:
                    path_table[StrConstant.HOST_PATH] = sub_path
                else:
                    path_table[StrConstant.DEVICE_PATH].append(sub_path)
                path_table.setdefault("collection_path", collect_path)
            elif sub_path and is_cluster:
                warn(
                    self.FILE_NAME,
                    'Invalid parsing dir("%s"), -dir must be profiling data dir '
                    'such as PROF_XXX_XXX_XXX' % collect_path,
                )
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)
            self.list_map['devices_list'] = ''
        run_in_subprocess(self._process_data, path_table)

    def _process_data(self, path_table: dict):
        if not path_table.get(StrConstant.HOST_PATH) and not path_table.get(StrConstant.DEVICE_PATH):
            warn(self.FILE_NAME, 'Can not find any host or device path in dir("%s"). ' % self.collection_path)
            return
        # start parse
        self._start_parse(path_table)
        # start calculate
        self._start_calculate(path_table)
        # start view
        try:
            self._start_view(path_table)
        finally:
            rm_list = []
            if path_table.get(StrConstant.HOST_PATH):
                rm_list.append(path_table.get(StrConstant.HOST_PATH))
            if path_table.get(StrConstant.DEVICE_PATH):
                rm_list.extend(path_table.get(StrConstant.DEVICE_PATH))
            if rm_list:
                PathManager.del_summary_and_timeline_dir(rm_list)
                self._clear_dir(rm_list)

    def _start_parse(self, path_table: dict):
        # host
        host_path = path_table.get(StrConstant.HOST_PATH)
        if host_path:
            self._parse_data(host_path)
            # host parse 后初始化logicStream模块
            GeLogicStreamSingleton().load_info(host_path)
        # device
        for device_path in path_table.get(StrConstant.DEVICE_PATH):
            self._parse_data(device_path)
        # device 执行完后执行device c化
        # path_table使用前明确校验host或device中必不为空,不存在越界情况
        if not self.is_data_analyzed:
            dump_device_data(host_path if host_path else path_table.get(StrConstant.DEVICE_PATH)[0])

    def _parse_data(self, device_path: str):
        if not device_path:
            return
        prepare_for_parse(device_path)
        LoadInfoManager.load_info(device_path)
        self._analyse_sample_config(device_path)
        self._analyse_data(device_path)

    def _start_calculate(self, path_table: dict):
        # host
        mode = ProfilingScene().get_mode()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        host_path = path_table.get(StrConstant.HOST_PATH)
        self._calculate_data(host_path)
        # device
        ProfilingScene().set_mode(mode)
        for device_path in path_table.get(StrConstant.DEVICE_PATH, []):
            self._calculate_data(device_path)

    def _calculate_data(self, device_path: str):
        if not device_path:
            return
        prepare_for_parse(device_path)
        LoadInfoManager.load_info(device_path)
        self._update_list_map(device_path)
        file_dispatch = FileDispatch(self._get_sample_json(device_path))
        file_dispatch.dispatch_calculator()

    def _check_export_timeline_with_so(self, path_table: dict):
        """
        有so文件，且是task-based场景下导出timeline时会返回true
        """
        host_path = path_table.get(StrConstant.HOST_PATH)
        valid_path = host_path if host_path else path_table.get(StrConstant.DEVICE_PATH)[0]
        is_sample_based = ConfigMgr.is_ai_core_sample_based(valid_path)
        return ExportTimelineScene(self.command_type, is_sample_based).is_cpp_enable()

    def _check_export_summary_with_so(self):
        """
        有so文件，且是全导，可以使用C++来导出summary
        """
        return ExportSummaryScene(self.command_type, ProfilingScene().is_all_export()).is_cpp_enable()

    def _check_and_split_json_trace(self, result_dir: str) -> None:
        output_path = os.path.join(result_dir, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
        if not os.path.exists(output_path):
            return
        expect_file = self._get_sorted_json_file(output_path)
        params = {
            StrConstant.PARAM_RESULT_DIR: result_dir,
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: output_path,
            StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE,
            StrConstant.PARAM_DATA_TYPE: 'msprof',
        }
        MsprofDataStorage().slice_msprof_json_for_so(expect_file, params)

    def _start_view(self, path_table: dict):
        if self.command_type == MsProfCommonConstant.DB:
            export_unified_db(path_table.get("collection_path"))
            return

        # host
        mode = ProfilingScene().get_mode()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        host_path = path_table.get(StrConstant.HOST_PATH)
        self._view_data(host_path)
        # device
        # viewer timeline
        ProfilingScene().set_mode(mode)
        if self._check_export_timeline_with_so(path_table) and ProfilingScene().is_all_export():
            export_timeline(path_table.get("collection_path"), self.reports_path)
            self._check_and_split_json_trace(path_table.get("collection_path"))
            return
        # viewer summary
        if self._check_export_summary_with_so():
            export_summary(path_table.get("collection_path"))
        device_paths_list = path_table.get(StrConstant.DEVICE_PATH, [])
        for device_path in device_paths_list:
            self._view_data(device_path)

        profiler = MsprofOutputSummary(path_table.get("collection_path"), self.export_format)
        profiler.export(self.command_type)

    def _view_data(self, device_path: str):
        if not device_path:
            return
        LoadInfoManager.load_info(device_path)
        self._update_list_map(device_path)
        MsprofTimeline().set_connection_list(device_path)
        MsprofTimeline().init_export_data()

        self._add_export_type(device_path)
        if not self._has_data_to_export():
            print_info(self.FILE_NAME, 'There is no %s data to export for "%s"' % (self.command_type, device_path))
            return

        if self.command_type == MsProfCommonConstant.SUMMARY:
            check_path_valid(PathManager.get_summary_dir(device_path), True)
        else:
            check_path_valid(PathManager.get_timeline_dir(device_path), True)

        self._handle_export(device_path)

    def _update_list_map(self, device_path: str):
        self._update_device_list()
        if not device_path.endswith(StrConstant.HOST_PATH):
            self._check_model_id(device_path)
            self._check_all_report(device_path)
            self._check_index_id(device_path)
            self._set_iteration_info(device_path)
