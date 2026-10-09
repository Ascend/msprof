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
from common_func.info_conf_reader import InfoConfReader
from sqlite.db_manager import DBManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from viewer.training.task_op_viewer import TaskOpViewer

NAMESPACE = 'viewer.training.task_op_viewer'
message = {
    "result_dir": '',
    "device_id": "4",
    "sql_path": '',
}


class TestTaskOpViewer(unittest.TestCase):

    def test_get_task_op_summary_empty(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_task_data_summary', return_value=([], 0)), \
                mock.patch(NAMESPACE + '.TaskOpViewer._add_memcpy_data', return_value=[]):
            _, _, count = TaskOpViewer.get_task_op_summary(message)
            self.assertEqual(count, 0)

    def test_get_task_op_summary_should_match_2aicore_and_1memcpy(self):
        task_data_summary = [
            ('trans_TransData_0', 'AI_CORE', 148, 3, '7.62', '155149754006240', '155149754013860'),
            ('trans_TransData_2', 'AI_CORE', 148, 6, '2.01', '155151161417390', '155151161419400'),
        ]

        expect_headers = [
            "kernel_name", "kernel_type", "stream_id", "task_id",
            "task_time(us)", "task_start(us)", "task_stop(us)",
        ]
        expect_data = [
            ('trans_TransData_0', 'AI_CORE', 148, 3, '7.62', '155149754006240', '155149754013860'),
            ('trans_TransData_2', 'AI_CORE', 148, 6, '2.01', '155151161417390', '155151161419400'),
            ("MemcopyAsync", "other", 11, 12, "100", "2200", "2300"),
        ]
        InfoConfReader()._start_info = {"collectionTimeBegin": '155149754000000'}
        InfoConfReader()._end_info = {"collectionTimeEnd": '155149754100000'}
        InfoConfReader()._info_json = {'pid': 1}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_task_data_summary',
                           return_value=(task_data_summary, len(task_data_summary))), \
                mock.patch('viewer.memory_copy.memory_copy_viewer.'
                           'MemoryCopyViewer.get_memory_copy_non_chip0_summary',
                           return_value=[("MemcopyAsync", "other", 11, 12, "100", "2200", "2300")]):
            headers, data, count = TaskOpViewer.get_task_op_summary(message)
            self.assertEqual(expect_headers, headers)
            self.assertEqual(len(expect_data), count)
            self.assertEqual(expect_data, data)
        InfoConfReader()._start_info.clear()
        InfoConfReader()._end_info.clear()
        InfoConfReader()._info_json.clear()

    def test_get_task_data_summary_success(self):
        message = {
            'result_dir': './data',
            'device_id': 2
        }
        ascend_task_data = [
            TopDownTask(1, 1, 1, 1001, 4294967295, 0, 1738412000000, 120, "", "", 9001),
            TopDownTask(1, 2, 1, 1002, 4294967295, 0, 1738412000150, 95, "", "", 9001),
            TopDownTask(2, 1, 1, 1002, 0, 0, 1738412000300, 350, "", "", 9002),
            TopDownTask(2, 1, 1, 1003, 4294967295, 0, 1738412000300, 350, "", "", 900),
            TopDownTask(2, 1, 1, 1003, 0, 0, 1738412000300, 350, "", "", 9002),
            TopDownTask(2, 1, 1, 1003, 1, 0, 1738412000300, 350, "", "", 9002),
        ]

        ge_info = [
            GeTaskDto(0, 0, 4294967295, 0, 0, 0, 'aclnn', '', 'AI_CORE', 1, 1001, 4455616449, 0, 0),
            GeTaskDto(0, 0, 4294967295, 0, 0, 0, 'aclnn', '', 'AI_CORE', 1, 1002, 0, 0, 0),
            GeTaskDto(0, 0, 0, 0, 0, 0, 'aclnn', '', 'AI_CORE', 1, 1002, 0, 0, 0),
        ]

        host_task = [
            ['', '', 1, 1001, '4294967295', 0, '', 'ConcatD_0d745295987e1c668b2ec8b2844aeada_high_performance_4000001'],
            ['', '', 1, 1002, '4294967295,0', 0, '', 'ConcatD_0d745295987e1c668b2ec8b2844aeada_high_performance_4000001'],
        ]

        with mock.patch("msmodel.task_time.ascend_task_model.AscendTaskModel.get_ascend_task_data_without_unknown", return_value=ascend_task_data), \
         mock.patch("os.path.exists", return_value=True), \
         mock.patch("msmodel.ge.ge_info_calculate_model.GeInfoModel.get_task_info", return_value=ge_info), \
         mock.patch("msmodel.runtime.runtime_host_task_model.RuntimeHostTaskModel.get_host_tasks", return_value=host_task):
            task_info, length = TaskOpViewer.get_task_data_summary(message)
            self.assertEqual(length, 2)
            self.assertEqual(task_info[0][0], "ConcatD_0d745295987e1c668b2ec8b2844aeada_high_performance_4000001")
            self.assertEqual(task_info[0][1], 4455616449)


    def test_get_task_data_summary_should_return_empty_when_miss_ascend_task(self):
        message = {
            'result_dir': './data',
            'device_id': 2
        }
        with mock.patch("common_func.db_manager.DBManager.judge_table_exist", return_value=False):
            TaskOpViewer.get_task_data_summary(message)

        with mock.patch("common_func.db_manager.DBManager.judge_table_exist", return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[]):
            TaskOpViewer.get_task_data_summary(message)

if __name__ == '__main__':
    unittest.main()
