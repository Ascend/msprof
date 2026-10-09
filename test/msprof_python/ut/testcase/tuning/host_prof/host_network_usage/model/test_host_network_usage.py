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
from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage

NAMESPACE = 'host_prof.host_network_usage.model.host_network_usage'


class TsetHostNetworkUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_flush_data(self):
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostNetworkUsage(self.result_dir)
            check.cache_data = {}
            check.flush_data()

    def test_has_network_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostNetworkUsage(self.result_dir)
            check.has_network_usage_data()

    def test_get_network_usage_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        disk_info_list = ((0, 1, 2, 3, 4, 5, 6),)
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                        return_value=disk_info_list):
            check = HostNetworkUsage(self.result_dir)
            result = check.get_network_usage_data()
        self.assertEqual(result, {'data': [{'end': '10.001', 'start': '10.000', 'usage': 2}]})


if __name__ == '__main__':
    unittest.main()
