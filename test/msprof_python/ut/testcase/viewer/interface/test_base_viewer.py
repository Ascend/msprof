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

from msmodel.stars.ffts_log_model import FftsLogModel
from viewer.interface.base_viewer import BaseViewer

NAMESPACE = 'viewer.interface.base_viewer'


class TestBaseViewer(unittest.TestCase):
    def test_get_summary_data(self):
        configs = {'headers': [1]}
        params = {'data_type': 'ffts_thread_log'}
        with mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.check_db', return_value=1), \
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.get_summary_data',
                           return_value=[1]), \
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.finalize'):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_summary_data()
            self.assertEqual(ret, ([1], [1], 1))
        with mock.patch(NAMESPACE + '.BaseViewer.get_model_instance', return_value=None):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_summary_data()
            self.assertEqual(ret, ([1], [], 0))

    def test_get_model_instance(self):
        configs = {}
        params = {'data_type': 'ffts_thread_log'}
        check = BaseViewer(configs, params)
        check.model_list = {'ffts_thread_log': FftsLogModel}
        check.get_model_instance()

    def test_get_trace_timeline(self):
        pass


if __name__ == '__main__':
    unittest.main()
