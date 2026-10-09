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
from host_prof.host_mem_usage.model.host_mem_usage import HostMemUsage
from host_prof.host_mem_usage.presenter.host_mem_usage_presenter import HostMemUsagePresenter
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_mem_usage.presenter.host_mem_usage_presenter'


class TestHostMemUsagePresenter(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_init(self):
        check = HostMemUsagePresenter('test')
        check.init()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostMemUsage')

    def test_parse_mem_data(self):
        data = [
            '191424758423535 2147716853', '191424778902215 2147716853 27461 4653 14 0 213261 0',
            '191424799235089 2147716853 27461 4653 14 0 213261 0\n',
            '191424819600139 2147716853 27461 4653 14 0 213261 0\n', ''
        ]
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch("common_func.file_manager.check_path_valid"):
            check = HostMemUsagePresenter('test')
            check.cur_model = HostMemUsage(self.result_dir)
            result = check._parse_mem_data(data, 197510316)
            result = check._parse_mem_data(data, "a")
        self.assertEqual(result, None)

    def test_parse_pro_data(self):
        data = '191424758423535 2147716853 27461 4653 14 0 213261 0\n' \
               '191424778902215 2147716853 27461 4653 14 0 213261 0\n' \
               '191424799235089 2147716853 27461 4653 14 0 213261 0\n' \
               '191424819600139 2147716853 27461 4653 14 0 213261 0\n' \
               ''
        db_manager = DBManager()
        res = db_manager.create_table('host_mem_usage.db')
        res[1].execute("create table if not exists MemUsage(start_time text,end_time text,"
                       "usage REAL)")
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            InfoConfReader()._info_json = {"memoryTotal": 197510316}
            check.cur_model = HostMemUsage('test')
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.mem_usage_info, [['191424758423535', '191424778902215', '0.055614'],
                                                ['191424778902215', '191424799235089', '0.055614'],
                                                ['191424799235089', '191424819600139', '0.055614']])
        with mock.patch('builtins.open', side_effect=ValueError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            check.parse_prof_data()
        res[1].execute("drop table MemUsage")
        db_manager.destroy(res)

    def test_get_mem_usage_data(self):
        with mock.patch('host_prof.host_mem_usage.model.host_mem_usage.'
                        'HostMemUsage.check_db', return_value=True), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('host_prof.host_mem_usage.model.host_mem_usage.'
                           'HostMemUsage.has_mem_usage_data', return_value=False):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostMemUsage('test')
            result = check.get_mem_usage_data()
            self.assertEqual(result, {})
        with mock.patch('host_prof.host_mem_usage.model.host_mem_usage.'
                        'HostMemUsage.check_db', return_value=True), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('host_prof.host_mem_usage.model.host_mem_usage.'
                           'HostMemUsage.has_mem_usage_data', return_value=True), \
                mock.patch('host_prof.host_mem_usage.model.host_mem_usage.'
                           'HostMemUsage.get_mem_usage_data', return_value=123):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostMemUsage('test')
            result = check.get_mem_usage_data()
            self.assertEqual(result, 123)

    def test_get_timeline_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch(NAMESPACE + '.HostMemUsagePresenter.get_mem_usage_data',
                        return_value=None), \
                mock.patch("common_func.file_manager.check_path_valid"):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.HostMemUsagePresenter.get_mem_usage_data',
                        return_value={'data': [{'start': 100, 'usage': 10000}]}), \
                mock.patch("common_func.file_manager.check_path_valid"):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [['Memory Usage', 100.0, {'Usage(%)': 10000}]])

    def test_get_timeline_header(self):
        check = HostMemUsagePresenter(self.result_dir, self.file_name)
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        result = check.get_timeline_header()
        self.assertEqual(result, [['process_name', 1, 0, 'Memory Usage']])

    def test_get_summary_data(self):
        with mock.patch('host_prof.host_mem_usage.model.host_mem_usage.HostMemUsage.check_db', return_value=True), \
                mock.patch('host_prof.host_mem_usage.model.host_mem_usage.HostMemUsage.has_mem_usage_data',
                           return_value=True), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('host_prof.host_mem_usage.model.host_mem_usage.HostMemUsage.get_recommend_value',
                           return_value=[12, 15]):
            check = HostMemUsagePresenter(self.result_dir, self.file_name)
            check.cur_model = HostMemUsage('test')
            InfoConfReader()._info_json = {"memoryTotal": 197510316}
            result = check.get_summary_data()
            self.assertEqual(result[0][0], 197510316)
            self.assertEqual(result[0][1], 12 * 197510316 / 100)
            self.assertEqual(result[0][2], 15 * 197510316 / 100)


if __name__ == '__main__':
    unittest.main()
