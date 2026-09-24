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
import json
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from constant.constant import ITER_RANGE
from sqlite.db_manager import DBManager, DBOpen
from viewer.training.step_trace_viewer import StepTraceViewer
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'viewer.training.step_trace_viewer'
message = {"project_path": '',
           "device_id": "4",
           "sql_path": ''}

DB_TRACE = DBNameConstant.DB_TRACE
TABLE_TRAINING_TRACE = DBNameConstant.TABLE_TRAINING_TRACE
TABLE_ALL_REDUCE = DBNameConstant.TABLE_ALL_REDUCE

DATA1 = (('127.0.0.1', 4, 1, 65, 144, 1013979616375, 65, 145, 1013981208724,
          71, 189, 1013981571636, 64, 5, 2955261, 1592349, 362912, 0, 0),)

DATA2 = (('127.0.0.1', 4, 1013981571636, 1013980654091, 70, 4, 1013981072280, 70, 103, 0),
         ('127.0.0.1', 4, 1013981571636, 1013981251417, 72, 6, 1013981476623, 72, 105, 0),)

TRAINING_TRACE_SQL = "create table if not exists {0} (host_id int, device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, " \
                     "BP_end int, BP_stream int, BP_task int, iteration_end int, iter_stream int," \
                     "iter_task int, iteration_time int, fp_bp_time int, grad_refresh_bound int, " \
                     "data_aug_bound int default 0, model_id int default 0)".format(TABLE_TRAINING_TRACE)

ALL_REDUCE_SQL = "create table if not exists {0}(host_id int, device_id int," \
                 "iteration_end int, start int, start_stream int, start_task int," \
                 "end int, end_stream int, end_task int, model_id int default 0)".format(TABLE_ALL_REDUCE)


