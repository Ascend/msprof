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
from collections import OrderedDict
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from viewer.msproftx_viewer import MsprofTxViewer
from profiling_bean.db_dto.msproftx_dto import MsprofTxDto, MsprofTxExDto

NAMESPACE = 'viewer.peripheral_report'


class TestMsprofTxViewer(unittest.TestCase):
    TRACE_HEADER_ARGS = 'args'
    TRACE_HEADER_PID = 'pid'
    TRACE_HEADER_TID = 'tid'
    TRACE_HEADER_PH = 'ph'
    TRACE_HEADER_NAME = 'name'
    TRACE_HEADER_ID = 'id'
    TEST_MSPROF_TX_MSG = 'test'
    MSPROFTX_DEVICE_HEADERS = ['index_id', 'start_time(us)', 'end_time(us)']
    configs = {'handler': '_get_msproftx_data',
               'headers': ['pid', 'tid', 'category', 'event_type', 'payload_type', 'payload_value', 'Start_time',
                           'End_time', 'message_type', 'message', 'domain', 'mark_id'],
               'db': 'msproftx.db',
               'unused_cols': []}
    params = {'data_type': 'msprof_tx', 'project': 'D:\\Job\\analyzer_testcase\\origin_data\\1910_mini_no_ai_core',
              'device_id': '0', 'job_id': 'job_default', 'export_type': 'summary', 'iter_id': 1, 'export_format': 'csv',
              'model_id': 1}


    def setUp(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._dev_cnt = 0.0
        InfoConfReader()._host_mon = 0.0
        InfoConfReader()._info_json = {
            'devices': '0', "pid": '1000', 'DeviceInfo': [{"hwts_frequency": "100"}], 'CPU': [{'Frequency': "1000"}]
        }

    def tearDown(self):
        InfoConfReader()._info_json = None

    def test_get_summary_data_return_valid_data_with_get_summary_from_model(self):
        valid_start_time = '10.001\t'
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_summary_data',
                        return_value=((0, 0, 0, 0, 0, 0, 0, 5, 0, 'test'), (0, 1, 0, 0, 1, 0, 1, 7, 0, 'test'))), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_summary_data',
                           return_value=((0, 0, 3, 1, 1, 'test', 'domain', 0), (0, 1, 3, 2, 11, 'test', 'domain', 1))):
            result = MsprofTxViewer(self.configs, self.params).get_summary_data()
            self.assertEqual(result,
                             (['pid', 'tid', 'category', 'event_type', 'payload_type', 'payload_value',
                               'Start_time', 'End_time', 'message_type', 'message', 'domain', 'mark_id'],
                              [(0, 0, 0, 0, 0, 0, '10.000\t', '10.005\t', 0, 'test', Constant.NA, Constant.NA),
                               (0, 1, 0, 0, 1, 0, valid_start_time, '10.007\t', 0, 'test', Constant.NA, Constant.NA),
                               (0, 0, Constant.NA, 3, Constant.NA, Constant.NA,
                                valid_start_time, valid_start_time, Constant.NA, 'test', 'domain', 0),
                               (0, 1, Constant.NA, 3, Constant.NA, Constant.NA,
                                '10.002\t', '10.011\t', Constant.NA, 'test', 'domain', 1)],
                              4))

    def test_get_summary_data_return_empty_data_with_get_none_summary_from_model(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_summary_data',
                        return_value=()), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_summary_data',
                           return_value=()):
            result = MsprofTxViewer(self.configs, self.params).get_summary_data()
            self.assertEqual(result, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_timeline_data_return_empty_data_with_get_none_data_from_model(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_timeline_data',
                       return_value=[]), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_timeline_data',
                           return_value=[]):
            result = MsprofTxViewer(self.configs, self.params).get_timeline_data()
            self.assertEqual(result, [])

    def test_get_timeline_data_return_valid_data_with_get_data_from_model(self):
        expect_timeline_data = [
            OrderedDict([(self.TRACE_HEADER_NAME, 'process_name'), (self.TRACE_HEADER_PID, 0),
                         (self.TRACE_HEADER_TID, 0),
                         (self.TRACE_HEADER_ARGS, OrderedDict([('name', 'MsprofTx')])), (self.TRACE_HEADER_PH, 'M')]),
            OrderedDict([(self.TRACE_HEADER_NAME, 'thread_name'), (self.TRACE_HEADER_PID, 0),
                         (self.TRACE_HEADER_TID, 0),
                         (self.TRACE_HEADER_ARGS, OrderedDict([('name', 'Thread 0')])), (self.TRACE_HEADER_PH, 'M')]),
            OrderedDict([(self.TRACE_HEADER_NAME, 'thread_sort_index'), (self.TRACE_HEADER_PID, 0),
                         (self.TRACE_HEADER_TID, 0),
                         (self.TRACE_HEADER_ARGS, OrderedDict([('sort_index', 0)])), (self.TRACE_HEADER_PH, 'M')]),
            OrderedDict([(self.TRACE_HEADER_NAME, self.TEST_MSPROF_TX_MSG), (self.TRACE_HEADER_PID, 0),
                         (self.TRACE_HEADER_TID, 0), ('ts', '10.010'), ('dur', 0.01),
                         (self.TRACE_HEADER_ARGS,
                          OrderedDict([('Category', '1'), ('Payload_type', 0), ('Payload_value', 0),
                                       ('Message_type', 0), ('event_type', 'marker')])), (self.TRACE_HEADER_PH, 'X')]),
            OrderedDict([(self.TRACE_HEADER_NAME, self.TEST_MSPROF_TX_MSG), (self.TRACE_HEADER_PID, 0),
                         (self.TRACE_HEADER_TID, 0), ('ts', '10.020'), ('dur', 0.0),
                         (self.TRACE_HEADER_ARGS,
                          OrderedDict([('mark_id', 0), ('event_type', 'marker_ex'), ('domain', 'domain')])),
                         (self.TRACE_HEADER_PH, 'X')])
        ]
        msproftx_dto = MsprofTxDto()
        msproftx_dto.pid = 0
        msproftx_dto.tid = 0
        msproftx_dto.category = 1
        msproftx_dto.event_type = 'marker'
        msproftx_dto.payload_type = 0
        msproftx_dto.start_time = 10
        msproftx_dto.end_time = 20
        msproftx_dto.message_type = 0
        msproftx_dto.message = self.TEST_MSPROF_TX_MSG
        msproftx_dto.dur_time = 10
        msproftx_ex_dto = MsprofTxExDto()
        msproftx_ex_dto.pid = 0
        msproftx_ex_dto.tid = 0
        msproftx_ex_dto.event_type = 'marker_ex'
        msproftx_ex_dto.start_time = 20
        msproftx_ex_dto.dur_time = 0
        msproftx_ex_dto.mark_id = 0
        msproftx_ex_dto.domain = "domain"
        msproftx_ex_dto.message = self.TEST_MSPROF_TX_MSG
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_timeline_data',
                        return_value=[msproftx_dto]), \
            mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_timeline_data',
                       return_value=[msproftx_ex_dto]):
            result = MsprofTxViewer(self.configs, self.params).get_timeline_data()
            self.assertEqual(result, expect_timeline_data)

    def test_get_device_summary_data_return_valid_data_with_get_device_summary_from_model(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_device_data',
                        return_value=[(0, 1, 0, 0, 0), (1, 2, 0, 0, 10)]):
            check = MsprofTxViewer(self.configs, self.params)
            res = check.get_device_summary_data()
            self.assertEqual(res, (self.MSPROFTX_DEVICE_HEADERS,
                                   [(0, '10.010\t', '10.010\t'), (1, '10.020\t', '10.120\t')], 2))

    def test_get_device_summary_data_return_empty_data_with_get_none_device_summary_from_model(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_device_data',
                        return_value=[]):
            check = MsprofTxViewer(self.configs, self.params)
            res = check.get_device_summary_data()
            self.assertEqual(res, (self.MSPROFTX_DEVICE_HEADERS, [], 0))

    def test_get_device_timeline_data_return_empty_data_with_get_none_device_data_from_model(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_device_data',
                        reture_value=[]):
            check = MsprofTxViewer(self.configs, self.params)
            res = check.get_device_timeline_data()
            self.assertEqual(res, [])

    def test_get_device_timeline_data_return_valid_data_with_get_device_data_from_model(self):
        expected_trace_data = [
            OrderedDict([
                (self.TRACE_HEADER_NAME, 'process_name'),
                (self.TRACE_HEADER_PID, 1000), (self.TRACE_HEADER_TID, 0),
                (self.TRACE_HEADER_ARGS, OrderedDict([
                    (self.TRACE_HEADER_NAME, 'Task Scheduler')])),
                (self.TRACE_HEADER_PH, 'M')
            ]),
            OrderedDict([
                (self.TRACE_HEADER_NAME, 'thread_name'),
                (self.TRACE_HEADER_PID, 1000), (self.TRACE_HEADER_TID, 0),
                (self.TRACE_HEADER_ARGS, OrderedDict([
                    (self.TRACE_HEADER_NAME, 'Stream 0')])),
                (self.TRACE_HEADER_PH, 'M')
            ]),
            OrderedDict([
                (self.TRACE_HEADER_NAME, 'thread_sort_index'),
                (self.TRACE_HEADER_PID, 1000), (self.TRACE_HEADER_TID, 0),
                (self.TRACE_HEADER_ARGS, OrderedDict([('sort_index', 0)])),
                (self.TRACE_HEADER_PH, 'M')
            ]),
            OrderedDict([
                (self.TRACE_HEADER_NAME, Constant.NA), (self.TRACE_HEADER_PID, 1000),
                (self.TRACE_HEADER_TID, 0), ('ts', '431145779385.630'), ('dur', 0.0),
                (self.TRACE_HEADER_ARGS, {'Physic Stream Id': 0, 'Task Id': 0}),
                (self.TRACE_HEADER_PH, 'X')]),
            {
                self.TRACE_HEADER_NAME: 'MsTx_0', self.TRACE_HEADER_PH: 'f', 'id': '0',
                'ts': '431145779385.630', 'cat': 'MsTx', self.TRACE_HEADER_PID: 1000,
                'tid': 0, 'bp': 'e'
            }
        ]
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.get_device_data') as MockMsprofTxExModel:
            MockMsprofTxExModel.return_value = [[0, 43114577937563, 0, 0, 0]]
            check = MsprofTxViewer(self.configs, self.params)
            res = check.get_device_timeline_data()
            self.assertEqual(res, expected_trace_data)

if __name__ == '__main__':
    unittest.main()
