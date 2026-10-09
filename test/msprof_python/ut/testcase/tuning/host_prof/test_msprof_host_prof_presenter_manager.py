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
import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from host_prof.host_prof_presenter_manager import get_host_prof_timeline, get_time_data, \
    get_column_data, HostExportType

NAMESPACE = 'host_prof.host_prof_presenter_manager'
CPUPRESENTSPACE = 'host_prof.host_cpu_usage.presenter.host_cpu_usage_presenter.' \
                  'HostCpuUsagePresenter'
SYSCALLSPACE = 'host_prof.host_syscall.presenter.host_syscall_presenter.HostSyscallPresenter'


def test_get_host_prof_timeline():
    result_dir = 'test\\host_data'
    export_type = HostExportType.CPU_USAGE
    with mock.patch(CPUPRESENTSPACE + '.init'), \
            mock.patch(CPUPRESENTSPACE + '.get_timeline_header',
                       return_value=[['process_name', 18070, 0, 'CPU Usage']]):
        with mock.patch(CPUPRESENTSPACE + '.get_timeline_data',
                        return_value=None):
            result = get_host_prof_timeline(result_dir, export_type)
        unittest.TestCase().assertEqual(result, [])
        with mock.patch(CPUPRESENTSPACE + '.get_timeline_data',
                        return_value=123), \
                mock.patch(NAMESPACE + '.get_column_data', return_value=123):
            InfoConfReader()._info_json = {'pid': 1}
            result = get_host_prof_timeline(result_dir, export_type)
        unittest.TestCase().assertEqual(result, 123)
    with mock.patch(SYSCALLSPACE + '.init'), \
            mock.patch(SYSCALLSPACE + '.get_timeline_header',
                       return_value=[['process_name', 18070, 0, 'CPU Usage']]):
        with mock.patch(SYSCALLSPACE + '.get_timeline_data',
                        return_value=1), \
                mock.patch(NAMESPACE + '.get_time_data', return_value=123):
            InfoConfReader()._info_json = {'pid': 1}
            result = get_host_prof_timeline(result_dir, HostExportType.HOST_RUNTIME_API)
        unittest.TestCase().assertEqual(result, 123)


def test_get_time_data():
    data_list = [['nanosleep', 18070, 18072, 191454817780.872, 1056.0],
                 ['nanosleep', 18070, 18072, 191454818891.872, 1057.0]]
    header = [['process_name', 18070, 0, 'OS Runtime API'],
              ['thread_name', 18070, 18072, 'Thread 18072'],
              ['thread_name', 18070, 18070, 'Thread 18070']]
    expected_result = [{"name": "process_name", "pid": 18070, "tid": 0,
                        "args": {"name": "OS Runtime API"}, "ph": "M"},
                       {"name": "thread_name", "pid": 18070, "tid": 18072,
                        "args": {"name": "Thread 18072"}, "ph": "M"},
                       {"name": "thread_name", "pid": 18070, "tid": 18070,
                        "args": {"name": "Thread 18070"}, "ph": "M"},
                       {"name": "nanosleep", "pid": 18070, "tid": 18072,
                        "ts": 191454817780.872, "dur": 1056.0, "ph": "X"},
                       {"name": "nanosleep", "pid": 18070, "tid": 18072,
                        "ts": 191454818891.872, "dur": 1057.0, "ph": "X"}]
    check = get_time_data(data_list, header)
    unittest.TestCase().assertEqual(check, expected_result)


def test_get_column_data():
    data_list = [['CPU 22', 191453386719.492, {'Usage(%)': 0.0}, 13],
                 ['CPU 22', 191453407257.401, {'Usage( %)': 0.0}, 13]]
    header = [['process_name', 18070, 0, 'CPU Usage'], ['thread_name', 18070, 0, 'CPU 0'],
              ['thread_sort_index', 18070, 0, 0]]
    InfoConfReader()._info_json = {'pid': 0}
    check = get_column_data(data_list, header)
    unittest.TestCase().assertEqual(len(check), 5)
