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

from common_func.db_name_constant import DBNameConstant
from mscalculate.cluster.trailing_calculator import TrailingCalculator
from sqlite.db_manager import DBManager

NAMESPACE = 'mscalculate.cluster.trailing_calculator'


def create_trace_db():
    create_ge_sql = "create table if not exists TaskInfo (device_id INTEGER," \
                    "model_name TEXT,model_id INTEGER,op_name TEXT,stream_id INTEGER," \
                    "task_id INTEGER,block_num INTEGER,op_state TEXT,task_type TEXT," \
                    "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT," \
                    "input_data_types TEXT,input_shapes TEXT,output_count INTEGER," \
                    "output_formats TEXT,output_data_types TEXT,output_shapes TEXT)"
    insert_ge_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)".format('TaskInfo')
    data = ((0, 'test', 1, 'model', 5, 3, 2, 'static', 'AI_CORE', 'trans_data',
             1, 1, '12', '1750', '1752', 0, 'test', 'test2', 'test3'),)
    db_manager = DBManager()
    res = db_manager.create_table("ge_info.db", create_ge_sql, insert_ge_sql, data)
    res[0].close()

    create_sql = "CREATE TABLE IF NOT EXISTS StepTrace (index_id INT,model_id INT,step_start REAL," \
                 " step_end INT,iter_id INT,ai_core_num INT)"
    insert_sql = "insert into {} values (?,?,?,?,?,?)".format('StepTrace')
    data = ((1, 1, 2, 3, 4, 5),)
    db_manager = DBManager()
    res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
    res[0].close()

    return db_manager


class TestTrailingCalculator(unittest.TestCase):

    def tearDown(self) -> None:
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_get_step_data(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=10):
            check = TrailingCalculator(['device_0', 'device_1'])
            self.assertEqual(check.get_step_data('test'), 10)

    def test_ms_run(self):
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.TrailingCalculator.calculate'), \
                mock.patch(NAMESPACE + '.TrailingCalculator.calculate_slow_node', return_value=[]):
            check = TrailingCalculator(['device_0', 'device_1'])
            self.assertEqual(check.run(), [])

    def test_calculate_slow_node(self):
        check = TrailingCalculator(['device_0', 'device_1'])
        check.trailing_dict = {'test_1': 10, 'test_2': 20}
        self.assertEqual(check.calculate_slow_node(),
                         {'Slow Node': ['Node: test_2, with a data augmentation bound duration of 20 ns '
                                        'on average, which is 33.33% higher than the average duration '
                                        'consumed by nodes.\t']})
        check.trailing_dict = {}
        self.assertEqual(check.calculate_slow_node(), {'Slow Node': []})
        with mock.patch(NAMESPACE + '.logging.error'):
            check.trailing_dict = {'ts': 'a'}
            self.assertEqual(check.calculate_slow_node(), {'Slow Node': []})
        check.trailing_dict = {'ts': 0}
        self.assertEqual(check.calculate_slow_node(), {'Slow Node': []})

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.TrailingCalculator.get_step_data', return_value=[(10,)]):
            check = TrailingCalculator(['device_0', 'device_1'])
            check.calculate('test')
            self.assertEqual(check.trailing_dict, {'test': 10})


if __name__ == '__main__':
    unittest.main()
