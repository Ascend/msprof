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
import threading

from common_func.config_mgr import ConfigMgr
from common_func.data_manager import DataManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from host_prof.host_prof_presenter_manager import HostExportType
from host_prof.host_prof_presenter_manager import get_host_prof_summary
from host_prof.host_prof_presenter_manager import get_host_prof_timeline
from host_prof.host_syscall.presenter.host_syscall_presenter import HostSyscallPresenter
from msconfig.config_manager import ConfigManager
from msinterface.msprof_data_storage import MsprofDataStorage
from msinterface.msprof_timeline import MsprofTimeline
from msparser.aicpu.parse_dp_data import ParseDpData
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_op_report import ReportOPCounter
from viewer.ai_core_report import AiCoreReport
from viewer.aicpu_viewer import ParseAiCpuData
from viewer.api_statistic_viewer import ApiStatisticViewer
from viewer.api_viewer import ApiViewer
from viewer.biu_perf.biu_perf_viewer import BiuPerfViewer
from viewer.cpu_data_report import get_aictrl_pmu_events
from viewer.cpu_data_report import get_cpu_hot_function
from viewer.cpu_data_report import get_ts_pmu_events
from viewer.cpu_usage_report import get_process_cpu_usage
from viewer.cpu_usage_report import get_sys_cpu_usage_data
from viewer.dpu_viewer import DPUViewer
from viewer.ge_info_report import get_ge_model_data
from viewer.get_hccl_export_data import HCCLExport
from viewer.get_l2_cache_data import add_op_name, process_hit_rate
from viewer.get_l2_cache_data import get_l2_cache_data
from viewer.get_msvp_llc_timeline_training import get_ddr_timeline
from viewer.get_msvp_llc_timeline_training import get_hbm_timeline
from viewer.get_msvp_llc_timeline_training import get_llc_timeline
from viewer.get_msvp_summary_mem import get_process_mem_data
from viewer.get_msvp_summary_mem import get_sys_mem_data
from viewer.get_msvp_summary_training import get_hbm_summary_data
from viewer.get_trace_timeline import get_aicore_utilization_timeline
from viewer.get_trace_timeline import get_dvpp_timeline
from viewer.get_trace_timeline import get_hccs_timeline
from viewer.get_trace_timeline import get_network_timeline
from viewer.get_trace_timeline import get_pcie_timeline
from viewer.hardware_info_report import get_ddr_data
from viewer.hardware_info_report import get_llc_bandwidth
from viewer.hardware_info_report import llc_capacity_data
from viewer.hardware_info_view import get_llc_train_summary
from viewer.hccl_op_report import ReportHcclStatisticData
from viewer.interconnection_view import InterConnectionView
from viewer.msproftx_viewer import MsprofTxViewer
from viewer.npu_mem.npu_mem_viewer import NpuMemViewer
from viewer.npu_mem.npu_op_mem_viewer import NpuOpMemViewer
from viewer.npu_mem.npu_mem_rec_viewer import NpuMemRecViewer
from viewer.npu_mem.npu_module_mem_viewer import NpuModuleMemViewer
from viewer.peripheral_report import get_peripheral_dvpp_data
from viewer.peripheral_report import get_peripheral_nic_data
from viewer.pipeline_overlap_viewer import PipelineOverlapViewer
from viewer.qos_viewer import StarsQosViewer
from viewer.runtime_report import get_task_scheduler_data
from viewer.soc_pmu_viewer import SocPmuViewer
from viewer.stars.acc_pmu_viewer import AccPmuViewer
from viewer.stars.block_detail_viewer import BlockDetailViewer
from viewer.stars.low_power_viewer import LowPowerViewer
from viewer.stars.sio_viewer import SioViewer
from viewer.stars.fusion_task_viewer import FusionTaskViewer
from viewer.stars.stars_chip_trans_view import StarsChipTransView
from viewer.stars.stars_soc_view import StarsSocView
from viewer.static_op_mem_viewer import StaticOpMemViewer
from viewer.task_time_viewer import TaskTimeViewer
from viewer.training.step_trace_viewer import StepTraceViewer
from viewer.training.task_op_viewer import TaskOpViewer
from viewer.ts_cpu_report import TsCpuReport
from viewer.lpm.ai_core_freq_viewer import AiCoreFreqViewer
from viewer.qos_viewer import QosViewer
from viewer.ub_viewer import UBViewer
from viewer.ccu.ccu_mission_viewer import CCUMissionViewer
from viewer.ccu.ccu_channel_viewer import CCUChannelViewer
from viewer.biu_perf.biu_perf_chip6_viewer import BiuPerfChip6Viewer
from viewer.lpm.voltage_viewer import VoltageViewer


