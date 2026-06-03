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

from common_func import file_name_manager
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager


class SystemDataCheckManager(DataCheckManager):
    """
    The system data check manager
    """

    @classmethod
    def contain_ddr_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ddr data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ddr_compiles(), device_id=device_id)

    @classmethod
    def contain_hbm_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hbm data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hbm_compiles(), device_id=device_id)

    @classmethod
    def contain_dvpp_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain dvpp data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_dvpp_compiles(), device_id=device_id)

    @classmethod
    def contain_nic_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain nic data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_nic_compiles(), device_id=device_id)

    @classmethod
    def contain_roce_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain roce data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_roce_compiles(), device_id=device_id)

    @classmethod
    def contain_hccs_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hccs data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hccs_compiles(), device_id=device_id)

    @classmethod
    def contain_llc_capacity_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain llc data or not
        """
        return ConfigMgr.has_llc_capacity(result_dir) and cls.check_data_exist(
            result_dir, file_name_manager.get_llc_compiles(), device_id=device_id
        )

    @classmethod
    def contain_read_write_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain llc data or not
        """
        return ConfigMgr.has_llc_read_write(result_dir) and cls.check_data_exist(
            result_dir, file_name_manager.get_llc_compiles(), device_id=device_id
        )

    @classmethod
    def contain_llc_bandwidth_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain llc data or not
        """
        return ConfigMgr.has_llc_bandwidth(result_dir) and cls.check_data_exist(
            result_dir, file_name_manager.get_llc_compiles(), device_id=device_id
        )

    @classmethod
    def contain_npu_mem_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain npu mem data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_npu_mem_compiles(), device_id=device_id)

    @classmethod
    def contain_npu_mem_summary_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain npu mem data or not
        """
        if cls.check_export_with_so():
            return False
        return cls.check_data_exist(result_dir, file_name_manager.get_npu_mem_compiles(), device_id=device_id)

    @classmethod
    def contain_pcie_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain pcie data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_pcie_compiles(), device_id=device_id)

    @classmethod
    def contain_cpu_usage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system cpu usage data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_sys_cpu_usage_compiles(), device_id=device_id)

    @classmethod
    def contain_pid_cpu_usage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain process cpu usage data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_pid_cpu_usage_compiles(), device_id=device_id)

    @classmethod
    def contains_sys_memory_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system memory data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_sys_mem_compiles(), device_id=device_id)

    @classmethod
    def contain_ai_cpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system memory data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ai_cpu_compiles(), device_id=device_id)

    @classmethod
    def contain_ctrl_cpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system memory data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ctrl_cpu_compiles(), device_id=device_id)

    @classmethod
    def contain_ts_cpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system memory data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ts_cpu_compiles(), device_id=device_id)

    @classmethod
    def contains_pid_memory_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain system memory data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_pid_mem_compiles(), device_id=device_id)

    @classmethod
    def contain_qos_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain qos data or not
        """
        return (
            cls.check_data_exist(result_dir, file_name_manager.get_qos_compiles(), device_id=device_id)
            or cls.check_data_exist(result_dir, file_name_manager.get_soc_profiler_compiles(), device_id=device_id)
        ) and path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_QOS))

    @classmethod
    def contain_ccu_channel_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ccu channel data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ccu_channel_compiles(), device_id=device_id)

    @classmethod
    def contain_soc_pmu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain soc pmu data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_soc_pmu_compiles(), device_id=device_id)

    @classmethod
    def contain_lpm_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain lpm data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_lpm_info_compiles(), device_id=device_id)
