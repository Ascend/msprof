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
from host_prof.host_disk_usage.presenter.host_disk_usage_presenter import HostDiskUsagePresenter
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_disk_usage.presenter.host_disk_usage_presenter'


class TestHostDiskUsagePresenter(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_init(self):
        check = HostDiskUsagePresenter('test')
        check.init()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostDiskUsage')

    def test_parse_prof_data(self):
        data = '03:52:49 Total DISK READ :       0.00 B/s | Total DISK WRITE :       0.00 B/s\n' \
               '03:52:49 Actual DISK READ:       0.00 B/s | Actual DISK WRITE:       0.00 B/s\n' \
               'TIME  PID  PRIO  USER     DISK READ  DISK WRITE  SWAPIN      IO    COMMAND\n' \
               '03:52:49 18070 be/4 root        0.00 B/s    0.00 B/s  ?unavailable?  ' \
               './main 40 0x16 /home/ch/mnt/collect 0\n' \
               '03:52:49 Total DISK READ :       0.00 B/s | Total DISK WRITE :       0.00 B/s\n' \
               '03:52:49 Actual DISK READ:       0.00 B/s | Actual DISK WRITE:       0.00 B/s\n' \
               'len=1\n' \
               'a 12 b C d e f g h i j k l\n' \
               'start_time 191425757976595'
        db_manager = DBManager()
        res = db_manager.create_table('host_disk_usage.db')
        res[1].execute("create table if not exists DiskUsage(start_time text,end_time text,"
                       "disk_read text,disk_write text,swap_in text,usage REAL)")
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.ConfigMgr.get_disk_freq', return_value=50), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            InfoConfReader()._host_freq = None
            InfoConfReader()._info_json = {"cpuNums": 8, "sysClockFreq": 100, 'CPU': [{'Frequency': "1000"}]}
            check.cur_model = HostDiskUsage('test')
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.disk_usage_info, [[191425757976595.0, 191425777976595.0, 0.0, 0.0, '0.00', 0.0]])

        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.ConfigMgr.get_disk_freq', return_value=0), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.error'):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            InfoConfReader()._info_json = {"cpuNums": 8, "sysClockFreq": 100}
            check.cur_model = HostDiskUsage('test')
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        with mock.patch('builtins.open', side_effect=ValueError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            check.parse_prof_data()
        res[1].execute("drop table DiskUsage")
        db_manager.destroy(res)

    def test_get_disk_usage_data(self):
        with mock.patch('host_prof.host_disk_usage.model.host_disk_usage.'
                        'HostDiskUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_disk_usage.model.host_disk_usage.'
                           'HostDiskUsage.has_disk_usage_data', return_value=False):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostDiskUsage('test')
            result = check.get_disk_usage_data()
            self.assertEqual(result, {})
        with mock.patch('host_prof.host_disk_usage.model.host_disk_usage.'
                        'HostDiskUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_disk_usage.model.host_disk_usage.'
                           'HostDiskUsage.has_disk_usage_data', return_value=True), \
                mock.patch('host_prof.host_disk_usage.model.host_disk_usage.'
                           'HostDiskUsage.get_disk_usage_data', return_value=123):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostDiskUsage('test')
            result = check.get_disk_usage_data()
            self.assertEqual(result, 123)

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.HostDiskUsagePresenter.get_disk_usage_data',
                        return_value=None):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.HostDiskUsagePresenter.get_disk_usage_data',
                        return_value={'data': [{'start': 100, 'usage': 10000}]}):
            check = HostDiskUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [['Disk Usage', 100, {'Usage(%)': 10000}]])

    def test_get_timeline_header(self):
        check = HostDiskUsagePresenter(self.result_dir, self.file_name)
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        result = check.get_timeline_header()
        self.assertEqual(result, [['process_name', 1, 0, 'Disk Usage']])


if __name__ == '__main__':
    unittest.main()