class TestStepTraceViewer(unittest.TestCase):

    def test_add_reduce_headers_should_return_headers_with_length_is_1(self):
        test_message = {'job_id': 'job_default', 'device_id': '4'}
        headers = ["Model ID"]
        sql = "create table if not exists {0}(host_id int, device_id int," \
              "iteration_end int, start int, start_stream int, start_task int," \
              "end int, end_stream int, end_task int, model_id int default 0)".format(
            TABLE_ALL_REDUCE)
        db_name = "test_add_reduce_headers_should_return_headers_with_length_is_1_" + DB_TRACE
        with DBOpen(db_name) as db_open:
            db_open.create_table(sql)
            StepTraceViewer.add_reduce_headers(db_open.db_conn, headers, test_message)
            self.assertEqual(len(headers), 1)

    def test_add_reduce_headers_should_return_headers_with_length_is_5(self):
        test_message = {'job_id': 'job_default', 'device_id': '4'}
        headers = ["Model ID"]
        db_name = "test_add_reduce_headers_should_return_headers_with_length_is_5_" + DB_TRACE
        with DBOpen(db_name) as db_open:
            db_open.create_table(TRAINING_TRACE_SQL)
            db_open.create_table(ALL_REDUCE_SQL)
            db_open.insert_data(TABLE_TRAINING_TRACE, DATA1)
            db_open.insert_data(TABLE_ALL_REDUCE, DATA2)
            StepTraceViewer.add_reduce_headers(db_open.db_conn, headers, test_message)
            self.assertEqual(len(headers), 5)

    def test_add_reduce_headers_should_return_headers_with_length_is_11(self):
        data = (
            ('127.0.0.1', 4, 1013981571636, 1013980654091, 70, 4, 1013981072280, 70, 103, 0),
            ('127.0.0.1', 4, 1013981571636, 1013981251417, 72, 6, 1013981476623, 72, 105, 0),
            ('127.0.0.1', 4, 1013981571636, 1013983452167, 74, 8, 1013981476623, 74, 107, 0),
        )
        test_message = {'job_id': 'job_default', 'device_id': '4'}
        headers = ["Model ID"]
        db_name = "test_add_reduce_headers_should_return_headers_with_length_is_11_" + DB_TRACE
        with DBOpen(db_name) as db_open:
            db_open.create_table(TRAINING_TRACE_SQL)
            db_open.create_table(ALL_REDUCE_SQL)
            db_open.insert_data(TABLE_TRAINING_TRACE, DATA1)
            db_open.insert_data(TABLE_ALL_REDUCE, DATA2)
            db_open.insert_data(TABLE_ALL_REDUCE, data)
            StepTraceViewer.add_reduce_headers(db_open.db_conn, headers, test_message)
            self.assertEqual(len(headers), 11)

    def test_get_step_trace_summary(self):
        message1 = {'job_id': 'job_default', 'device_id': '4'}
        InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info]).apply()
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_step_trace_data', side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = StepTraceViewer.get_step_trace_summary(message1)
        self.assertEqual(res, ([], [], 0))

        message1 = {'job_id': 'job_default', 'device_id': '4', 'project_path': ''}
        db_name = "test_get_step_trace_summary_" + DB_TRACE
        with DBOpen(db_name) as db_open, \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_open.db_path):
            db_open.create_table(TRAINING_TRACE_SQL)
            db_open.create_table(ALL_REDUCE_SQL)
            db_open.insert_data(TABLE_TRAINING_TRACE, DATA1)
            db_open.insert_data(TABLE_ALL_REDUCE, DATA2)
            res = StepTraceViewer.get_step_trace_summary(message1)
            self.assertEqual(res[2], 0)

    def test_step_trace_timeline(self):
        message1 = {'job_id': 'job_default', 'device_id': '4', 'project_path': ''}
        InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info]).apply()
        db_name = "test_step_trace_timeline_" + DB_TRACE
        with DBOpen(db_name) as db_open, \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_open.db_path):
            db_open.create_table(TRAINING_TRACE_SQL)
            db_open.create_table(ALL_REDUCE_SQL)
            db_open.insert_data(TABLE_TRAINING_TRACE, DATA1)
            db_open.insert_data(TABLE_ALL_REDUCE, DATA2)
            res = StepTraceViewer.get_step_trace_timeline(message1)
            self.assertEqual(len(res), 0)

    def test_get_one_iter_timeline_data(self):
        InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info]).apply()
        ProfilingScene().init("")
        db_name = "test_get_one_iter_timeline_data_" + DB_TRACE
        with DBOpen(db_name) as db_open, \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_open.db_path), \
                mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO):
            db_open.create_table(TRAINING_TRACE_SQL)
            db_open.create_table(ALL_REDUCE_SQL)
            db_open.insert_data(TABLE_TRAINING_TRACE, DATA1)
            db_open.insert_data(TABLE_ALL_REDUCE, DATA2)
            res = StepTraceViewer.get_one_iter_timeline_data("", ITER_RANGE)
            self.assertEqual(len(res), 0)

    def test_reformat_step_trace_data(self):
        data_list = [
            (1, 4223700155120, 4223701928137, 4223701928704, 35557.520000000004, 35460.340000000004, 11.34, 'N/A', 1)
        ]
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        with mock.patch(NAMESPACE + '.StepTraceViewer._StepTraceViewer__select_reduce',
                        return_value=[(4248168266400, 4248177564931)]), \
                mock.patch(NAMESPACE + '.StepTraceConstant.syscnt_to_micro',
                           return_value=0):
            res = StepTraceViewer._reformat_step_trace_data(data_list, ITER_RANGE)
            self.assertEqual(res,
                             [[1, '42237001561.200\t', '42237019291.370\t', '42237019297.040\t',
                               355.57520000000005, 354.6034, 0.1134, 'N/A', 1, '42481682674.000\t', 0]])

    def test_get_trace_timeline_data(self):
        values = [
            (1, 189746302368077, 0, 189746320443000, 19802989, 0, 0, 2422963027, 9)
        ]
        get_next_value = [
            (189746300646091, 189746300646091),
            (189746300646091,),
        ]
        all_reduce_value = [
            (189746300646091, 189746300646091),
            (189746300646091,),
        ]
        InfoConfReader()._info_json = {"pid": 0}
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        with mock.patch(NAMESPACE + '.StepTraceViewer._StepTraceViewer__select_getnext', return_value=get_next_value), \
                mock.patch(NAMESPACE + '.StepTraceViewer._StepTraceViewer__select_reduce',
                           return_value=all_reduce_value), \
                mock.patch(NAMESPACE + '.StepTraceViewer.transfer_trace_unit'), \
                mock.patch(NAMESPACE + '.StepTraceConstant.syscnt_to_micro', return_value=1):
            res = StepTraceViewer.get_trace_timeline_data(EmptyClass(""), values)
            self.assertEqual(len(json.dumps(res)), 1874)


if __name__ == '__main__':
    unittest.main()
