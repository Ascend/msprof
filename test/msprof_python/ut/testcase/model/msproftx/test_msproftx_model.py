# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel
from profiling_bean.db_dto.step_trace_dto import MsproftxMarkDto

NAMESPACE = 'msmodel.msproftx.msproftx_model'


class TestMsprofTxModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MsprofTxModel.insert_data_to_db'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.get_timeline_data()

    def test_get_summary_data_should_return_empty_list_when_no_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            ret = check.get_summary_data()
            self.assertEqual([], ret)


class TestMsprofTxExModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxExModel('test', 'msproftx.db', ['MsprofTxEx'])
            check.flush([])

    def test_get_timeline_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
                check = MsprofTxExModel('test1', 'msproftx.db', ['MsprofTxEx'])
                res = check.get_timeline_data()
        self.assertEqual(res, [1])

    def test_get_timeline_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test2', 'msproftx.db', ['MsprofTxEx'])
            res = check.get_timeline_data()
        self.assertEqual(res, [])

    def test_get_summary_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
                check = MsprofTxExModel('test3', 'msproftx.db', ['MsprofTxEx'])
                res = check.get_summary_data()
        self.assertEqual(res, [1])

    def test_get_summary_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test4', 'msproftx.db', ['MsprofTxEx'])
            res = check.get_summary_data()
        self.assertEqual(res, [])

    def test_get_device_summary_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test5', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [])

    def test_get_device_summary_data_should_return_true_when_last_data_is_range_and_cann_version_before_910(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(index_id=0, timestamp=10, stream_id=0, task_id=0, tag_id=11),
                    MsproftxMarkDto(index_id=1, timestamp=11, stream_id=0, task_id=1, tag_id=11),
                    MsproftxMarkDto(index_id=1, timestamp=12, stream_id=0, task_id=2, tag_id=11)]):
            InfoConfReader()._info_json = {'cannVersion': '9.0.0'}
            check = MsprofTxExModel('test6', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[0, 10, 0, 0, 0], [1, 11, 0, 1, 1]])

    def test_get_device_summary_data_should_return_true_when_last_data_is_mark_and_cann_version_before_910(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(index_id=0, timestamp=10, stream_id=0, task_id=0, tag_id=11),
                    MsproftxMarkDto(index_id=0, timestamp=11, stream_id=0, task_id=1, tag_id=11),
                    MsproftxMarkDto(index_id=1, timestamp=12, stream_id=0, task_id=2, tag_id=11)]):
            InfoConfReader()._info_json = {'cannVersion': '9.0.0'}
            check = MsprofTxExModel('test7', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[0, 10, 0, 0, 1], [1, 12, 0, 2, 0]])

    def test_get_device_summary_data_should_return_true_when_last_data_is_range_and_cann_version_after_910(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                     MsproftxMarkDto(index_id=0, timestamp=10, stream_id=0, task_id=0, tag_id=11),
                     MsproftxMarkDto(index_id=1, timestamp=11, stream_id=0, task_id=1, tag_id=12),
                     MsproftxMarkDto(index_id=1, timestamp=12, stream_id=0, task_id=2, tag_id=12)]):
            InfoConfReader()._info_json = {'cannVersion': '9.1.0'}
            check = MsprofTxExModel('test8', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[0, 10, 0, 0, 0], [1, 11, 0, 1, 1]])

    def test_get_device_summary_data_should_return_true_when_last_data_is_mark_and_cann_version_after_910(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(index_id=0, timestamp=10, stream_id=0, task_id=0, tag_id=12),
                    MsproftxMarkDto(index_id=0, timestamp=11, stream_id=0, task_id=1, tag_id=12),
                    MsproftxMarkDto(index_id=1, timestamp=12, stream_id=0, task_id=2, tag_id=11)]):
            InfoConfReader()._info_json = {'cannVersion': '9.1.0'}
            check = MsprofTxExModel('test9', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[1, 12, 0, 2, 0], [0, 10, 0, 0, 1]])

    def test_get_device_summary_data_should_return_true_when_multiple_data_in_same_id_and_cann_version_after_910(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
            mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(index_id=0, timestamp=10, stream_id=0, task_id=0, tag_id=12),
                    MsproftxMarkDto(index_id=0, timestamp=11, stream_id=0, task_id=1, tag_id=12),
                    MsproftxMarkDto(index_id=0, timestamp=12, stream_id=0, task_id=2, tag_id=12),
                    MsproftxMarkDto(index_id=0, timestamp=13, stream_id=0, task_id=3, tag_id=12),
                    MsproftxMarkDto(index_id=1, timestamp=14, stream_id=0, task_id=2, tag_id=11)]):
            InfoConfReader()._info_json = {'cannVersion': '9.1.0'}
            check = MsprofTxExModel('test10', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[1, 14, 0, 2, 0], [0, 10, 0, 0, 1], [0, 12, 0, 2, 1]])