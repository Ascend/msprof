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

from host_prof.host_network_usage.network_usage_analysis import NetworkUsageAnalysis

NAMESPACE = 'host_prof.host_network_usage.network_usage_analysis'


class TestNetworkUsageAnalysis(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch('os.path.exists', return_value=False),\
                mock.patch(NAMESPACE + '.logging.error'):
            check = NetworkUsageAnalysis({'result': 'test'})
            check.ms_run()
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value=True), \
                mock.patch('os.listdir', return_value=['host_network.data.slice_0']), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.HostNetworkUsagePresenter.run'),\
                mock.patch('host_prof.host_prof_base.host_prof_presenter_base.'
                           'PathManager.get_data_file_path', return_value='test'):
            check = NetworkUsageAnalysis({'result': 'test'})
            check.ms_run()
