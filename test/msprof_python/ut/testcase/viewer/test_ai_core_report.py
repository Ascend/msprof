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

from common_func.msvp_constant import MsvpConstant
from viewer.ai_core_report import AiCoreReport

NAMESPACE = 'viewer.ai_core_report'
params = {'core_data_type': 'ai_core_pmu_events',
          'project': '', 'device_id': '0',
          'job_id': 'job_default',
          'export_type': 'summary', 'iter_id': 1,
          'export_format': 'csv', 'model_id': 1}


class TestAclReport(unittest.TestCase):
    def test_get_core_sample_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = AiCoreReport.get_core_sample_data('', '', params)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_core_sample_data_2(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.AiCoreReport.get_output_event_counter', return_value=([], [])):
            res = AiCoreReport.get_core_sample_data('', '', params)
        self.assertEqual(res[1], [])

    def test_get_core_sample_data_3(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.AiCoreReport.get_output_event_counter', return_value=([], [])):
            params["data_type"] = "ai_vector_core_pmu_events"
            res = AiCoreReport.get_core_sample_data('', '', params)
        self.assertEqual(res[1], [])

    def test_get_core_sample_data_4(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.AiCoreReport.get_output_event_counter', return_value=([], [])):
            params["data_type"] = ""
            res = AiCoreReport.get_core_sample_data('', '', params)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_output_event_counter_1(self):
        res = AiCoreReport.get_output_event_counter(None)
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_2(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = AiCoreReport.get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_3(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            res = AiCoreReport.get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_4(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]), \
             mock.patch(NAMESPACE + '.AiCoreReport._get_output_event_counter', return_value=([], [])):
            res = AiCoreReport.get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test__get_output_event_counter_1(self):
        res = AiCoreReport._get_output_event_counter([["vec_bankgroup_cflt_ratio", 0], ["vec_bankgroup_cflt_ratio", 1]], [(0,), (1,)])
        self.assertEqual(res, (["Core ID", "vec_bankgroup_cflt_ratio"], [["Core0", 0.0], ["Core1", 1.0], ["Average", 0.5]]))


if __name__ == '__main__':
    unittest.main()
