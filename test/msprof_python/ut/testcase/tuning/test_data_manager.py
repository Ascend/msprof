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

import sqlite3
import unittest
from unittest import mock

from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from tuning.data_manager import DataManager, OpSummaryTuningDataHandle, OpParallelTuningDataHandle
from tuning.data_manager import ModelSummaryTuningDataHandle

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}
NAMESPACE = 'tuning.data_manager'


class TestDataManager(unittest.TestCase):
    def test_get_data(self):
        para = {
            'project': '',
            'device_id': '0',
        }
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_op_summary_data', return_value=False):
            key = DataManager(para)
            key.data = {'type1': [1, 2]}
            result1 = key.get_data('type1')
            self.assertEqual(result1, [1, 2])
            result2 = key.get_data('type2')
            self.assertEqual(result2, [])


class TestDataLoader(unittest.TestCase):
    def test_get_data_by_infer_id(self):
        project_path = 'home\\data_loader'
        device_id = 123
        headers = ['device_id', 'infer_id', 'project_path']
        datas = [[0, 0, 'home\\data_manager']]
        check = [{'device_id': 0, 'infer_id': 0, 'project_path': 'home\\data_manager'}]
        with mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle.select_memory_workspace', return_value=[5, 4, 0]), \
                mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle._get_base_data', return_value=(headers, datas)), \
                mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle._get_extend_data'), \
                mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle.get_memory_workspace'):
            key = OpSummaryTuningDataHandle()
            para = {
                'project': project_path,
                'device_id': device_id,
            }
            result = key.get_data_by_infer_id(para)
        self.assertEqual(result, check)

    def test_get_vector_bound(self):
        extend_data_dict = {"vector_bound": 1, "mte2_ratio": 2, "mac_ratio": 3}
        operator_dict = {"vec_ratio": 4, "mte2_ratio": 5, "mac_ratio": 6}
        OpSummaryTuningDataHandle.get_vector_bound(extend_data_dict, operator_dict)

    def test_get_memory_workspace_1(self):
        memory_workspaces = [[1, '123', 3]]
        operator_dict = {"stream_id": 1, "task_id": '123', "memory_workspace": 2}
        OpSummaryTuningDataHandle.get_memory_workspace(memory_workspaces, operator_dict)

    def test_get_memory_workspace_2(self):
        memory_workspaces = None
        operator_dict = {"memory_workspace": 2}
        OpSummaryTuningDataHandle.get_memory_workspace(memory_workspaces, operator_dict)

    def test_select_memory_workspace(self):
        project = 123
        device_id = 0
        create_sql = "create table IF NOT EXISTS GELoad (modelname TEXT,model_id INTEGER,device_id INTEGER," \
                     "replayid INTEGER,load_start INTEGER,load_end INTEGER,fusion_name TEXT," \
                     "fusion_op_nums INTEGER,op_names TEXT,stream_id INTEGER,memory_input INTEGER," \
                     "memory_output INTEGER,memory_weight INTEGER,memory_workspace INTEGER," \
                     "memory_total INTEGER,task_num INTEGER,task_ids TEXT)"
        data = [('resnet50', 1, 0, 0, 960265000000, 960593000000, 'conv1', 3, 'bn_conv1, conv1, scale_conv1',
                 5, 1706208, 1605664, 100544, 0, 3412416, 1, 4)]
        insert_sql = "insert into {0} values ({value})".format("GELoad", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle.is_network', return_value=False):
            result = OpSummaryTuningDataHandle.select_memory_workspace(project, device_id)
        self.assertEqual(result, [])

        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.OpSummaryTuningDataHandle.is_network', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(res[0], res[1])), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            result = OpSummaryTuningDataHandle.select_memory_workspace(project, device_id)
        self.assertEqual(result, [])

        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            OpSummaryTuningDataHandle.select_memory_workspace(project, device_id)
        res[1].execute("drop table GELoad")
        db_manager.destroy(res)

    def test_is_network(self):
        project = 123
        device_id = 0
        create_sql = "create table IF NOT EXISTS AclData (api_name text,api_type text,start_time INTEGER," \
                     "end_time INTEGER,process_id INTEGER,thread_id INTEGER,device_id INTEGER)"
        data = [(None, None, 117962568517, 118405906350, 2244, 2246, None)]
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_ACL_DATA, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                result = OpSummaryTuningDataHandle.is_network(project, device_id)
            self.assertEqual(result, True)

            curs = mock.Mock()
            curs.execute.side_effect = sqlite3.Error
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(curs, curs)), \
                    mock.patch(NAMESPACE + '.logging.error'):
                OpSummaryTuningDataHandle.is_network(project, device_id)

            with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                OpSummaryTuningDataHandle.is_network(project, device_id)

    def test_print_format(self):
        data = {
            "AI CPU Execution Time(us)": 0
        }
        formatted_print = OpParallelTuningDataHandle.print_format(data)
        self.assertEqual(formatted_print,
                         "Percentage of AI CPU Execution Time is 0.00%, Exceed the experience threshold 5%.")
        data = {
            "AI CPU Execution Time(us)": 2.5,
            "others": 97.5
        }
        formatted_print = OpParallelTuningDataHandle.print_format(data)
        self.assertEqual(formatted_print,
                         "Percentage of AI CPU Execution Time is 2.50%, Exceed the experience threshold 5%.")


