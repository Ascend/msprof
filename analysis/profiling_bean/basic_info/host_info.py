# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.basic_info.base_info import BaseInfo
from profiling_bean.basic_info.cpu_info import CpuInfo


class HostInfo(BaseInfo):
    """
    host info class
    """

    HOST_CPU_INFO = "CPU"
    CPU_INDEX = "CPU{}"

    # The json keys under the host info
    HOST_OS = "OS"
    HOST_NAME = "hostname"
    CPU_ID = "Id"
    CPU_FREQUENCY = "Frequency"
    LOGICAL_CPU_COUNT = "Logical_CPU_Count"
    CPU_NAME = "Name"
    CPU_TYPE = "Type"

    def __init__(self: any) -> None:
        super().__init__()
        self.host_computer_name = ""
        self.host_operating_system = ""
        self.cpu_num = 0
        self.cpu_info = []

    def run(self: any, _: str) -> None:
        """
        run host info
        :return: None
        """
        self.merge_data()

    def merge_data(self: any) -> None:
        """
        merge data
        :return: None
        """
        self.host_operating_system = InfoConfReader().get_root_data(self.HOST_OS)

        self.host_computer_name = InfoConfReader().get_root_data(self.HOST_NAME)

        cpu_items = InfoConfReader().get_root_data(self.HOST_CPU_INFO)
        if cpu_items:
            for cpu_item in cpu_items:
                if cpu_item.get(self.CPU_ID, None) is None:
                    logging.error("Can't get cpu id from info.json, please check.")
                    continue
                cpu_info = CpuInfo(
                    self.CPU_INDEX.format(cpu_item.get(self.CPU_ID, "")),
                    cpu_item.get(self.CPU_FREQUENCY),
                    cpu_item.get(self.LOGICAL_CPU_COUNT),
                    cpu_item.get(self.CPU_NAME),
                    cpu_item.get(self.CPU_TYPE),
                )
                self.cpu_num += int(cpu_item.get(self.LOGICAL_CPU_COUNT))
                self.cpu_info.append(cpu_info)
