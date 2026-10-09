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
import os
import sqlite3
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager, DBOpen
from viewer.runtime_report import _get_output_event_counter
from viewer.runtime_report import add_memcpy_data
from viewer.runtime_report import add_op_total
from viewer.runtime_report import add_ts_opname
from viewer.runtime_report import cal_metrics
from viewer.runtime_report import get_opname
from viewer.runtime_report import get_output_tasktype
from viewer.runtime_report import get_task_based_core_data
from viewer.runtime_report import get_task_scheduler_data

NAMESPACE = 'viewer.runtime_report'
configs = {"headers": "Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)",
           "columns": "duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,txPacket,"
                      "txErrorRate,txDroppedRate,funcId"}
params = {'data_type': '',
          'project': '', 'device_id': "0",
          'job_id': 'job_default',
          'export_type': 'summary', 'iter_id': 1,
          'export_format': 'csv', 'model_id': 1}


class TestRuntimeReport(unittest.TestCase):

    def test_get_task_scheduler_data_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("runtime.db")
        tmp_config = {"headers": "Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                                 "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID"}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_task_scheduler_data('', "ReportTask", tmp_config, params)
        self.assertEqual(res, ("Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                               "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID", [], 0))
        db_manager.destroy(test_sql)

    def test_get_task_scheduler_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ReportTask" \
                     " (timeratio REAL,time REAL,count INTEGER,avg REAL,min REAL,max REAL,waiting REAL,running REAL," \
                     "pending REAL,type TEXT,api TEXT,task_id INTEGER,stream_id INTEGER,device_id)"
        data = ((0.1, 1, 2, 1, 1, 5, 3, 1, 0.5, "aicore", "a", 0, 0, 0),)
        tmp_config = {"headers": "Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                                 "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID"}
        db_name = "test_get_task_scheduler_data_2_runtime.db"
        with DBOpen(db_name) as db_open, \
                mock.patch(NAMESPACE + '.add_memcpy_data', return_value=data):
            db_open.create_table(create_sql)
            db_open.insert_data("ReportTask", data)
            res = get_task_scheduler_data(db_open.db_path, "ReportTask", tmp_config, params)
            self.assertEqual(res[2], 1)

    def test_add_memcpy_data(self):
        data = [(50, 10000, 1, 10000, 10000,
                 10000, 116.770796875, 47177.968796875, 0.0, 'model execute task', '', 2, 2),
                (50, 10000, 1, 10000, 10000,
                 10000, 0.0, 1596.19790625, 0.0, 'kernel AI core task', '', 3, 5)]
        memcpy_summary = [(0, 20000, 1, 20000, 20000, 20000, 100, 200, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                           MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                           MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)]
        expect_res = [(25.0, 10000, 1, 10000, 10000,
                       10000, 116.770796875, 47177.968796875, 0.0, 'model execute task', '', 2, 2),
                      (25.0, 10000, 1, 10000, 10000,
                       10000, 0.0, 1596.19790625, 0.0, 'kernel AI core task', '', 3, 5),
                      (50.0, 20000, 1, 20000, 20000, 20000, 100, 200, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                       MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                       MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)
                      ]

        with mock.patch('viewer.memory_copy.memory_copy_viewer.'
                        'MemoryCopyViewer.get_memory_copy_chip0_summary', return_value=memcpy_summary):
            res = add_memcpy_data("./", data)
        self.assertEqual(expect_res, res)

    def test_add_ts_opname(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_GE_INFO)
        task_data = [[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]]
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.get_opname', return_value="test"):
            res = add_ts_opname(task_data, "")
        self.assertEqual(len(res), 1)

    def test_get_task_based_core_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, {})
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, {})
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_task_based_core_data_2(self):
        params["data_type"] = StrConstant.AI_CORE_PMU_EVENTS
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_METRIC_SUMMARY + \
                     " (total_time, total_cycles, vec_time, vec_ratio, mac_time, mac_ratio, scalar_time, " \
                     "scalar_ratio, mte1_time, mte1_ratio, mte2_time, mte2_ratio, mte3_time, mte3_ratio, " \
                     "icache_miss_rate, device_id, task_id, stream_id, index_id, model_id)"
        data = ((0.0426514705882353, 58006, 0.019154, 0.449091473295866, 0, 0, 0.000387, 0.00906802744543668, 0, 0,
                 0.010296, 0.241388821846016, 0.019614, 0.459866220735786, 0.0535714285714286, 0, 3, 5, 1, 1),)
        db_name = "test_get_task_based_core_data_2_" + DBNameConstant.DB_RUNTIME
        with DBOpen(db_name) as db_open, \
                mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=configs), \
                mock.patch(NAMESPACE + '.add_op_total', return_value=[]), \
                mock.patch(NAMESPACE + '.cal_metrics', return_value=[1, 2]), \
                mock.patch('common_func.path_manager.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_METRIC_SUMMARY, data)
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            res = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME, params)
            self.assertEqual(res[0], 1)

    def test_get_task_based_core_data_3(self):
        params["data_type"] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_AIV_METRIC_SUMMARY + \
                     " ( total_time, total_cycles, vec_time, vec_ratio, mac_time, mac_ratio, scalar_time, " \
                     "scalar_ratio, mte1_time, mte1_ratio, mte2_time, mte2_ratio, mte3_time, mte3_ratio, " \
                     "icache_miss_rate, device_id, task_id, stream_id, index_id, model_id)"
        data = ((0.0426514705882353, 58001, 0.019154, 0.449091473295866, 0, 0, 0.000387, 0.00906802744543668, 0, 0,
                 0.010296, 0.241388821846016, 0.019614, 0.459866220735786, 0.0535714285714286, 0, 3, 5, 1, 1),)
        db_name = "test_get_task_based_core_data_3_" + DBNameConstant.DB_RUNTIME
        with DBOpen(db_name) as db_open, \
                mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=configs), \
                mock.patch(NAMESPACE + '._get_output_event_counter', return_value=[1, 2]), \
                mock.patch('common_func.path_manager.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_AIV_METRIC_SUMMARY, data)
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            res = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME, params)
            self.assertEqual(res[0], 1)

    def test_get_output_task_type(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=None):
            res = get_output_tasktype(test_sql[1], params)
        self.assertEqual(res, [])
        db_manager.destroy(test_sql)

    def test_get_output_event_counter(self):
        with mock.patch(NAMESPACE + '._get_output_event_counter', return_value=[]):
            res = get_output_tasktype("", params)
        self.assertEqual(res, [])

    def test_get_output_event_counter_1(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=None):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '._get_event_counter_metric_res', side_effect=TypeError), \
                mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=None):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

        with mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

    def test_get_opname_1(self):
        with mock.patch('os.path.join', side_effect=sqlite3.DatabaseError):
            res = get_opname([], "", "")
        self.assertEqual(res, Constant.NA)

    def test_get_opname_2(self):
        create_ge_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_TASK + \
                        " (device_id, model_name, model_id, op_name, stream_id, task_id, batch_id, block_num, " \
                        "op_state, task_type, op_type, iter_id, input_count, input_formats, input_data_types, " \
                        "input_shapes, output_count, output_formats, output_data_types, output_shapes)"
        data = ((0, "resnet50", 1, "trans_TransData_0", 5, 3, 0, 1, "static", "AI_CORE", "TransData", 0, 1, "NCHW",
                 "DT_FLOAT16", "1,3,224,224", 1, "NC1HWC0", "DT_FLOAT16", "1,1,224,224,16"),)
        db_name = "test_get_opname_2_" + DBNameConstant.DB_GE_INFO
        with DBOpen(db_name) as db_open, \
                mock.patch('common_func.path_manager.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_ge_sql)
            db_open.insert_data(DBNameConstant.TABLE_GE_TASK, data)
            res_dir = os.path.realpath(os.path.join(db_open.db_path, ".."))
            # [-1, 3, 5, 0]: unknown, task_id, stream_id, batch_id
            res_ge = get_opname([-1, 3, 5, 0], res_dir, db_open.db_curs)
            self.assertEqual(res_ge, 'trans_TransData_0')

    def test_get_opname_3(self):
        with mock.patch('os.path.exists', return_value=False):
            res = get_opname([3, 5, -1], "", "")
        self.assertEqual(res, 'N/A')

    def test_add_op_total_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = add_op_total([], "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)):
            res = add_op_total([], "")
        self.assertEqual(res, [])

    def test_add_op_total_2(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RTS_TRACK)
        db_manager_ge = DBManager()
        test_ge_sql = db_manager_ge.create_table(DBNameConstant.DB_GE_INFO)
        res_dir = os.path.realpath(os.path.join(db_manager.db_path, ".."))

        with mock.patch(NAMESPACE + ".get_opname", return_value=[1]):
            res = add_op_total([[3, 4, -1]], res_dir)
            res_ge = add_op_total([[3, 4, -1]], res_dir)
        db_manager_ge.destroy(test_ge_sql)
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)
        self.assertEqual(len(res_ge), 1)

    def test_cal_metrics(self):
        headers = ['Task ID', "Stream ID", "Op Name", "device_id", "x", "mac_ratio", "vec_ratio", "mte2_ratio"]
        result = [[1, 1, "resent", 0, 1, 1, 1, -1, -1, 1, 1]]
        res = cal_metrics(result, [], headers)
        self.assertEqual(len(res), 2)


if __name__ == '__main__':
    unittest.main()
