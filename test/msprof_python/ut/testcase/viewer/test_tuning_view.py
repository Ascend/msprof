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

from tuning.data_manager import OpSummaryTuningDataHandle
from viewer.tuning_view import TuningView

NAMESPACE = 'viewer.tuning_view'


class TestTuningView(unittest.TestCase):
    def test_show_by_dev_id(self):
        with mock.patch(NAMESPACE + '.TuningView.tuning_report'):
            TuningView("", {}, 0).show_by_dev_id()

    def test_load_result_file_1(self):
        with mock.patch('builtins.open', side_effect=FileNotFoundError), \
                mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value='test'), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=200), \
                mock.patch('os.access', return_value=True), \
                mock.patch('os.path.exists', return_value=True):
            res = TuningView("", {}, 0).get_tuning_data()
        self.assertEqual(res, None)

    def test_tuning_report(self):
        data = {"data": {"Op Summary": [{"result": [{'rule_subtype': 'test'}]}]}, "Rule Type": 1}
        with mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value='test'),\
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=200), \
                mock.patch('os.access', return_value=True), \
                mock.patch('builtins.open', mock.mock_open(read_data=json.dumps(data))):

            TuningView("", {}, 0).tuning_report()

        with mock.patch(NAMESPACE + '.TuningView._load_result_file', return_value=data), \
                mock.patch(NAMESPACE + '.TuningView.print_first_level'), \
                mock.patch(NAMESPACE + '.TuningView.print_second_level'):
            TuningView("", {}, 0).tuning_report()

    def test_print_first_level(self):
        data = {"Rule Type": 1}
        TuningView("", {}, 0).print_first_level(0, data)

    def test_print_second_level(self):
        data = [{"Rule Type": "1", "Op List": "3", "Rule Suggestion": "4"}]
        data_sub = [{"Rule Type": "1", "Rule Subtype": "2", "Op List": "3", "Rule Suggestion": "4"},
                    {"Rule Type": "1", "Rule Subtype": "2", "Op List": "3", "Rule Suggestion": "4"}]
        TuningView("", {}, 0).print_second_level(None, OpSummaryTuningDataHandle)
        TuningView("", {}, 0).print_second_level(data, OpSummaryTuningDataHandle)
        TuningView("", {}, 0).print_second_level(data_sub, OpSummaryTuningDataHandle)


if __name__ == '__main__':
    unittest.main()
