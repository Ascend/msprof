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
from profiling_bean.basic_info.host_info import HostInfo

NAMESPACE = 'profiling_bean.basic_info.host_info'

info_json = {"jobInfo": "NA", "mac": "A4-DC-BE-F9-9A-C4",
             "OS": "Linux-4.15.0-45-generic-#48-Ubuntu SMP Tue Jan 29 16:28:13 UTC 2019", "cpuCores": 2,
             "hostname": "ubuntu--81", "hwtype": "x86_64", "devices": "0", "platform": "",
             "CPU": [{"Id": 0, "Name": "GenuineIntel", "Frequency": 0, "Logical_CPU_Count": 12,
                      "Type": "Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz"},
                     {"Id": 1, "Name": "GenuineIntel", "Frequency": 0, "Logical_CPU_Count": 12,
                      "Type": "Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz"}],
             "DeviceInfo": [
                 {"id": 0, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55", "ctrl_cpu_core_num": 4,
                  "ctrl_cpu_endian_little": 1,
                  "ts_cpu_core_num": 1, "ai_cpu_core_num": 4, "ai_core_num": 2, "ai_cpu_core_id": 4, "ai_core_id": 0,
                  "aicpu_occupy_bitmap": 240, "ctrl_cpu": "0,1,2,3", "ai_cpu": "4,5,6,7", "aiv_num": 0,
                  "hwts_frequency": "19.2",
                  "aic_frequency": "680", "aiv_frequency": "1000"}],
             "platform_version": "0", "pid": "3251", "memoryTotal": 197688432, "cpuNums": 24,
             "sysClockFreq": 100, "upTime": "7702.10 184716.76", "netCardNums": 0,
             "netCard": [{"netCardName": "enp2s0f0", "speed": 1000}]}


class TestHostInfo(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.HostInfo.merge_data'):
            HostInfo().run(0)

    def test_merge_data_1(self):
        InfoConfReader()._info_json = info_json
        HostInfo().merge_data()


if __name__ == '__main__':
    unittest.main()
