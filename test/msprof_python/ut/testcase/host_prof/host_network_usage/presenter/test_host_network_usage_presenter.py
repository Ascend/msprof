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

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage
from host_prof.host_network_usage.presenter.host_network_usage_presenter import HostNetworkUsagePresenter, \
    parse_net_stats
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_network_usage.presenter.host_network_usage_presenter'


def test_parse_net_stats():
    with mock.patch(NAMESPACE + '.logging.error'):
        parse_net_stats('ea b c 0 0 0 0 0 0 0 0 0 0 0 0 0')


class TestHostNetworkUsage(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_init(self):
        check = HostNetworkUsagePresenter('test')
        check.init()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostNetworkUsage')

    def test_parse_prof_data(self):
        data = 'time 191424758430371\n' \
               'enp2s0f0: 23258858569 17604359 0 114831 0 0 0 130266 2155315993 8755574 0 0 0 0 0 0\n' \
               'time 191424778907458\n' \
               'enp2s0f0: 23258858569 17604359 0 114831 0 0 0 130266 2155315993 8755574 0 0 0 0 0 0\n' \
               'time 191424799238846\n' \
               'enp2s0f0: 23258858569 17604359 0 114831 0 0 0 130266 2155315993 8755574 0 0 0 0 0 0\n' \
               ''
        netcard = [{'netCardName': 'enp2s0f0', 'speed': 1000}]
        db_manager = DBManager()
        res = db_manager.create_table('host_network_usage.db')
        res[1].execute("create table if not exists NetworkUsage(start_time text,end_time text,usage REAL)")
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            InfoConfReader()._info_json = {"netCardNums": 8, "netCard": netcard}
            check.cur_model = HostNetworkUsage('test')
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.speeds, {'enp2s0f0': 125000000})
        with mock.patch('builtins.open', side_effect=ValueError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            check.parse_prof_data()
        res[1].execute("drop table NetworkUsage")
        db_manager.destroy(res)

    def test_get_network_usage_data(self):
        with mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                        'HostNetworkUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.has_network_usage_data', return_value=False):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostNetworkUsage('test')
            result = check.get_network_usage_data()
            self.assertEqual(result, {})
        with mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                        'HostNetworkUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.has_network_usage_data', return_value=True), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.get_network_usage_data', return_value=123):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostNetworkUsage('test')
            result = check.get_network_usage_data()
            self.assertEqual(result, 123)

    def test_get_timeline_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch(NAMESPACE + '.HostNetworkUsagePresenter.get_network_usage_data',
                        return_value={'data': [{'start': 100, 'usage': 10000}]}):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [['Network Usage', 100.0, {'Usage(%)': 10000}]])

    def test_get_timeline_header(self):
        check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        result = check.get_timeline_header()
        self.assertEqual(result, [['process_name', 1, 0, 'Network Usage']])

    def test_compute_netcard1_speed(self):
        netcard = [{'netCardName': 'enp2s0f0', 'speed': 1024}]
        InfoConfReader()._info_json = {"netCardNums": 8, "netCard": netcard}
        check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
        ret = check.compute_netcard_speed()
        self.assertEqual(ret, Constant.MBPS_TO_BYTES)

    def test_compute_netcard2_speed(self):
        netcard = [
            {'netCardName': 'enp2s0f0', 'speed': 1024},
            {'netCardName': 'enp2s0f1', 'speed': 100},
            {'netCardName': 'enp2s0f1', 'speed': 10}
        ]
        InfoConfReader()._info_json = {"netCardNums": 8, "netCard": netcard}
        check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
        ret = check.compute_netcard_speed()
        self.assertEqual(ret, 1134 / 1024 * Constant.MBPS_TO_BYTES)

    def test_get_summary_data(self):
        with mock.patch('host_prof.host_network_usage.model.host_network_usage.HostNetworkUsage.check_db',
                        return_value=True), \
             mock.patch('host_prof.host_network_usage.model.host_network_usage.HostNetworkUsage.has_network_usage_data',
                        return_value=True), \
             mock.patch('host_prof.host_network_usage.model.host_network_usage.HostNetworkUsage.get_recommend_value',
                        return_value=[1233, 1444]), \
             mock.patch(NAMESPACE + '.HostNetworkUsagePresenter.compute_netcard_speed',
                        return_value=25000):
            check = HostNetworkUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostNetworkUsage('test')
            result = check.get_summary_data()
            self.assertEqual(result[0][0], 25000)
            self.assertEqual(result[0][1], 1233)
            self.assertEqual(result[0][2], 1444)

if __name__ == '__main__':
    unittest.main()