class TestModelSummaryTuningDataHandle(unittest.TestCase):
    def test_load_data_incorrect_metrics(self):
        param = {
            StrConstant.SAMPLE_CONFIG:
                {
                    StrConstant.AI_CORE_PROFILING_METRICS: 'xxx'
                }
        }
        handler = ModelSummaryTuningDataHandle()
        ret = handler.load_data(param, [])
        self.assertEqual(ret, [])

    def test_load_data_correct_metrics_no_ops(self):
        param = {
            StrConstant.SAMPLE_CONFIG:
                {
                    StrConstant.AI_CORE_PROFILING_METRICS: AiCoreMetricsManager.PMU_PIPE
                }
        }
        handler = ModelSummaryTuningDataHandle()
        with mock.patch(NAMESPACE + '.ModelSummaryTuningDataHandle.get_data_by_infer_id', return_value=[]):
            ret = handler.load_data(param, [])
            self.assertEqual(ret, [])

    def test_load_data_correct_metrics_with_ops(self):
        param = {
            StrConstant.SAMPLE_CONFIG:
                {
                    StrConstant.AI_CORE_PROFILING_METRICS: AiCoreMetricsManager.PMU_PIPE
                }
        }
        op1 = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.1,
            StrConstant.SCALAR_RATIO: 0.01,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        op2 = {
            StrConstant.MAC_RATIO: 0.2,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.12,
            StrConstant.SCALAR_RATIO: 0.11,
            StrConstant.TASK_DURATION: 10,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        with mock.patch(NAMESPACE + '.ModelSummaryTuningDataHandle.get_data_by_infer_id', return_value=[op1, op2]):
            ret = handler.load_data(param, [op1, op2])[0]
            self.assertEqual(ret.get(StrConstant.CUBE_UTILIZATION), 30)
            self.assertEqual(ret.get(StrConstant.VECTOR_UTILIZATION), 10)
            self.assertEqual(ret.get(StrConstant.MTE_UTILIZATION), 0)
            self.assertEqual(ret.get(StrConstant.SCALAR_UTILIZATION), 0)

    def test_get_operator_bound_type_cube(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.1,
            StrConstant.SCALAR_RATIO: 0.01,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, StrConstant.CUBE_UTILIZATION)

    def test_get_operator_bound_type_vector(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.54,
            StrConstant.MTE1_RATIO: 0.1,
            StrConstant.SCALAR_RATIO: 0.01,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, StrConstant.VECTOR_UTILIZATION)

    def test_get_operator_bound_type_mte(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.5,
            StrConstant.SCALAR_RATIO: 0.01,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, StrConstant.MTE_UTILIZATION)

    def test_get_operator_bound_type_scalar(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.1,
            StrConstant.SCALAR_RATIO: 0.41,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, StrConstant.SCALAR_UTILIZATION)

    def test_get_operator_bound_type_op_ratio_all_0(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0,
            StrConstant.VEC_RATIO: 0,
            StrConstant.MTE1_RATIO: 0,
            StrConstant.SCALAR_RATIO: 0,
            StrConstant.TASK_DURATION: 0,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CORE
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, Constant.NA)

    def test_get_operator_bound_type_op_aicpu(self):
        op_dict = {
            StrConstant.MAC_RATIO: 0.3,
            StrConstant.VEC_RATIO: 0.24,
            StrConstant.MTE1_RATIO: 0.1,
            StrConstant.SCALAR_RATIO: 0.41,
            StrConstant.TASK_DURATION: 30,
            Constant.TASK_TYPE: Constant.TASK_TYPE_AI_CPU
        }
        handler = ModelSummaryTuningDataHandle()
        bound_type = handler.get_operator_bound_type(op_dict)
        self.assertEqual(bound_type, Constant.NA)


if __name__ == '__main__':
    unittest.main()
