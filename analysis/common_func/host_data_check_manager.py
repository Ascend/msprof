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
from common_func.data_check_manager import DataCheckManager


class HostDataCheckManager(DataCheckManager):
    """
    The system data check manager
    """

    @classmethod
    def contain_runtime_api_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain runtime api data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_os_runtime_api_compiles(), device_id=device_id)

    @classmethod
    def contain_host_cpuusage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain cpu usage data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_host_cpu_usage_compiles(), device_id=device_id)

    @classmethod
    def contain_host_mem_usage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain mem data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_host_mem_usage_compiles(), device_id=device_id)

    @classmethod
    def contain_host_network_usage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain network data or not
        """
        return cls.check_data_exist(
            result_dir, file_name_manager.get_host_network_usage_compiles(), device_id=device_id
        )

    @classmethod
    def contain_host_disk_usage_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain disk usage data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_host_disk_usage_compiles(), device_id=device_id)
