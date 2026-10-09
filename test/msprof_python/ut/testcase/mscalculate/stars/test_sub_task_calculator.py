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


import os
import unittest
from unittest import mock

import pytest

from common_func.info_conf_reader import InfoConfReader
from constant.constant import clear_dt_project
from mscalculate.stars.sub_task_calculator import SubTaskCalculator
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.stars.sub_task_calculator'
MODELNAMESPACE = 'msmodel.stars.ffts_log_model'


class StarsCommon:
    stream_id = 1
    task_id = 2
    timestamp = 3


class DataConstant:
    stars_common = StarsCommon
    subtask_id = 4
    thread_id = 5
    subtask_type = 6
    ffts_type = 0
    func_type = 7


def get_ffts_log_dto(ffts_log_data: list) -> list:
    ffts_log_dto = []
    for data in ffts_log_data:
        data_dto = TaskTimeDto()
        data_dto.stream_id = data[0]
        data_dto.task_id = data[1]
        data_dto.subtask_id = data[2]
        data_dto.thread_id = data[3]
        data_dto.subtask_type = data[4]
        data_dto.ffts_type = data[5]
        data_dto.task_type = data[6]
        data_dto.task_time = data[7]
        ffts_log_dto.append(data_dto)
    return ffts_log_dto


class TestSubTaskCalculator(unittest.TestCase):
    sample_config = {"result_dir": "test"}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_DeviceTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    PROF_HOST_DIR = os.path.join(PROF_DIR, 'host')

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        path = os.path.join(self.PROF_DEVICE_DIR, 'sqlite')
        os.makedirs(path)
        path = os.path.join(self.PROF_HOST_DIR, 'sqlite')
        os.makedirs(path)
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5"}

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)
        info_reader = InfoConfReader()
        info_reader._info_json = {}

    def test_insert_log_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.insert_data_to_db'):
            check = FftsLogModel('test', 'test', ['test'])
            check.flush([DataConstant])

    def test_ms_run(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.SubTaskCalculator.calculate'), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save'):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.ms_run()
            check = SubTaskCalculator({}, self.sample_config)
            check.ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.calculate = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()


    def test_get_subtask_time_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.get_all_data', return_value=1):
            check = FftsLogModel('test', 'test', ['test'])
            ret = check._get_subtask_time_data()
        self.assertEqual(ret, 1)

    def test_calculate(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save', return_value=[]):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.calculate()
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save', side_effect=ValueError):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.calculate()

    def test_init(self: any) -> None:
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
            with pytest.raises(ValueError) as err:
                check = SubTaskCalculator(file_list, self.sample_config)
                check.init()

    def test_save(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            with pytest.raises(AttributeError) as err:
                check = SubTaskCalculator(file_list, self.sample_config)
                check.save()

    def test_get_thread_task_time_sql(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        sql = "Select end_log.subtask_id as subtask_id, end_log.task_id as task_id," \
              "end_log.stream_id as stream_id,end_log.subtask_type as subtask_type, " \
              "end_log.ffts_type as ffts_type,end_log.thread_id as thread_id," \
              "start_log.task_time as task_time," \
              "(end_log.task_time-start_log.task_time) as dur_time " \
              "from FftsLog end_log " \
              "join FftsLog start_log on end_log.thread_id=start_log.thread_id " \
              "and end_log.subtask_id=start_log.subtask_id and end_log.stream_id=start_log.stream_id " \
              "and end_log.task_id=start_log.task_id " \
              "where end_log.task_type='100011' and start_log.task_type='100010' " \
              "group by end_log.subtask_id,end_log.thread_id, end_log.task_id " \
              "order by end_log.subtask_id"
        check = SubTaskCalculator(file_list, self.sample_config)
        ret = check._get_thread_task_time_sql()
        self.assertEqual(ret, sql)

    def test_get_subtask_time_should_return_all_matched_result_when_fftslog_data_is_completed(self):
        ffts_log_data = [
            [0, 23, 1, 0, "AIC", "FFTS+", '100010', 38140480339123],
            [0, 24, 2, 1, "AIV", "FFTS+", '100010', 38140480339683],
            [0, 23, 1, 0, "AIC", "FFTS+", '100011', 38140480340343],
            [0, 24, 2, 1, "AIV", "FFTS+", '100011', 38140480398243],
        ]
        ffts_log_dto = get_ffts_log_dto(ffts_log_data)
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_log_data', return_value=ffts_log_dto):
            subtask_calculator = SubTaskCalculator(file_list, self.sample_config)
            result = subtask_calculator.get_subtask_time()
            self.assertEqual(len(result), 2)
            self.assertEqual(result[0][7], 1220)
            self.assertEqual(result[1][7], 58560)

    def test_get_subtask_time_should_mismatch_when_fftslog_data_is_not_completed(self):
        ffts_log_data = [
            [0, 23, 1, 0, "AIC", "FFTS+", '100010', 38140480339123],
            [0, 24, 2, 1, "AIV", "FFTS+", '100010', 38140480339683],
            [0, 23, 1, 0, "AIC", "FFTS+", '100011', 38140480339121],
            [0, 24, 2, 1, "AIV", "FFTS+", '100011', 38140480398243],
            [0, 25, 1, 0, "MIX_AIV", "FFTS+", '100010', 38140480398643],
        ]
        ffts_log_dto = get_ffts_log_dto(ffts_log_data)
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_log_data', return_value=ffts_log_dto):
            subtask_calculator = SubTaskCalculator(file_list, self.sample_config)
            result = subtask_calculator.get_subtask_time()
            self.assertEqual(len(result), 1)

    def test_get_subtask_time_should_notify_wait_mismatch_when_missing_notify_wait_end(self):
        ffts_log_data = [
            [0, 23, 1, 0, "AIC", "FFTS+", '100010', 38140480339123],
            [0, 24, 2, 1, "AIV", "FFTS+", '100010', 38140480339683],
            [0, 27, 2, 1, "Notify Wait", "FFTS+", '100010', 38140480339688],
            [0, 23, 1, 0, "AIC", "FFTS+", '100011', 38140480340343],
            [0, 24, 2, 1, "AIV", "FFTS+", '100011', 38140480398243],
        ]
        ffts_log_dto = get_ffts_log_dto(ffts_log_data)
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_log_data', return_value=ffts_log_dto):
            subtask_calculator = SubTaskCalculator(file_list, self.sample_config)
            result = subtask_calculator.get_subtask_time()
            self.assertEqual(len(result), 3)
