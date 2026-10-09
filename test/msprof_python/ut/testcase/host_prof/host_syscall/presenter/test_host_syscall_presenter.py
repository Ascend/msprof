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
from host_prof.host_syscall.model.host_syscall import HostSyscall
from host_prof.host_syscall.presenter.host_syscall_presenter import HostSyscallPresenter
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_syscall.presenter.host_syscall_presenter'


class TestHostSyscallPresenter(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_init(self):
        InfoConfReader()._info_json = {'pid': 1}
        check = HostSyscallPresenter('test')
        check.init()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostSyscall')

    def test_parse_prof_data(self):
        pthreadgap_data = '\n' \
                          '[pid 18145] 1615405971.249133 +++ exited (status 0) >\n' \
                          '[pid 18145] a +++ exited (status 0) 12.1>\n' \
                          '[pid 18145] 1615405971.249133 +++ pthread_once (status 0) 12.2>\n' \
                          '[pid 19618] 1615843248.065895 libascendcl.so->pthread_mutex_lock' \
                          '(0x23e49f0, 0x23e49f0, 0, 0) = 0 <0.000430>'
        syscall_data = '\n' \
                       '         ? (         ): MSVP_UploaderD/18072  ... [continued]: nanosleep()) = 0\n' \
                       '191419454.930 ( 0.002 ms): MSVP_UploaderD/18072 clock_gettime(which_clock: ' \
                       'MONOTONIC_RAW, tp: 0x7f43583b3a50          ) = 0\n' \
                       '191419454.934 ( 1.057 ms): MSVP_UploaderD/18072 nanosleep(rqtp: 0x7f43583b3a40' \
                       '                                        ) = 0\n' \
                       '191419455.995 ( 0.002 ms): MSVP_UploaderD/18072 clock_gettime(which_clock:' \
                       ' MONOTONIC_RAW, tp: 0x7f43583b3a50          ) = 0\n' \
                       '\n'
        db_manager = DBManager()
        res = db_manager.create_table('host_runtime_api.db')
        res[1].execute("create table if not exists Syscall(runtime_comm text,runtime_pid INTEGER,"
                       "runtime_tid INTEGER,runtime_api_name text,runtime_start_time REAL,"
                       "runtime_duration REAL,runtime_end_time REAL,runtime_trans_start REAL,"
                       "runtime_trans_end REAL)")
        with mock.patch('builtins.open', mock.mock_open(read_data=pthreadgap_data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            InfoConfReader()._info_json = {"cpuNums": 8, "sysClockFreq": 100, 'pid': 1}
            InfoConfReader()._start_info = {"clockMonotonicRaw": 191424756318872}
            InfoConfReader()._end_info = {"clockMonotonicRaw": 191455803872302}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            check.file_name = 'host_pthreadcall.data.slice_0'
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.pid, 1)
        with mock.patch('builtins.open', mock.mock_open(read_data=syscall_data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'):
            InfoConfReader()._info_json = {'pid': 1}
            InfoConfReader()._start_info = {"clockMonotonicRaw": 191424756318872}
            InfoConfReader()._end_info = {"clockMonotonicRaw": 191455803872302}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            check.file_name = 'host_syscall.data.slice_0'
            check.cur_model.conn = res[0]
            check.parse_prof_data()
        self.assertEqual(check.pid, 1)
        with mock.patch('builtins.open', side_effect=ValueError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.parse_prof_data()
        res[1].execute("drop table Syscall")
        db_manager.destroy(res)

    def test_get_command_api_info(self):
        raw_data_list = ['123', ' MSVP_Uploader clock_gettime(which_clock', ]
        pid = 1
        InfoConfReader()._info_json = {'pid': 1}
        check = HostSyscallPresenter(self.result_dir, self.file_name)
        check.cur_model = HostSyscall('test')
        result = check.get_command_api_info(raw_data_list, pid)
        self.assertEqual(result, ('*', 1, 'MSVP_Uploader'))

    def test_get_summary_api_info(self):
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=False):
            InfoConfReader()._info_json = {'pid': 1}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_summary_api_info('test')
            self.assertEqual(result, [])
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=False):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_summary_api_info('test')
            self.assertEqual(result, [])
        data_list = [(1, 2, 'name', 0.6, 200, 8, 120, 50, 20)]
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.get_summary_runtime_api_data', return_value=data_list):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_summary_api_info('test')
            self.assertEqual(result, [(1, 2, 'name', 0.6, 0.2, 8, 0.12, 0.05, 0.02)])

    def test_get_runtime_api_data(self):
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=False):
            InfoConfReader()._info_json = {'pid': 1}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_runtime_api_data()
            self.assertEqual(result, [])
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=False):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_runtime_api_data()
            self.assertEqual(result, [])
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.get_runtime_api_data', return_value=123):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check.get_runtime_api_data()
            self.assertEqual(result, 123)

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.HostSyscallPresenter.get_runtime_api_data',
                        return_value=[]):
            InfoConfReader()._host_freq = None
            InfoConfReader()._info_json = {'pid': 1, 'tid': 0, 'CPU': [{'Frequency': "1000"}]}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.HostSyscallPresenter.get_runtime_api_data',
                        return_value=[('MSVP_UploaderD', 18070, 18072, 'nanosleep', 191449517.503,
                                       1057.0, 191449518.56, 191454818891.872, 191454819948.872)]):
            InfoConfReader()._host_freq = None
            InfoConfReader()._info_json = {'pid': 1, 'tid': 0, 'CPU': [{'Frequency': "1000"}]}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            result = check.get_timeline_data()
        self.assertEqual(result, [['nanosleep', 18070, 18072, '1890264311.302', 1.057]])

    def test_get_timeline_header(self):
        with mock.patch(NAMESPACE + '.HostSyscallPresenter._get_tid_list',
                        return_value=[(18072,), (18070,)]):
            InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            result = check.get_timeline_header()
        self.assertEqual(result, [['process_name', 1, 0, 'OS Runtime API'],
                                  ['thread_name', 1, 18072, 'Thread 18072'],
                                  ['thread_sort_index', 1, 18072, 18072],
                                  ['thread_name', 1, 18070, 'Thread 18070'],
                                  ['thread_sort_index', 1, 18070, 18070]])

    def test_get_tid_list(self):
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=False):
            InfoConfReader()._info_json = {'pid': 1}
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check._get_tid_list()
            self.assertEqual(result, [])
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=False):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check._get_tid_list()
            self.assertEqual(result, [])
        with mock.patch('host_prof.host_syscall.model.host_syscall.'
                        'HostSyscall.check_db', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.has_runtime_api_data', return_value=True), \
                mock.patch('host_prof.host_syscall.model.host_syscall.'
                           'HostSyscall.get_all_tid', return_value=123):
            check = HostSyscallPresenter(self.result_dir, self.file_name)
            check.cur_model = HostSyscall('test')
            result = check._get_tid_list()
            self.assertEqual(result, 123)

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}
        info_reader._host_freq = None


if __name__ == '__main__':
    unittest.main()
