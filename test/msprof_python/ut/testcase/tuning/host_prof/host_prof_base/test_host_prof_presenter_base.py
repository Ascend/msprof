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

import pytest

from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase

NAMESPACE = 'host_prof.host_prof_base.host_prof_presenter_base'


class TestHostProfPresenterBase(unittest.TestCase):
    result_dir = 'test\\test'
    file_name = 'host_cpu.data.slice_0'

    def test_create_tables(self):
        with mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                        'HostNetworkUsage.init', return_value=False),\
                mock.patch(NAMESPACE + '.logging.error'),\
                pytest.raises(SystemExit) as err:
            check = HostProfPresenterBase('test', 'host_network.data.slice_0')
            check.cur_model = HostNetworkUsage('test')
            check._create_tables()
        self.assertEqual(err.value.code, 1)

    def test_run(self):
        with mock.patch(NAMESPACE + '.check_file_readable', return_value=True), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.init', return_value=True), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.create_table'), \
                mock.patch('host_prof.host_network_usage.model.host_network_usage.'
                           'HostNetworkUsage.flush_data'):
            check = HostProfPresenterBase('test', 'host_network.data.slice_0')
            check.cur_model = HostNetworkUsage('test')
            check.run()
        self.assertEqual(check.cur_model.__class__.__name__, 'HostNetworkUsage')


if __name__ == '__main__':
    unittest.main()
