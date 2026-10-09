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
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.basic_info.device_info import DeviceInfo

NAMESPACE = 'profiling_bean.basic_info.device_info'

info_json = {"DeviceInfo": [
    {"id": 1, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55", "ctrl_cpu_core_num": 4,
     "ctrl_cpu_endian_little": 1,
     "ts_cpu_core_num": 1, "ai_cpu_core_num": 4, "ai_core_num": 2, "ai_cpu_core_id": 4, "ai_core_id": 0,
     "aicpu_occupy_bitmap": 240, "ctrl_cpu": "0,1,2,3", "ai_cpu": "4,5,6,7", "aiv_num": 0,
     "hwts_frequency": "19.2",
     "aic_frequency": "680", "aiv_frequency": "1000"}]}


class TestDeviceInfo(unittest.TestCase):
    def test_run_1(self):
        InfoConfReader()._info_json = {}
        DeviceInfo().run("")

    def test_run_2(self):
        InfoConfReader()._info_json = info_json
        with mock.patch(NAMESPACE + '.DeviceInfo.merge_data'):
            DeviceInfo().run("")

    def test_merge_data_1(self):
        InfoConfReader()._info_json = info_json
        DeviceInfo().merge_data()


if __name__ == '__main__':
    unittest.main()
