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
from collections import deque
from unittest import mock

from common_func.constant import Constant
from common_func.data_manager import DataManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.ut_db_name_constant import DB_AICORE_OP_SUMMARY
from constant.ut_db_name_constant import DB_OP_COUNTER
from constant.ut_db_name_constant import TABLE_OP_COUNTER_OP_REPORT
from mscalculate.hccl.hccl_task import HcclOps
from sqlite.db_manager import DBOpen
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_op_report import ReportOPCounter

NAMESPACE = 'viewer.ai_core_op_report'
config = {'handler': '_get_op_summary_data_with_headers',
          'headers': ['Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type', 'OP State',
                      'Task Type', 'Task Start Time(us)', 'Task Duration(us)', 'Task Wait Time(us)', 'Block Num'],
          'db': 'ai_core_op_summary.db', 'unused_cols': []}


class TestAiCoreOpReport(unittest.TestCase):
    def test_get_op_summary_data1(self):
        try:
            with mock.patch("os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + '.HcclViewModel.init', return_value=True), \
                    mock.patch(NAMESPACE + '.HcclViewModel.finalize', return_value=None), \
                    mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_data_by_group', return_value=[]):
                check = AiCoreOpReport()
                ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
                res = check.get_op_summary_data("0", "", config)
            expect_res = [
                'Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type', 'OP State',
                'Task Type', 'Task Start Time(us)', 'Task Duration(us)', 'Task Wait Time(us)'
            ]
            self.assertEqual(res[0], expect_res)
            self.assertEqual(res[1], [])
            self.assertEqual(res[2], 0)
        finally:
            ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_get_ai_core_op_summary_data(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data_with_headers', return_value=[]):
            check = AiCoreOpReport()
            ProfilingScene()._scene = Constant.SINGLE_OP
            res = check.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], ['Context ID']))

        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data_with_headers', return_value=[]):
            check = AiCoreOpReport()
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.STEP_INFO
            res = check.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], ['Context ID']))

    def test_get_ai_core_op_summary_data_1(self):
        res = AiCoreOpReport.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], ['Context ID']))

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data_with_headers', side_effect=TypeError):
            res = AiCoreOpReport.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], []))

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = AiCoreOpReport.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], ['Context ID']))

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = AiCoreOpReport.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], ['Context ID']))

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data_with_headers', return_value=([], [])), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.STEP_INFO
            ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
            res = AiCoreOpReport.get_ai_core_op_summary_data_with_headers('', '', [])
        self.assertEqual(res, ([], []))
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_update_model_name_and_infer_id(self):
        data = [[1, 2, 9, 'global_step/Assign', 'Assign', 'AI_CORE',
                 99655949576850, 3.4, 0.0, 1, '', 'INT64;INT64', 'FORMAT_ND;FORMAT_ND', '',
                 'INT64', 'FORMAT_ND', 0.002988, 2988, 0, 0, 0, 0, 0, 0, 0, 0]]
        with mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={}):
            res = AiCoreOpReport._update_model_name_and_infer_id('', data)
        self.assertEqual(len(res), 1)

    def test_update_op_name_from_hash(self):
        data = [[1, 2, 9, 'global_step/Assign', 'Assign', 'AI_CORE',
                 99655949576850, 3.4, 0.0, 1, '', 'INT64;INT64', 'FORMAT_ND;FORMAT_ND', '',
                 'INT64', 'FORMAT_ND', 0.002988, 2988, 0, 0, 0, 0, 0, 0, 0, 0]]
        with mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={}):
            res = AiCoreOpReport._update_op_name_from_hash('', data)
        self.assertEqual(len(res), 1)

    def test_get_aicore_data(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._count_num', return_value=2), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_used_headers', return_value=[]), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_ai_core_float_cols', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            headers = config.get("headers")
            res = AiCoreOpReport._get_aicore_data('', headers)
        self.assertEqual(res[0], {})

        with mock.patch(NAMESPACE + '.AiCoreOpReport._count_num', return_value=2), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            headers = config.get("headers")
            res = AiCoreOpReport._get_aicore_data('', headers)
        self.assertEqual(res[0], {})

    def test_get_sql_and_headers(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_tensor_table_sql_and_headers', return_value=(1, 2)):
            res = AiCoreOpReport._get_sql_and_headers(config.get("headers"))
        self.assertEqual(res[0], 1)

    def test_get_op_summary_data(self):
        create_ge_sql = "CREATE TABLE IF NOT EXISTS ge_summary(model_name text,model_id INTEGER,task_id INTEGER," \
                        "stream_id INTEGER, op_name text,op_type text,op_state text," \
                        "block_num INTEGER,input_shapes text," \
                        "input_data_types text,input_formats text,output_shapes text,output_data_types text," \
                        "output_formats text,device_id INTEGER,task_type text,index_id INTEGER)"
        union_sql = "select * from ge_summary where ge_summary.index_id=? and ge_summary.task_type!=?"
        ge_data = ((0, 4294967295, 3, 6, "TransData", "TransData", 'static',
                    32, "1,512,1,1", "DT_FLOAT", "NCHW", "1,32,1,1,16", "DT_FLOAT", "NC1HWC0", 0, "AI_CORE", 1),)
        with DBOpen(DB_AICORE_OP_SUMMARY) as db_open:
            db_open.create_table(create_ge_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_data)
            with mock.patch(NAMESPACE + '.AiCoreOpReport._get_sql_and_headers', return_value=(union_sql, [])), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._get_aicore_data', return_value=([], [])), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._union_task_ge_ai_core_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._update_model_name_and_infer_id', return_value=[]), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._update_op_name_from_hash',
                               return_value=[]), \
                    mock.patch(NAMESPACE + '.add_aicore_units', return_value=[]):
                headers = config.get("headers")
                ProfilingScene().init('')
                ProfilingScene()._scene = Constant.SINGLE_OP
                res = AiCoreOpReport._get_op_summary_data_with_headers('', db_open.db_curs, headers)
            self.assertEqual(res, ([], []))

    def test_union_task_ge_ai_core_data(self):
        op_state = "static"
        expect_res = [
            (1, 2, 3, 11, 16, op_state, 4, 10),
            (4, 5, 6, 11, 16, op_state, 7, 40),
            (7, 8, 9, 11, 16, op_state, 10, "N/A")
        ]
        data = [
            (1, 2, 3, 11, 16, op_state, 4, -1),
            (4, 5, 6, 11, 16, op_state, 7, -1),
            (7, 8, 9, 11, 16, op_state, 10, -1)
        ]
        ai_core_group_dict = {(2, 3, 4): deque([(10,)]), (5, 6, 7): deque([(40,)]), (10, 11, 12): deque([(20,)])}
        AiCoreOpReport.IS_PMU_UNIQUE_ID = False
        res = AiCoreOpReport._union_task_ge_ai_core_data(data, ai_core_group_dict)
        self.assertEqual(res, expect_res)
        AiCoreOpReport.IS_PMU_UNIQUE_ID = True

    def test_format_summary_data(self):
        headers = ['Task Start Time(us)', 'Task Duration(us)', 'Task Wait Time(us)']
        data = [
            [68962865935627, 205464.1015620, 0], [68962866127471.1, 2679357453.55469, 0],
            [68962866128490.1, 2662291832.59375, 0]
        ]
        expect = [
            ['68962865945.627\t', 205.464, 0], ['68962866137.471\t', 2679357.454, 0.0],
            ['68962866138.490\t', 2662291.833, 0.0]
        ]
        check = AiCoreOpReport()
        res = check._format_summary_data(headers, data)
        self.assertEqual(res, expect)

    def test_union_task_ge_ai_core_data_success_when_exist_hardware_op_list(self):
        op_state = "dynamic"
        expect_res = [
            (1, 2, 3, "11", 16, op_state, "AI_CPU", "N/A"),
            (4, 5, 6, "11", 16, op_state, "AI_CPU", "N/A"),
            (7, 8, 9, "11", 16, op_state, 10, "N/A")
        ]
        data = [
            (1, 2, 3, "11", 16, op_state, "AI_CPU", 0),
            (4, 5, 6, "11", 16, op_state, "AI_CPU", 0),
            (7, 8, 9, "11", 16, op_state, 10, 0),
            (1, 2, 3, "11", 16, op_state, "COMMUNICATION", 0),
            (1, 3, 3, "11", 16, op_state, "HCCL_AI_CPU", 0)
        ]
        ai_core_group_dict = {
            (0, 2, 3, "AI_CPU"): deque([(10,)]), (0, 5, 6, "AI_CPU"): deque([(40,)]),
            (0, 10, 11, 12): deque([(20,)]), (0, 2, 3, "COMMUNICATION"): deque([(50,)])
        }
        res = AiCoreOpReport._union_task_ge_ai_core_data(data, ai_core_group_dict)
        self.assertEqual(res, expect_res)

    def test_union_task_ge_ai_core_data_should_return_3_data_when_have_no_pmu_data(self):
        op_state = "N/A"
        expect_res = [
            (1, 2, 3, "AI_CPU", 16, op_state, "AI_CPU"),
            (4, 5, 6, "AI_CPU", 16, op_state, "AI_CPU"),
            (7, 8, 9, "10", 16, op_state, 10),
            (7, 8, 9, "AivKernel", 16, op_state, "COMMUNICATION"),
        ]
        data = [
            (1, 2, 3, "AI_CPU", 16, op_state, "AI_CPU", 0),
            (4, 5, 6, "AI_CPU", 16, op_state, "AI_CPU", 0),
            (7, 8, 9, "10", 16, op_state, 10, 0),
            (1, 2, 3, "COMMUNICATION", 16, op_state, "COMMUNICATION", 0),
            (1, 3, 3, "HCCL_AI_CPU", 16, op_state, "HCCL_AI_CPU", 0),
            (7, 8, 9, "AivKernel", 16, op_state, "COMMUNICATION", 0),
        ]
        ai_core_group_dict = {}
        res = AiCoreOpReport._union_task_ge_ai_core_data(data, ai_core_group_dict)
        self.assertEqual(res, expect_res)

    def test_count_num(self):
        curs = mock.Mock()
        curs.execute.return_value = mock.Mock()
        curs.execute.return_value.fetchone.return_value = [0]
        res = AiCoreOpReport._count_num("test", curs)
        self.assertEqual(res, 0)

    def test_get_used_headers(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data",
                        return_value=[[None, "r1"], [None, "r2"], [None, "1"], [None, "2"]]):
            res = AiCoreOpReport._get_used_headers(mock.Mock(), "test", ["1", "2"])
        self.assertEqual(res, ["r1", "r2"])

    def test_get_ai_core_float_cols(self):
        with mock.patch(NAMESPACE + ".read_cpu_cfg", return_value={1: "r1", 2: "r2_ratio"}):
            res = AiCoreOpReport._get_ai_core_float_cols(["r1", "r2_time"])
        self.assertEqual(res, ['round(r1, 3)', 'round(r2_time, 3)'])

    def test_add_cube_usage(self):
        InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 20, 'aic_frequency': 1800}]}
        headers = ["id", "mac_ratio", "total_cycles", "Task Duration(us)"]
        data = [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]]
        DataManager.add_cube_usage(headers, data)
        self.assertEqual(data, [[1, 0.668, 401282788, 11437780, 97.455], [2, 0.624, 341282228, 10237240, 92.604]])

    def test_add_cube_usage_failed_when_info_json_data_invalid(self):
        InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 0, 'aic_frequency': 18}]}
        headers = ["id", "mac_ratio", "total_cycles", "Task Duration(us)"]
        data = [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]]
        DataManager.add_cube_usage(headers, data)
        self.assertEqual(data, [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]])
        InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 2, 'aic_frequency': ""}]}
        DataManager.add_cube_usage(headers, data)
        self.assertEqual(data, [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]])
        InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 2, 'aic_frequency': "abc"}]}
        DataManager.add_cube_usage(headers, data)
        self.assertEqual(data, [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]])
        InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 2, 'aic_frequency': "0.00000"}]}
        DataManager.add_cube_usage(headers, data)
        self.assertEqual(data, [[1, 0.668, 401282788, 11437780], [2, 0.624, 341282228, 10237240]])

    def test_add_memory_bound(self):
        headers = ["mac_ratio", "vec_ratio", "mte2_ratio"]
        data = [[1, 2, 3], [5, 6, 2]]
        DataManager.add_memory_bound(headers, data)
        headers = ["mac_exe_ratio", "vec_exe_ratio", "mte2_exe_ratio"]
        data = [[1, 2, 3], [5, 6, 2]]
        DataManager.add_memory_bound(headers, data)

    def test_get_hccl_op_data_should_concat_hccl_and_kfc_op(self):
        hccl_op = HcclOps(model_id=1, index_id=0, op_name="hcom_allReduce", op_type="hcom_allReduce_",
                          start=100, end=110)
        kfc_op = HcclOps(model_id=2, index_id=0, op_name="mc2_matmul_allreduce", op_type="MatmulAllReduce",
                         start=200, end=220)

        def fake_get_hccl_op_data_by_group(table_name=DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE):
            if table_name == DBNameConstant.TABLE_KFC_OP:
                return [kfc_op]
            return [hccl_op]

        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.finalize', return_value=None), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_data_by_group',
                           side_effect=fake_get_hccl_op_data_by_group), \
                mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={}):
            res = AiCoreOpReport.get_hccl_op_data("proj", 11)
        self.assertEqual(len(res), 2)
        self.assertEqual(res[0][3], "hcom_allReduce")
        self.assertEqual(res[0][6], Constant.TASK_TYPE_COMMUNICATION)
        self.assertEqual(res[0][7], 100)
        self.assertEqual(res[0][8], 10)
        self.assertEqual(res[1][3], "mc2_matmul_allreduce")
        self.assertEqual(res[1][6], Constant.TASK_TYPE_COMMUNICATION)
        self.assertEqual(res[1][7], 200)
        self.assertEqual(res[1][8], 20)

    def test_get_hccl_op_data_should_keep_hccl_when_kfc_empty(self):
        hccl_op = HcclOps(model_id=1, index_id=0, op_name="hcom_allReduce", op_type="hcom_allReduce_",
                          start=100, end=110)

        def fake_get_hccl_op_data_by_group(table_name=DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE):
            if table_name == DBNameConstant.TABLE_KFC_OP:
                return []
            return [hccl_op]

        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.finalize', return_value=None), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_data_by_group',
                           side_effect=fake_get_hccl_op_data_by_group), \
                mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={}):
            res = AiCoreOpReport.get_hccl_op_data("proj", 11)
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0][3], "hcom_allReduce")

    def test_get_hccl_op_data_should_keep_kfc_when_hccl_empty(self):
        kfc_op = HcclOps(model_id=2, index_id=0, op_name="mc2_matmul_allreduce", op_type="MatmulAllReduce",
                         start=200, end=220)

        def fake_get_hccl_op_data_by_group(table_name=DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE):
            if table_name == DBNameConstant.TABLE_KFC_OP:
                return [kfc_op]
            return []

        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.finalize', return_value=None), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_data_by_group',
                           side_effect=fake_get_hccl_op_data_by_group), \
                mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={}):
            res = AiCoreOpReport.get_hccl_op_data("proj", 11)
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0][3], "mc2_matmul_allreduce")
        self.assertEqual(res[0][6], Constant.TASK_TYPE_COMMUNICATION)


