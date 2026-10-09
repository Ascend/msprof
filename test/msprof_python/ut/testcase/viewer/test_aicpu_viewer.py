#!/usr/bin/python3
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

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from constant.constant import ITER_RANGE
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBOpen
from viewer.aicpu_viewer import AiCpuData
from viewer.aicpu_viewer import ParseAiCpuData
from common_func.msprof_object import CustomizedNamedtupleFactory

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'viewer.aicpu_viewer'

AiCpuDataTuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(AiCpuData, [])


class TestAicpuViewer(unittest.TestCase):
    def get_aicpu_dto(self, aicpu: list) -> list:
        aicpu_dto = []
        for data in aicpu:
            data_dto = AiCpuDataTuple(stream_id=data[0], task_id=data[1], sys_start=data[2], sys_end=data[3],
                                      node_name=data[4], compute_time=data[5], memcpy_time=data[6], task_time=data[7],
                                      dispatch_time=data[8], total_time=data[9], batch_id=data[10])
            aicpu_dto.append(data_dto)
        return aicpu_dto

    def get_ge_task_dto(self, ge_task: list) -> list:
        ge_task_dto = []
        for data in ge_task:
            data_dto = GeTaskDto()
            data_dto.op_name = data[0]
            data_dto.stream_id = data[1]
            data_dto.task_id = data[2]
            data_dto.batch_id = data[3]
            ge_task_dto.append(data_dto)
        return ge_task_dto

    def get_ascend_task_dto(self, ascend_task: list) -> list:
        ascend_task_dto = []
        for data in ascend_task:
            data_dto = TaskTimeDto()
            data_dto.stream_id = data[0]
            data_dto.task_id = data[1]
            data_dto.batch_id = data[2]
            data_dto.start_time = data[3]
            data_dto.end_time = data[4]
            ascend_task_dto.append(data_dto)
        return ascend_task_dto

    def test_analysis_aicpu_when_normal_then_pass(self):
        project_path = 'home\\project'
        with mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_data'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ascend_task_ai_cpu_data'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ge_summary_aicpu_data'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_aicpu_batch_id'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.match_aicpu_with_ge_summary', return_value=[]):
            check = ParseAiCpuData()
            result = check.analysis_aicpu(project_path, ITER_RANGE)
            unittest.TestCase().assertEqual(result, [])

    def test_get_ai_cpu_data_when_connect_failed_then_return_empty(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(False, False)):
            check = ParseAiCpuData()
            result = check.get_ai_cpu_data("", ITER_RANGE)
            unittest.TestCase().assertEqual(result, [])

    def test_get_ai_cpu_data_when_connect_success_then_return_data(self):
        aicpu_data = [[2, 0, 100, 200, "N/A", 0, 0, 15, 0, 300, 0], ]
        res = self.get_aicpu_dto(aicpu_data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData._get_aicpu_data', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = ParseAiCpuData()
            result = check.get_ai_cpu_data("", ITER_RANGE)
            unittest.TestCase().assertEqual(result, res)

    def test_get_ascend_task_ai_cpu_data_when_connect_failed_then_return_empty(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(False, False)):
            check = ParseAiCpuData()
            result = check.get_ascend_task_ai_cpu_data("")
            unittest.TestCase().assertEqual(result, [])

    def test_get_ascend_task_ai_cpu_data_when_connect_success_then_return_data(self):
        ascend_task_data = [[2, 0, 0, 100, 200], ]
        res = self.get_ascend_task_dto(ascend_task_data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData._get_ascend_task_aicpu_data', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = ParseAiCpuData()
            result = check.get_ascend_task_ai_cpu_data("")
            unittest.TestCase().assertEqual(result, res)

    def test_get_ge_summary_aicpu_data_when_connect_failed_then_return_empty(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(False, False)):
            check = ParseAiCpuData()
            result = check.get_ge_summary_aicpu_data("")
            unittest.TestCase().assertEqual(result, [])

    def test_get_ge_summary_aicpu_data_when_connect_success_then_return_data(self):
        ge_task_data = [["2zx", 2, 1, 0], ]
        res = self.get_ge_task_dto(ge_task_data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData._get_ge_summary_aicpu_data', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = ParseAiCpuData()
            result = check.get_ge_summary_aicpu_data("")
            unittest.TestCase().assertEqual(result, res)

    def test_get_aicpu_data_when_is_operator_then_normal(self):
        with DBOpen('test.db') as db_open:
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                    mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                               return_value=[]), \
                    mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO), \
                    mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
                ProfilingScene().init(" ")
                check = ParseAiCpuData()
                result = check._get_aicpu_data(db_open.db_conn, ITER_RANGE, "")
                unittest.TestCase().assertEqual(result, [])

    def test_get_ge_summary_aicpu_data_when_normal_then_pass(self):
        with DBOpen('test.db') as db_open:
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
                check = ParseAiCpuData()
                result = check._get_ge_summary_aicpu_data(db_open.db_conn)
                unittest.TestCase().assertEqual(result, [])

    def test_get_ascend_task_aicpu_data_when_normal_then_pass(self):
        with DBOpen('test.db') as db_open:
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
                check = ParseAiCpuData()
                result = check._get_ascend_task_aicpu_data(db_open.db_conn)
                unittest.TestCase().assertEqual(result, [])

    def test_get_aicpu_batch_id_when_data_key_same_then_return_data_with_batch_id(self):
        aicpu_data = [
            [2, 0, 1, 2, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 3.5, 4, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 7, 8, "N/A", 0, 0, 15, 0, 300, 0],
        ]
        ascend_task_data = [[2, 0, 0, 900, 2100], [2, 1, 0, 3300, 4500], [2, 1, 1, 6300, 9000]]
        target_data = [
            [2, 0, 1, 2, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 3.5, 4, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 7, 8, "N/A", 0, 0, 15, 0, 300, 1],
        ]
        ai_cpu_results = self.get_aicpu_dto(aicpu_data)
        ascend_task_results = self.get_ascend_task_dto(ascend_task_data)
        target_result = self.get_aicpu_dto(target_data)
        check = ParseAiCpuData()
        result = check.get_aicpu_batch_id(ai_cpu_results, ascend_task_results)
        unittest.TestCase().assertEqual(len(target_result), len(result))
        for i, _ in enumerate(target_result):
            unittest.TestCase().assertEqual(target_result[i].batch_id, result[i].batch_id)

    def test_get_aicpu_batch_id_when_aicpu_data_can_not_match_ascend_task_then_return_empty_list(self):
        aicpu_data = [
            [2, 0, 5, 8, "N/A", 0, 0, 15, 0, 300, 0],
        ]
        ascend_task_data = [[2, 0, 0, 1000, 2000], [2, 0, 0, 9000, 10000]]
        target_data = [[2, 0, 5, 8, "N/A", 0, 0, 15, 0, 300, 0]]
        ai_cpu_results = self.get_aicpu_dto(aicpu_data)
        ascend_task_results = self.get_ascend_task_dto(ascend_task_data)
        target_result = self.get_aicpu_dto(target_data)

        check = ParseAiCpuData()
        result = check.get_aicpu_batch_id(ai_cpu_results, ascend_task_results)
        unittest.TestCase().assertEqual(len(target_result), len(result))
        for i, _ in enumerate(target_result):
            unittest.TestCase().assertEqual(target_result[i].batch_id, result[i].batch_id)

    def test_match_aicpu_with_ge_summary_when_aicpu_data_more_than_ge_then_return_data_name_is_NA(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        aicpu_data_with_batch_id = [
            [2, 3, 108, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 2, 112, 200, "N/A", 0, 0, 15, 0, 300, 1],
            [3, 2, 115, 200, "N/A", 0, 0, 15, 0, 300, 2],
        ]
        ge_task_data = [
            ["2zx", 2, 3, 1],
            ["3zx", 2, 2, 1],
            ["5zx", 3, 2, 2],
        ]
        target_data = [
            [2, 3, 108, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 2, 112, 200, "3zx", 0, 0, 15, 0, 300, 1],
            [3, 2, 115, 200, "5zx", 0, 0, 15, 0, 300, 2],
        ]
        ai_cpu_results = self.get_aicpu_dto(aicpu_data_with_batch_id)
        ge_task_results = self.get_ge_task_dto(ge_task_data)
        check = ParseAiCpuData()
        result = check.match_aicpu_with_ge_summary(ai_cpu_results, ge_task_results)
        unittest.TestCase().assertEqual(len(aicpu_data_with_batch_id), len(result))
        for i, _ in enumerate(target_data):
            unittest.TestCase().assertEqual(target_data[i][4], result[i][1])

    def test_sep_task_by_stream_task_when_get_data_then_return_stream(self):
        aicpu_data = [
            [2, 0, 100, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 103, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 2, 106, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 3, 108, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 0, 109, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [2, 1, 112, 200, "N/A", 0, 0, 15, 0, 300, 0],
            [3, 2, 115, 200, "N/A", 0, 0, 15, 0, 300, 0],
        ]
        ascend_task_data = [
            [2, 0, 0, 1000, 2000],
            [2, 1, 0, 2100, 3300],
            [2, 3, 0, 6000, 7100],
            [3, 0, 0, 7200, 8200],
            [2, 2, 1, 10000, 12000],
            [3, 2, 2, 15000, 20000],
        ]
        ge_task_data = [
            ["2zx", 2, 1, 0],
            ["3zx", 2, 2, 1],
            ["4zx", 3, 0, 0],
            ["5zx", 3, 2, 2],
        ]
        ai_cpu_results = self.get_aicpu_dto(aicpu_data)
        ge_task_results = self.get_ge_task_dto(ge_task_data)
        ascend_task_results = self.get_ascend_task_dto(ascend_task_data)
        check = ParseAiCpuData()
        tasks = check._sep_task_by_stream_task(ai_cpu_results)
        self.assertEqual(len(tasks), 5)

        tasks = check._sep_task_by_stream_task(ge_task_results)
        self.assertEqual(len(tasks), 4)

        tasks = check._sep_task_by_stream_task(ascend_task_results)
        self.assertEqual(len(tasks), 6)

    def test_analysis_aicpu_chip6_when_normal_then_pass(self):
        project_path = 'home\\project'
        with mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_data'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ge_summary_aicpu_data'), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.match_aicpu_with_ge_summary', return_value=[]):
            result = ParseAiCpuData.analysis_aicpu_by_sqe_id(project_path, ITER_RANGE)
            unittest.TestCase().assertEqual(result, [])

    def test_get_ge_summary_aicpu_data_when_use_sqe_id_true_then_return_data(self):
        ge_task_data = [["2zx", 2, 1, 0], ]
        res = self.get_ge_task_dto(ge_task_data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData._get_ge_summary_data', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = ParseAiCpuData()
            result = check.get_ge_summary_aicpu_data("", use_sqe_id=True)
            unittest.TestCase().assertEqual(result, res)


if __name__ == '__main__':
    unittest.main()