class MsProfExportDataUtils:
    """
    Export data such as csv or json
    """

    cfg_parser = None
    init_cfg_finished = False
    LOCK = threading.Lock()

    @staticmethod
    def _get_task_time_data(configs: dict, params: dict) -> any:
        """
        get task scheduler data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return MsProfExportDataUtils._get_task_timeline(configs, params)
        if ChipManager().is_chip_v1():
            db_path = PathManager.get_db_path(
                params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB)
            )
            return get_task_scheduler_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs, params)
        message = {
            'host_id': MsProfCommonConstant.DEFAULT_IP,
            'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
            'result_dir': params.get(StrConstant.PARAM_RESULT_DIR),
        }
        return TaskOpViewer.get_task_op_summary(message)

    @staticmethod
    def _get_ddr_data(configs: dict, params: dict) -> any:
        """
        get ddr data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            ddr_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME,
            }
            result = get_ddr_timeline(ddr_param)
            return result

        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        return get_ddr_data(db_path, params.get(StrConstant.PARAM_DEVICE_ID), configs)

    @staticmethod
    def _get_cpu_usage_data(configs: dict, params: dict) -> any:
        """
        get system cpu usage data
        """
        db_path = PathManager.get_db_path(
            params.get(StrConstant.PARAM_RESULT_DIR), DBNameConstant.DB_HOST_SYS_USAGE_CPU
        )
        if params.get(StrConstant.DATA_TYPE) == "cpu_usage":
            return get_sys_cpu_usage_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        if params.get(StrConstant.DATA_TYPE) == "process_cpu_usage":
            return get_process_cpu_usage(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_cpu_top_funcs(configs: dict, params: dict) -> any:
        """
        get cpu pmu events
        """
        return get_cpu_hot_function(
            params.get(StrConstant.PARAM_RESULT_DIR),
            configs.get(StrConstant.CONFIG_DB),
            configs.get(StrConstant.CONFIG_TABLE),
            configs.get(StrConstant.CONFIG_HEADERS),
        )

    @staticmethod
    def _get_ts_cpu_top_funcs(configs: dict, params: dict) -> any:
        """
        get ts cpu top function
        """
        return TsCpuReport().get_output_top_function(
            configs.get(StrConstant.CONFIG_DB), params.get(StrConstant.PARAM_RESULT_DIR)
        )

    @staticmethod
    def _get_cpu_pmu_events(configs: dict, params: dict) -> any:
        """
        get cpu pmu events
        """
        if (
            params.get(StrConstant.PARAM_DATA_TYPE) == StrConstant.CTL_CPU_PMU
            or params.get(StrConstant.PARAM_DATA_TYPE) == StrConstant.AI_CPU_PMU
        ):
            return get_aictrl_pmu_events(
                params.get(StrConstant.PARAM_RESULT_DIR),
                configs.get(StrConstant.CONFIG_DB),
                configs.get(StrConstant.CONFIG_TABLE),
                configs.get(StrConstant.CONFIG_HEADERS),
            )
        if params.get(StrConstant.PARAM_DATA_TYPE) == StrConstant.TS_CPU_PMU:
            return get_ts_pmu_events(
                params.get(StrConstant.PARAM_RESULT_DIR),
                configs.get(StrConstant.CONFIG_DB),
                configs.get(StrConstant.CONFIG_TABLE),
                configs.get(StrConstant.CONFIG_HEADERS),
            )
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_memory_data(configs: dict, params: dict) -> any:
        """
        get memory data
        """
        db_path = PathManager.get_db_path(
            params.get(StrConstant.PARAM_RESULT_DIR), DBNameConstant.DB_HOST_SYS_USAGE_MEM
        )
        if params.get(StrConstant.PARAM_DATA_TYPE) == "sys_mem":
            return get_sys_mem_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        if params.get(StrConstant.PARAM_DATA_TYPE) == "process_mem":
            return get_process_mem_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_op_summary_data(configs: dict, params: dict) -> any:
        """
        get ai core op summary detail table
        """
        db_name = DBNameConstant.DB_AICORE_OP_SUMMARY
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        return AiCoreOpReport.get_op_summary_data(params.get(StrConstant.PARAM_RESULT_DIR), db_path, configs)

    @staticmethod
    def _get_l2_cache_data(configs: dict, params: dict) -> tuple:
        """
        get l2 cache data
        """
        db_name = configs.get(StrConstant.CONFIG_DB)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        headers, data, count = get_l2_cache_data(
            db_path, configs.get(StrConstant.CONFIG_TABLE), configs.get(StrConstant.CONFIG_UNUSED_COLS)
        )
        if headers and data:
            data = process_hit_rate(headers, data)
            op_dict = DataManager.get_op_dict(params.get(StrConstant.PARAM_RESULT_DIR))
            if op_dict:
                if add_op_name(headers, data, op_dict):
                    headers.append("Op Name")
        return headers, data, count

    @staticmethod
    def _get_step_trace_data(configs: dict, params: dict) -> any:
        """
        get training trace data for desired job ID
        """
        message = {
            "device_id": params.get(StrConstant.PARAM_DEVICE_ID),
            'host_id': MsProfCommonConstant.DEFAULT_IP,
            'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
        }
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return StepTraceViewer.get_step_trace_timeline(message)
        else:
            reduce_headers, data, count = StepTraceViewer.get_step_trace_summary(message)
            headers = configs[StrConstant.CONFIG_HEADERS] + reduce_headers
            return headers, data, count

    @staticmethod
    def _get_api_statistic_data(configs: dict, params: dict) -> any:
        """
        get API(acl/hccl/model/node/runtime) statistic data
        """
        return ApiStatisticViewer(configs, params).get_api_statistic_data()

    @staticmethod
    def _get_op_statistic_data(configs: dict, params: dict) -> any:
        """
        get op statistic data
        """
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        return ReportOPCounter.report_op(db_path, configs.get(StrConstant.CONFIG_HEADERS))

    @staticmethod
    def _get_hccl_statistic_data(configs: dict, params: dict) -> any:
        """
        get hccl statistic data
        """
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        return ReportHcclStatisticData.report_hccl_op(db_path, configs.get(StrConstant.CONFIG_HEADERS))

    @staticmethod
    def _get_dvpp_data(configs: dict, params: dict) -> any:
        """
        get dvpp data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            dvpp_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'dvppid': 'all',
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME,
            }
            result = get_dvpp_timeline(dvpp_param)
            return result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        if data_type == StrConstant.DVPP_DATA:
            return get_peripheral_dvpp_data(
                db_path, configs.get(StrConstant.CONFIG_TABLE), params.get(StrConstant.PARAM_DEVICE_ID), configs
            )
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_nic_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            nic_result = get_network_timeline(
                params.get(StrConstant.PARAM_RESULT_DIR),
                params.get(StrConstant.PARAM_DEVICE_ID),
                NumberConstant.DEFAULT_START_TIME,
                NumberConstant.DEFAULT_END_TIME,
                params.get(StrConstant.DATA_TYPE),
            )
            return nic_result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        if data_type == StrConstant.NIC_DATA:
            return get_peripheral_nic_data(
                db_path, configs.get(StrConstant.CONFIG_TABLE), params.get(StrConstant.PARAM_DEVICE_ID), configs
            )
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_roce_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            roce_result = get_network_timeline(
                params.get(StrConstant.PARAM_RESULT_DIR),
                params.get(StrConstant.PARAM_DEVICE_ID),
                NumberConstant.DEFAULT_START_TIME,
                NumberConstant.DEFAULT_END_TIME,
                params.get(StrConstant.DATA_TYPE),
            )
            return roce_result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        if data_type == StrConstant.ROCE_DATA:
            return get_peripheral_nic_data(
                db_path, configs.get(StrConstant.CONFIG_TABLE), params.get(StrConstant.PARAM_DEVICE_ID), configs
            )
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_hccs_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            result = get_hccs_timeline(
                params.get(StrConstant.PARAM_RESULT_DIR),
                params.get(StrConstant.PARAM_DEVICE_ID),
                NumberConstant.DEFAULT_START_TIME,
                NumberConstant.DEFAULT_END_TIME,
            )
            return result

        hccs_view = InterConnectionView(params.get(StrConstant.PARAM_RESULT_DIR), {})
        return hccs_view.get_hccs_data(params.get(StrConstant.PARAM_DEVICE_ID))

    @staticmethod
    def __get_mini_llc_data(sample_config: dict, params: dict) -> any:
        """
        get mini llc data, contains bandwidth and capacity
        """
        if sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM:
            return get_llc_bandwidth(params.get(StrConstant.PARAM_RESULT_DIR), params.get(StrConstant.PARAM_DEVICE_ID))
        if not sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM:
            return MsvpConstant.MSVP_EMPTY_DATA
        if not (
            params.get(StrConstant.DATA_TYPE) == StrConstant.LLC_AICPU
            or params.get(StrConstant.DATA_TYPE) == StrConstant.LLC_CTRLCPU
        ):
            return MsvpConstant.MSVP_EMPTY_DATA
        types = StrConstant.AICPU
        if params.get(StrConstant.DATA_TYPE) != StrConstant.LLC_AICPU:
            types = StrConstant.CTRL_CPU
        return llc_capacity_data(
            params.get(StrConstant.PARAM_RESULT_DIR), params.get(StrConstant.PARAM_DEVICE_ID), types
        )

    @staticmethod
    def __get_non_mini_llc_data(sample_config: dict, params: dict) -> any:
        """
        get non mini llc data, contains read and write
        """
        result = get_llc_train_summary(
            params.get(StrConstant.PARAM_RESULT_DIR), sample_config, params.get(StrConstant.PARAM_DEVICE_ID)
        )
        res = json.loads(result)
        if res['status'] == NumberConstant.SUCCESS:
            llc_data = res['data']['table']
            llc_header = ['Mode', 'Task', 'Hit Rate(%)', 'Throughput(MB/s)']
            llc_data = [[item[key] for key in llc_header] for item in llc_data]
            return llc_header, llc_data, len(llc_data)
        return (
            [],
            json.dumps(
                {
                    "status": NumberConstant.ERROR,
                    "info": "Failed to get LLC data(%s) in non-mini scene." % (params.get(StrConstant.DATA_TYPE)),
                }
            ),
            0,
        )

    @staticmethod
    def _get_llc_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_llc_timeline(params)
        sample_config = ConfigMgr.read_sample_config(params.get(StrConstant.PARAM_RESULT_DIR))
        if sample_config:
            if ChipManager().is_chip_v1():
                return MsProfExportDataUtils.__get_mini_llc_data(sample_config, params)
            return MsProfExportDataUtils.__get_non_mini_llc_data(sample_config, params)
        return (
            [],
            json.dumps(
                {
                    "status": NumberConstant.ERROR,
                    "info": "Failed to get LLC sample config data(%s)." % (params.get(StrConstant.DATA_TYPE)),
                }
            ),
            0,
        )

    @staticmethod
    def _get_hbm_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            hbm_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'hbm_id': 0,
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME,
            }
            result = get_hbm_timeline(hbm_param)
            return result
        data = get_hbm_summary_data(params.get(StrConstant.PARAM_RESULT_DIR), params.get(StrConstant.PARAM_DEVICE_ID))
        if data:
            if isinstance(data, list):
                return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
            return MsvpConstant.MSVP_EMPTY_DATA
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_pcie_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            pcie_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME,
            }
            result = get_pcie_timeline(pcie_param)
            return result
        return InterConnectionView(params.get(StrConstant.PARAM_RESULT_DIR), {}).get_pcie_summary_data()

    @staticmethod
    def _get_aicpu_data(configs: dict, params: dict) -> any:
        if ChipManager().is_sqe_id_supported():
            aicpu_data = ParseAiCpuData.analysis_aicpu_by_sqe_id(
                params.get(StrConstant.PARAM_RESULT_DIR), params.get(StrConstant.PARAM_ITER_ID)
            )
        else:
            aicpu_data = ParseAiCpuData.analysis_aicpu(
                params.get(StrConstant.PARAM_RESULT_DIR), params.get(StrConstant.PARAM_ITER_ID)
            )
        return configs.get(StrConstant.CONFIG_HEADERS), aicpu_data, len(aicpu_data)

    @staticmethod
    def _get_dp_data(configs: dict, params: dict) -> any:
        _ = configs
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        device_id = params.get(StrConstant.PARAM_DEVICE_ID)

        dp_result = ParseDpData.analyse_dp(PathManager.get_data_dir(result_dir), device_id)
        dp_headers = configs.get('headers')
        if dp_result:
            return dp_headers, dp_result, len(dp_result)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_fusion_op_data(configs: dict, params: dict) -> any:
        return get_ge_model_data(params, configs.get(StrConstant.CONFIG_TABLE), configs)

    @staticmethod
    def _get_host_cpu_usage_data(configs: dict, params: dict) -> any:
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_host_prof_timeline(result_dir, HostExportType.CPU_USAGE)
        else:
            return get_host_prof_summary(result_dir, HostExportType.CPU_USAGE, configs)

    @staticmethod
    def _get_host_mem_usage_data(configs: dict, params: dict) -> any:
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_host_prof_timeline(result_dir, HostExportType.MEM_USAGE)
        else:
            return get_host_prof_summary(result_dir, HostExportType.MEM_USAGE, configs)

    @staticmethod
    def _get_host_network_usage_data(configs: dict, params: dict) -> any:
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_host_prof_timeline(result_dir, HostExportType.NETWORK_USAGE)
        else:
            return get_host_prof_summary(result_dir, HostExportType.NETWORK_USAGE, configs)

    @staticmethod
    def _get_host_disk_usage_data(configs: dict, params: dict) -> any:
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_host_prof_timeline(result_dir, HostExportType.DISK_USAGE)
        else:
            return get_host_prof_summary(result_dir, HostExportType.DISK_USAGE, configs)

    @staticmethod
    def _get_host_runtime_api(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            data = HostSyscallPresenter.get_summary_api_info(params.get(StrConstant.PARAM_RESULT_DIR))
            if data:
                return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
            return MsvpConstant.MSVP_EMPTY_DATA
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR), HostExportType.HOST_RUNTIME_API)

    @staticmethod
    def _get_bulk_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) != MsProfCommonConstant.TIMELINE:
            logging.warning("Please check params, currently bulk data export params should be timeline.")
            return []
        row_timeline = PipelineOverlapViewer(configs, params).get_timeline_data()
        MsprofTimeline().add_export_data(row_timeline, params.get(StrConstant.PARAM_DATA_TYPE))

        return MsprofTimeline().export_all_data()

    @staticmethod
    def _get_freq_data(configs: dict, params: dict) -> any:
        freq_list = AiCoreFreqViewer(params).get_all_data()
        return freq_list

    @staticmethod
    def _get_voltage_data(configs: dict, params: dict) -> any:
        voltage_list = []
        # only chip v4 support voltage info data
        if ChipManager().is_chip_v4():
            voltage_list = VoltageViewer(params).get_all_data()
        return voltage_list

    @staticmethod
    def _get_task_timeline(configs: dict, params: dict) -> list:
        """
        get ffts task time data
        """
        return TaskTimeViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_hccl_timeline(configs: dict, params: dict) -> list:
        """
        get hccl timeline data
        :param configs:
        :param params:
        :return:
        """
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return HCCLExport(params).get_hccl_timeline_data()
        logging.warning(
            "Please check params, currently hccl data does not support exporting files other than timeline."
        )
        return []

    @staticmethod
    def _get_msproftx_data(configs: dict, params: dict) -> any:
        if params.get("project").endswith('host'):
            if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
                return MsprofTxViewer(configs, params).get_summary_data()
            return MsprofTxViewer(configs, params).get_timeline_data()
        else:
            if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
                return MsprofTxViewer(configs, params).get_device_summary_data()
            return MsprofTxViewer(configs, params).get_device_timeline_data()

    @staticmethod
    def _get_stars_soc_data(configs: dict, params: dict) -> any:
        return StarsSocView(configs, params).get_timeline_data()

    @staticmethod
    def _get_stars_chip_trans_data(configs: dict, params: dict) -> any:
        return StarsChipTransView(configs, params).get_timeline_data()

    @staticmethod
    def _get_low_power_data(configs: dict, params: dict) -> any:
        return LowPowerViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_biu_perf_timeline(configs: dict, params: dict) -> any:
        _ = configs
        if ChipManager().is_chip_v6():
            return BiuPerfChip6Viewer(configs, params).get_timeline_data()
        return BiuPerfViewer(params.get(StrConstant.PARAM_RESULT_DIR)).get_timeline()

    @staticmethod
    def _get_acc_pmu(configs: dict, params: dict) -> any:
        return AccPmuViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_npu_mem_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return NpuMemViewer(configs, params).get_timeline_data()
        return NpuMemViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_operator_memory_data(configs: dict, params: dict) -> any:
        return NpuOpMemViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_mem_record_data(configs: dict, params: dict) -> any:
        return NpuMemRecViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_npu_module_mem_data(configs: dict, params: dict) -> any:
        return NpuModuleMemViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_api_data(configs: dict, params: dict) -> any:
        return ApiViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_sio_data(configs: dict, params: dict) -> any:
        return SioViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_fusion_task_data(configs: dict, params: dict) -> any:
        configs['result_dir'] = params.get(StrConstant.PARAM_RESULT_DIR)
        return FusionTaskViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_aicpu_mi_data(configs: dict, params: dict) -> any:
        data = ParseAiCpuData.get_aicpu_mi_data(params.get(StrConstant.PARAM_RESULT_DIR))
        return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)

    @staticmethod
    def _get_qos_data(configs: dict, params: dict) -> any:
        if ChipManager().is_chip_v4():
            return QosViewer(configs, params).get_timeline_data()
        else:
            return StarsQosViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_static_op_mem_data(configs: dict, params: dict) -> any:
        return StaticOpMemViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_ccu_mission_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return CCUMissionViewer(configs, params).get_timeline_data()
        return CCUMissionViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_ccu_channel_data(configs: dict, params: dict) -> any:
        return CCUChannelViewer(configs, params).get_summary_data()

    @classmethod
    def export_data(cls: any, params: dict) -> str:
        """
        export data, support different data types and export types
        :param params: param object
        :return: result of export
        """
        if params.get(StrConstant.DATA_TYPE) is None:
            return json.dumps({"status": NumberConstant.ERROR, "info": "Parameter data_type is none."})
        if not cls.init_cfg_finished:
            cls.LOCK.acquire()  # pylint: disable=consider-using-with
            if not cls.init_cfg_finished:
                cls._load_export_data_config()
            cls.LOCK.release()
        configs = cls._get_configs_with_data_type(params.get(StrConstant.PARAM_DATA_TYPE))
        if configs.get(StrConstant.CONFIG_HANDLER) is not None and hasattr(
            cls, configs.get(StrConstant.CONFIG_HANDLER)
        ):
            handler = getattr(cls, configs.get(StrConstant.CONFIG_HANDLER))
            if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
                headers, data, _ = handler(configs, params)
                return MsprofDataStorage().export_summary_data(headers, data, params)
            timeline_data = handler(configs, params)
            cls.add_timeline_data(params, timeline_data)
            skip_list = ["event", "api", "sio"]
            if params.get(StrConstant.PARAM_DATA_TYPE) in skip_list:
                return json.dumps({"status": NumberConstant.SKIP})
            return MsprofDataStorage().export_timeline_data_to_json(timeline_data, params)
        return json.dumps(
            {
                "status": NumberConstant.ERROR,
                "info": "Unable to handler data type %s." % params.get(StrConstant.PARAM_DATA_TYPE),
            }
        )

    @classmethod
    def add_timeline_data(cls: any, params: dict, data: any) -> None:
        """
        add timeline data to bulk
        :param params:
        :param data:
        :return:
        """
        filter_list = [
            "msprof",
            "ffts_sub_task_time",
        ]
        if not ProfilingScene().is_all_export():
            filter_list.append("step_trace")
        if params.get(StrConstant.PARAM_DATA_TYPE) not in filter_list:
            MsprofTimeline().add_export_data(data, params.get(StrConstant.PARAM_DATA_TYPE))

    @classmethod
    def _load_export_data_config(cls: any) -> None:
        """
        load export configuration
        :return: None
        """
        cls.cfg_parser = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA)
        cls.init_cfg_finished = True

    @classmethod
    def _get_configs_with_data_type(cls: any, data_type: str) -> dict:
        """
        get configs according to data type
        :param data_type: data type
        :return: config parser
        """
        configs = {}
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_HANDLER):
                configs[StrConstant.CONFIG_HANDLER] = cls.cfg_parser.get(data_type, StrConstant.CONFIG_HANDLER)

            configs[StrConstant.CONFIG_HEADERS] = []
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_HEADERS):
                headers = cls.cfg_parser.get(data_type, StrConstant.CONFIG_HEADERS)
                if headers:
                    configs[StrConstant.CONFIG_HEADERS] = headers.split(",")

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_DB)

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_COLUMNS)

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_TABLE)

            configs[StrConstant.CONFIG_UNUSED_COLS] = []
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_UNUSED_COLS):
                unused_cols = cls.cfg_parser.get(data_type, StrConstant.CONFIG_UNUSED_COLS)
                if unused_cols:
                    configs[StrConstant.CONFIG_UNUSED_COLS] = unused_cols.split(",")
        return configs

    @classmethod
    def _get_configs_with_option(cls: any, configs: dict, data_type: str, option: str) -> None:
        if cls.cfg_parser.has_option(data_type, option):
            configs[option] = cls.cfg_parser.get(data_type, option)

    @classmethod
    def _get_ai_core_sample_based_data(cls: any, configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_aicore_utilization_timeline(params.get(StrConstant.PARAM_RESULT_DIR, ""))
        params[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
        return AiCoreReport.get_core_sample_data(
            params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB), params
        )

    @classmethod
    def _get_aiv_sample_based_data(cls: any, configs: dict, params: dict) -> any:
        params[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
        return AiCoreReport.get_core_sample_data(
            params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB), params
        )

    @classmethod
    def _get_block_detail_data(cls: any, configs: dict, params: dict) -> any:
        return BlockDetailViewer(configs, params).get_timeline_data()

    @classmethod
    def _get_ub_data(cls: any, configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return UBViewer(configs, params).get_timeline_data()
        return UBViewer(configs, params).get_summary_data()

    @classmethod
    def _get_soc_pmu_data(cls: any, configs: dict, params: dict) -> any:
        return SocPmuViewer(configs, params).get_summary_data()

    @classmethod
    def _get_dpu_data(cls: any, configs: dict, params: dict) -> any:
        return DPUViewer(configs, params).get_timeline_data()
