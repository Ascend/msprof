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

import os

from common_func import file_name_manager
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from msmodel.stars.ffts_pmu_model import V6PmuViewModel


class AiStackDataCheckManager(DataCheckManager):
    """
    The ai stack data check manager
    """

    @staticmethod
    def _is_device_exist(result_dir: str) -> bool:
        parent_dir = os.path.dirname(result_dir)
        subdir_names = os.listdir(parent_dir)
        for subdir_name in subdir_names:
            if not subdir_name.startswith('device_'):
                continue
            return True
        return False

    @staticmethod
    def _check_output(result_dir: str, device_id: any = None) -> bool:
        if device_id != NumberConstant.HOST_ID or (
            result_dir.endswith("host") and not AiStackDataCheckManager._is_device_exist(result_dir)
        ):
            return True
        return False

    @classmethod
    def contain_fusion_op_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        if cls.check_export_with_so():
            return False
        return (
            AiStackDataCheckManager._check_output(result_dir, device_id)
            and DBManager.check_connect_db(result_dir, DBNameConstant.DB_GE_MODEL_INFO)[0]
            and DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_GE_MODEL_INFO),
                DBNameConstant.TABLE_GE_FUSION_OP_INFO,
                DBNameConstant.TABLE_MODEL_NAME,
            )
        )

    @classmethod
    def contain_l2_cache_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain l2_cache data or not
        """
        return cls.check_data_exist(
            result_dir, file_name_manager.get_l2_cache_compiles(), device_id=device_id
        ) or cls.check_data_exist(result_dir, file_name_manager.get_soc_pmu_compiles(), device_id=device_id)

    @classmethod
    def contain_ai_core_sample_based(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ai core data or not
        """
        return bool(path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_NAME_AICORE))) or (
            cls.check_data_exist(result_dir, file_name_manager.get_ai_core_compiles(), device_id=device_id)
            and ConfigMgr.is_ai_core_sample_based(result_dir)
        )

    @classmethod
    def contain_aiv_core_sample_based(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ai core data or not
        """
        return (
            bool(path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_NAME_AI_VECTOR_CORE)))
            or cls.check_data_exist(result_dir, file_name_manager.get_aiv_compiles(), device_id=device_id)
            and ConfigMgr.is_aiv_sample_based(result_dir)
        )

    @classmethod
    def contain_training_trace_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain training trace data or not
        """
        return bool(path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_TRACE)))

    @classmethod
    def contain_dp_aicpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain aicpu data or not
        aicpu data by hdc and all aicpu data by driver
        """
        return cls.check_data_exist(
            result_dir, file_name_manager.get_data_preprocess_compiles('AICPU'), device_id=device_id
        ) or cls.check_data_exist(result_dir, file_name_manager.get_aicpu_compiles(), device_id=device_id)

    @classmethod
    def contain_data_preprocess_dp_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain dp data or not
        dp data by hdc and all aicpu data by driver
        """
        return cls.check_data_exist(
            result_dir, file_name_manager.get_data_preprocess_compiles('DP'), device_id=device_id
        ) or cls.check_data_exist(result_dir, file_name_manager.get_aicpu_compiles(), device_id=device_id)

    @classmethod
    def contain_task_time_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain task_time data or not
        """
        return AiStackDataCheckManager._check_output(result_dir, device_id) and (
            cls._contain_ts_track_data(result_dir, device_id=device_id)
            or cls._contain_hwts_data(result_dir, device_id=device_id)
            or cls.contain_stars_soc_data(result_dir, device_id=device_id)
            or (
                cls._contain_hwts_aiv_data(result_dir, device_id=device_id)
                or cls._contain_ts_track_aiv_data(result_dir, device_id=device_id)
            )
            or cls._contain_v5_data(result_dir, device_id=device_id)
        )

    @classmethod
    def contain_task_time_task(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        if cls.check_export_with_so():
            return False
        return cls.contain_task_time_data(result_dir, device_id) or AiStackDataCheckManager.contain_stars_soc_data(
            result_dir, device_id
        )

    @classmethod
    def contain_op_summary_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        if (
            not AiStackDataCheckManager._check_output(result_dir, device_id)
            or not DBManager.check_connect_db(result_dir, DBNameConstant.DB_AICORE_OP_SUMMARY)[0]
        ):
            return False
        if cls.check_export_with_so():
            return False
        return True

    @classmethod
    def contain_op_summary_without_ge_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        return not cls._contain_ge_task_data(result_dir, device_id=device_id) and cls.contain_task_time_data(
            result_dir, device_id=device_id
        )

    @classmethod
    def contain_op_statistic_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        if cls.check_export_with_so() or InfoConfReader().is_level0():
            return False
        return (
            AiStackDataCheckManager._check_output(result_dir, device_id)
            and DBManager.check_connect_db(result_dir, DBNameConstant.DB_OP_COUNTER)[0]
        )

    @classmethod
    def contain_api_statistic_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain api data or not
        """
        if cls.check_export_with_so():
            return False
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_API_EVENT), DBNameConstant.TABLE_API_DATA
        )

    @classmethod
    def contain_core_cpu_reduce_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.contain_task_time_data(result_dir, device_id=device_id) or cls.contain_training_trace_data(
            result_dir
        )

    @classmethod
    def contain_stars_soc_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars soc log data or not
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained stars soc data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_soc_log_compiles(), device_id=device_id)

    @classmethod
    def contain_sub_task_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_SOC_LOG), DBNameConstant.TABLE_FFTS_LOG
        )

    @classmethod
    def contain_hccl_hcom_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain hccl hcom data or not
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained hccl hcom data, true or false
        """
        return AiStackDataCheckManager._check_output(result_dir, device_id) and (
            DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE),
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
            )
            or DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE), DBNameConstant.TABLE_KFC_OP
            )
        )

    @classmethod
    def contain_training_trace_data_or_step(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain training trace data or step trace data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained training trace data or step trace data, true or false
        """
        return cls.contain_training_trace_data(result_dir, device_id=device_id)

    @classmethod
    def contain_step_trace_summary_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain training trace data or step trace data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained training trace data or step trace data, true or false
        """
        if cls.check_export_with_so():
            return False
        return cls.contain_training_trace_data(result_dir, device_id=device_id)

    @classmethod
    def contain_msproftx_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain msproftx data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls.check_data_exist(
            result_dir,
            file_name_manager.get_msproftx_compiles(),
        ) or DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_STEP_TRACE), DBNameConstant.TABLE_STEP_TRACE
        )

    @classmethod
    def contain_stars_soc_profiler_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars soc profile data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) and path_check(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_STARS_SOC)
        )

    @classmethod
    def contain_stars_chip_trans_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain msproftx data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) and path_check(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_STARS_CHIP_TRANS)
        )

    @classmethod
    def contain_acc_pmu_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain accpmu data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained accpmu data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) and bool(
            path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_ACC_PMU))
        )

    @classmethod
    def contain_sio_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain sio data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained sio data
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) and path_check(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_SIO)
        )

    @classmethod
    def contain_stars_low_power_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain lowpower sample data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained lowpower sample data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id) and path_check(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_LOW_POWER)
        )

    @classmethod
    def contain_biu_perf_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain biu perf data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained biu data, true or false
        """
        return (
            cls._check_output(result_dir, device_id)
            and DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_BIU_PERF), DBNameConstant.TABLE_BIU_INSTR_STATUS
            )
            or cls.check_data_exist(result_dir, file_name_manager.get_biu_compiles(), device_id=device_id)
        )

    @classmethod
    def contain_op_mem_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain npu op mem data or not
        """
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_MEMORY_OP), DBNameConstant.TABLE_NPU_OP_MEM
        )

    @classmethod
    def contain_mem_rec_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain memory record data or not
        """
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_MEMORY_OP), DBNameConstant.TABLE_NPU_OP_MEM_REC
        )

    @classmethod
    def contain_npu_module_mem_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain npu module mem data or not
        """
        if cls.check_export_with_so():
            return False
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_NPU_MODULE_MEM), DBNameConstant.TABLE_NPU_MODULE_MEM
        )

    @classmethod
    def contain_api_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain api data or not
        """
        return AiStackDataCheckManager._check_output(result_dir, device_id) and (
            DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_API_EVENT), DBNameConstant.TABLE_API_DATA
            )
            or DBManager.check_tables_in_db(
                PathManager.get_db_path(result_dir, DBNameConstant.DB_API_EVENT), DBNameConstant.TABLE_EVENT_DATA
            )
        )

    @classmethod
    def contain_hccl_statistic_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain hccl statistic data or not
        """
        if cls.check_export_with_so() or InfoConfReader().is_level0():
            return False
        return AiStackDataCheckManager._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE),
            DBNameConstant.TABLE_HCCL_OP_REPORT,
        )

    @classmethod
    def contain_static_op_mem_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain static_op_mem data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_static_op_mem_compiles(), device_id=None)

    @classmethod
    def contain_block_log_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain block log data or not
        """
        if not ChipManager().is_chip_v6():
            return False

        with V6PmuViewModel(result_dir) as _model:
            if _model.check_table():
                return True

        return cls._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_SOC_LOG), DBNameConstant.TABLE_BLOCK_LOG
        )

    @classmethod
    def contain_ub_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ub data or not
        """
        if not ChipManager().is_chip_v6():
            return False

        return cls._check_output(result_dir, device_id) and DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_UB), DBNameConstant.TABLE_UB_BW
        )

    @classmethod
    def contain_ccu_mission_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ccu mission data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ccu_mission_compiles(), device_id=device_id)

    @classmethod
    def _contain_stars_profiler_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars_soc_profiler data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_soc_profiler_compiles(), device_id=device_id)

    @classmethod
    def _contain_ge_task_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.task_desc_info data or not
        """
        return DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_RTS_TRACK), DBNameConstant.TABLE_TASK_TRACK
        )

    @classmethod
    def _contain_hwts_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hwts_log data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hwts_compiles(), device_id=device_id)

    @classmethod
    def _contain_hwts_aiv_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hwts_log data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hwts_vector_compiles(), device_id=device_id)

    @classmethod
    def _contain_ts_track_aiv_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ts_track_aiv_compiles(), device_id=device_id)

    @classmethod
    def _contain_ts_track_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ts_track_compiles(), device_id=device_id)

    @classmethod
    def _contain_v5_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain v5_device data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_v5_stars_profile_compiles(), device_id=device_id)

    @classmethod
    def contain_dpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain dpu data or not
        """
        return cls.check_data_exist(
            result_dir, file_name_manager.get_dpu_track_compact_compiles()
        ) or cls.check_data_exist(result_dir, file_name_manager.get_dpu_hccl_track_compact_compiles())

    @classmethod
    def contain_fusion_task_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain fusion task data or not
        """
        return DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_FUSION_TASK), DBNameConstant.TABLE_FUSION_TASK
        )
