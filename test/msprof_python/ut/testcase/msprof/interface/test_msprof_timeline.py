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
import decimal
import json
import unittest
from unittest import mock

from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from constant.constant import ITER_RANGE
from msinterface.msprof_timeline import MsprofTimeline
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode

NAMESPACE = 'msinterface.msprof_timeline'


class TestMsprofTimeline(unittest.TestCase):
    def test_add_export_data(self):
        key = MsprofTimeline()

        self.assertEqual(key.add_export_data("", 'acl'), None)

        data1 = json.dumps(
            [{'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'Task Scheduler'}, 'ph': 'M'},
             {'name': 'trans_TransData_7', 'pid': 0, 'tid': '0', 'ts': 1210122,
              'args': {'Stream Id': 1, 'Task Id': 1}, 'ph': 'X'}])

        with mock.patch(
                'common_func.ai_stack_data_check_manager.AiStackDataCheckManager.contain_core_cpu_reduce_data',
                return_value=True):
            key.add_export_data(data1, 'task_time')
            with mock.patch('common_func.db_manager.DBManager.fetch_all_data',
                            return_value=(
                                    (4294967295, 'Add', 1, 1, 32, 0, 'AI_CORE', 'Add', 0, 10621, 620199010400, 0),)):
                data2 = json.dumps(
                    [{'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
                     {'name': 'aclopExecuteV2', 'pid': 0, 'tid': 0, 'ts': 620199010.277, 'dur': 569.359,
                      'args': {'Mode': 'ACL_OP'}, 'ph': 'X'}])
                key = MsprofTimeline()
                key.add_export_data(data2, 'acl')

                ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
                data = [
                    {
                        'name': 'process_name', 'pid': 1, 'tid': 1,
                        'args': {'name': 'AscendCL'}, 'ph': 'M'
                    },
                    {
                        'name': 'aclopExecuteV2', 'pid': 1, 'tid': 1,
                        'ts': 545235234.236, 'dur': 652.544, 'args': {'Mode': 'ACL_OP'}, 'ph': 'X'
                    }
                ]
                key.add_export_data(data, 'acl')
                ProfilingScene().set_mode(ExportMode.ALL_EXPORT)


    def test_export_all_data(self):
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=EmptyClass()):
            key = MsprofTimeline()
            key.export_all_data()

    def test_export_all_data_1(self):
        data = json.dumps([
            {'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'Step Trace'}, 'ph': 'M'},
            {'name': 'Reduce', 'pid': 0, 'ph': 'X', 'ts': 1210122}])
        InfoConfReader()._info_json = {"devices": '0'}
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=data):
            key = MsprofTimeline()
            key.export_all_data()

    def test_is_in_iteration_1(self):
        from msinterface.msprof_timeline import MsprofTimeline
        time_stamp = {'ts': 111, 'dur': 2}
        key = MsprofTimeline()
        key._iteration_time = [1, 4]
        result = key.is_in_iteration(time_stamp, decimal.Decimal(1), decimal.Decimal(4))
        self.assertEqual(result, False)

    def test_is_in_iteration_2(self):
        from msinterface.msprof_timeline import MsprofTimeline
        InfoConfReader()._local_time_offset = 10.0
        time_stamp = {'ts': 11, 'dur': 1}
        key = MsprofTimeline()
        key._iteration_time = [1, 13]
        result = key.is_in_iteration(time_stamp, decimal.Decimal(1), decimal.Decimal(13))
        self.assertEqual(result, True)

    def test_set_iteration_info(self):
        profiling_scene_mock = mock.Mock()
        profiling_scene_mock.is_all_export.return_value = True
        info_conf_reader_mock = mock.Mock()
        info_conf_reader_mock.get_collect_time.return_value = (0, 0)
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=0), \
                mock.patch(NAMESPACE + '.ProfilingScene', return_value=profiling_scene_mock), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=info_conf_reader_mock):
            key = MsprofTimeline()
            key._iteration_time = [[1, 2], [1, 2]]
            key.set_iteration_info('test', ITER_RANGE)

    def test_init_export_data(self):
        MsprofTimeline().init_export_data()
        self.assertEqual(MsprofTimeline()._export_data_list, [])

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}
        info_reader._host_freq = None

    def test_get_start_end_time(self):
        key = MsprofTimeline()
        key._iteration_time = (1, 13)
        res = key.get_start_end_time()
        self.assertEqual(len(res), 2)
        self.assertEqual(res, key._iteration_time)

if __name__ == '__main__':
    unittest.main()