class TestReportOPCounter(unittest.TestCase):
    def test_check_param(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = ReportOPCounter.check_param('', '')
        self.assertEqual(res, False)

    def test_report_op_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = ReportOPCounter.report_op('', [])
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_report_op_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + TABLE_OP_COUNTER_OP_REPORT + \
                     "(model_name, op_type, core_type, occurrences, total_time, " \
                     "min, avg, max, ratio, device_id)"
        data = ((1, 4, 1695387952406, 1695385794913, 59, 4, 1695386978300, 59, 103, 0),)

        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_OP_REPORT, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.isinstance', return_value=False):
                ProfilingScene()._scene = Constant.STEP_INFO
                res = ReportOPCounter.report_op('', [])
            self.assertEqual(res[2], 1)

        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_OP_REPORT, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.isinstance', return_value=False):
                check = ReportOPCounter()
                ProfilingScene().init('')
                ProfilingScene()._scene = Constant.SINGLE_OP
                res = check.report_op('', config.get("headers"))
            self.assertEqual(res[2], 1)

    def test_format_statistic_data(self):
        headers = ['Total Time(us)', 'Min Time(us)', 'Avg Time(us)', 'Max Time(us)', 'Ratio(%)']
        data = [[192421, 11452, 22321, 82532, 1], [321346, 34324, 88135, 180900, 1]]
        expect = [[192.421, 11.452, 22.321, 82.532, 1], [321.346, 34.324, 88.135, 180.9, 1]]
        check = ReportOPCounter()
        res = check._format_statistic_data(data, headers)
        self.assertEqual(res, expect)

        headers = ['Total Time(us)', 'Min Time(us)', 'Avg Time(us)']
        res = check._format_statistic_data(data, headers)
        self.assertEqual(res, expect)

    def test_get_op_summary_data(self):
        headers = [AiCoreOpReport.TASK_START_TIME, AiCoreOpReport.TASK_DURATION, AiCoreOpReport.TASK_WAIT_TIME]
        tasks = [['-1', '0', 1], ['0', '2', 1], ['2', '3', 1], ['3', '4', 1]]
        InfoConfReader()._start_info = {'collectionTimeBegin': '1'}
        InfoConfReader()._end_info = {'collectionTimeEnd': '10'}
        InfoConfReader()._info_json = {'platform_version': '5'}
        with mock.patch(NAMESPACE + '.AiCoreOpReport.get_op_summary_header', return_value=[]), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_ai_core_op_summary_data_with_headers', return_value=([], headers)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_hccl_op_data', return_value=[]), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._format_summary_data', return_value=tasks):
            res = AiCoreOpReport.get_op_summary_data('', '', {})
            self.assertEqual(3, res[2])
            self.assertEqual(['0', '2', 0], res[1][0])

if __name__ == '__main__':
    unittest.main()
