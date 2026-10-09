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
from host_prof.host_cpu_usage.model.host_cpu_usage import HostCpuUsage
from host_prof.host_cpu_usage.presenter.host_cpu_usage_presenter import HostCpuUsagePresenter
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_cpu_usage.presenter.host_cpu_usage_presenter'


class TestHostCpuUsagePresenter(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_process_per_usage(self):
        check = HostCpuUsagePresenter(self.result_dir, self.file_name)
        result = check._process_per_usage((1, 2, 3), (1, 2, 3))
        self.assertEqual(result, None)

    def test_init(self):
        check = HostCpuUsagePresenter('test')
        check.init()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostCpuUsage')

    def test_parse_prof_data(self):
        data = 'time 191424757317191\n' \
               '191424.75 4526958.41\n' \
               '18070 (main) S 18159 18070 18159 34823 18070 1077944576 6629 0 0 0 4 2 0 0 ' \
               '20 0 9 0 19141641 8797048229888 27461 18446744073709551615 4194304 4247842 ' \
               '140728616566416 140728616563728 139927196887133 0 0 0 0 18446744072038935147 ' \
               '0 0 17 19 0 0 0 0 0 6348192 6349532 18935808 140728616568091 140728616568129 ' \
               '140728616568129 140728616570865 0\n' \
               'time 191424777703721\n' \
               '191424.78 4526958.78\n' \
               '18070 (main) S 18159 18070 18159 34823 18070 1077944576 6629 0 0 0 4 2 0 0 ' \
               '20 0 9 0 19141641 8797048229888 27461 18446744073709551615 4194304 4247842 ' \
               '140728616566416 140728616563728 139927196887133 0 0 0 0 18446744072038935147 ' \
               '0 0 17 19 0 0 0 0 0 6348192 6349532 18935808 140728616568091 140728616568129 ' \
               '140728616568129 140728616570865 0\n' \
               'time 191424798133446\n' \
               '191424.80 4526958.94\n' \
               '18070 (main) S 18159 18070 18159 34823 18070 1077944576 6629 0 0 0 4 2 0 0 ' \
               '20 0 9 0 19141641 8797048229888 27461 18446744073709551615 4194304 4247842 ' \
               '140728616566416 140728616563728 139927196887133 0 0 0 0 18446744072038935147 ' \
               '0 0 17 19 0 0 0 0 0 6348192 6349532 18935808 140728616568091 140728616568129 ' \
               '140728616568129 140728616570865 0'
        db_manager = DBManager()
        res = db_manager.create_table('host_cpu_usage.db')
        res[1].execute("create table if not exists CpuUsage(start_time text,end_time text,"
                       "cpu_no text,usage REAL)")
        res[1].execute("create table if not exists ProcessUsage(timestamp text,uptime text,"
                       "pid INTEGER,tid INTEGER,jiffies INTEGER,cpu_no text,usage REAL)")
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                        'HostCpuUsage.insert_cpu_info_data'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            InfoConfReader()._info_json = {"cpuNums": 8, "sysClockFreq": 100}
            check.cur_model = HostCpuUsage('test')
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.cpu_info, [8, 100])
        with mock.patch('builtins.open', side_effect=ValueError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.parse_prof_data()
        res[1].execute("drop table CpuUsage")
        res[1].execute("drop table ProcessUsage")
        db_manager.destroy(res)

    def test_get_cpu_usage_data(self):
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                        'HostCpuUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.has_cpu_usage_data', return_value=False):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostCpuUsage('test')
            result = check.get_cpu_usage_data()
            self.assertEqual(result, {})
        data_list = [[1, 2, 'name', 0.6, 200, 8, 120, 50, 20]]
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                        'HostCpuUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.has_cpu_usage_data', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.get_cpu_usage_data', return_value=data_list):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostCpuUsage('test')
            result = check.get_cpu_usage_data()
            self.assertEqual(result, data_list)

    def test_get_cpu_list(self):
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                        'HostCpuUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.has_cpu_usage_data', return_value=False):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostCpuUsage('test')
            result = check._get_cpu_list()
            self.assertEqual(result, {})
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                        'HostCpuUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.has_cpu_usage_data', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.'
                           'HostCpuUsage.get_cpu_list', return_value=123):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostCpuUsage('test')
            result = check._get_cpu_list()
            self.assertEqual(result, 123)

    def test_get_timeline_header(self):
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        with mock.patch(NAMESPACE + '.HostCpuUsagePresenter._get_cpu_list',
                        return_value=[('0',), ('Avg',)]):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_header()
            self.assertEqual(result, [['process_name', 1, 0, 'CPU Usage'],
                                      ['thread_name', 1, 0, 'CPU 0'],
                                      ['thread_sort_index', 1, 0, 0],
                                      ['thread_name', 1, 1, 'CPU Avg'],
                                      ['thread_sort_index', 1, 1, 1]])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.HostCpuUsagePresenter.get_cpu_usage_data',
                        return_value=(['CPU Avg', 191442613124.606, {'Usage(%)': 0.0}],)):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.sort_map = {'CPU Avg': 1}
            result = check.get_timeline_data()
            self.assertEqual(result, (['CPU Avg', 191442613124.606, {'Usage(%)': 0.0}, 1],))

    def test_get_summary_data(self):
        with mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.HostCpuUsage.check_db', return_value=True), \
             mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.HostCpuUsage.has_cpu_usage_data',
                        return_value=True), \
             mock.patch('host_prof.host_cpu_usage.model.host_cpu_usage.HostCpuUsage.get_num_of_used_cpus',
                        return_value=[10, 5]):
            check = HostCpuUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostCpuUsage('test')
            InfoConfReader()._info_json = {"cpuNums": 28, "sysClockFreq": 100}
            result = check.get_summary_data()
            self.assertEqual(result[0][0], 28)
            self.assertEqual(result[0][1], 10)
            self.assertEqual(result[0][2], 5)

if __name__ == '__main__':
    unittest.main()
