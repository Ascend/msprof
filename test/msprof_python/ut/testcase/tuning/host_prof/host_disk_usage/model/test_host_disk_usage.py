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
from host_prof.host_disk_usage.model.host_disk_usage import HostDiskUsage

NAMESPACE = 'host_prof.host_disk_usage.model.host_disk_usage'


class TsetHostDiskUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_insert_disk_usage_data(self):
        disk_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostDiskUsage(self.result_dir)
            check.insert_disk_usage_data(disk_usage_info)

    def test_has_disk_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostDiskUsage(self.result_dir)
            check.has_disk_usage_data()

    def test_get_disk_usage_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        disk_info_list = ((0, 1, 2, 3, 4, 5, 6),)
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                        return_value=disk_info_list):
            check = HostDiskUsage(self.result_dir)
            result = check.get_disk_usage_data()
        self.assertEqual(result, {'data': [{'end': '10.001', 'start': '10.000', 'usage': 5}]})


if __name__ == '__main__':
    unittest.main()
