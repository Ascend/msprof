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

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.basic_info.base_info import BaseInfo


class DeviceInfo(BaseInfo):
    """
    device info
    """

    DEVICE_INFO = "DeviceInfo"

    # The json keys under the DeviceInfo
    KEY_DEVICE_ID = "id"
    KEY_CTRL_CPU_ID = "ctrl_cpu_id"
    KEY_CTRL_CPU_CORE_NUM = "ctrl_cpu_core_num"
    KEY_TS_CPU_CORE_NUM = "ts_cpu_core_num"
    KEY_AI_CPU_CORE_NUM = "ai_cpu_core_num"
    KEY_AI_CORE_NUM = "ai_core_num"

    def __init__(self: any) -> None:
        super().__init__()
        self.device_id = 0
        self.ai_cpu_num = 0
        self.ai_core_num = 0
        self.control_cpu_num = 0
        self.control_cpu_type = ""
        self.ts_cpu_num = 0

    def run(self: any, _: str) -> None:
        """
        run data
        :return: None
        """
        if not InfoConfReader().get_root_data(self.DEVICE_INFO):
            # clear all Member variables to remind the key of device_info.
            self.__dict__.clear()
        else:
            self.merge_data()

    def merge_data(self: any) -> None:
        """
        merge data
        :return:None
        """
        self.device_id = InfoConfReader().get_data_under_device(self.KEY_DEVICE_ID)
        self.control_cpu_type = InfoConfReader().get_data_under_device(self.KEY_CTRL_CPU_ID)
        self.control_cpu_num = InfoConfReader().get_data_under_device(self.KEY_CTRL_CPU_CORE_NUM)
        self.ts_cpu_num = InfoConfReader().get_data_under_device(self.KEY_TS_CPU_CORE_NUM)
        self.ai_cpu_num = InfoConfReader().get_data_under_device(self.KEY_AI_CPU_CORE_NUM)
        self.ai_core_num = InfoConfReader().get_data_under_device(self.KEY_AI_CORE_NUM)
