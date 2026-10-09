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
from host_prof.host_syscall.host_syscall_analysis import HostSyscallAnalysis

NAMESPACE = 'host_prof.host_syscall.host_syscall_analysis'


class TestHostSyscallAnalysis(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch('os.path.exists', return_value=False),\
                mock.patch(NAMESPACE + '.logging.error'):
            check = HostSyscallAnalysis({'result': 'test'})
            check.ms_run()
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value=True), \
                mock.patch(NAMESPACE + '.get_data_dir_sorted_files', return_value=['host_syscall.data.slice_0']), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info', return_value=True), \
                mock.patch(NAMESPACE + '.HostSyscallPresenter.run'), \
                mock.patch('os.path.isfile', return_value=False), \
                mock.patch('host_prof.host_prof_base.host_prof_presenter_base.'
                           'PathManager.get_data_file_path', return_value='test'):
            InfoConfReader()._info_json = {'pid': 2}
            check = HostSyscallAnalysis({'result': 'test'})
            check.ms_run()
