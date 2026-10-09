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

from constant.constant import clear_dt_project
from common_func.file_manager import FdOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from mscalculate.stars.stars_iter_rec_calculator import StarsIterRecCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.db_dto.step_trace_dto import IterationRange

NAMESPACE = 'mscalculate.stars.stars_iter_rec_calculator'


class TestStarsIterRecCalculator(unittest.TestCase):
    """
    测试用例覆盖如下场景：
    1. step开始时间 <= 数据开始时间 <= step结束时间 <= 数据结束时间
    2. 数据开始时间 <= step开始时间 <= step结束时间 <= 数据结束时间
    3. 数据开始时间 <= step开始时间 <= 数据结束时间 <= step结束时间
    4. step开始时间 <= 数据开始时间 <= 数据结束时间 <= step结束时间
    5. 一个数据分布在连续的多个slice文件中
    6. 老化
    7. step开始时间或者结束时间正好等于某个数据的sys_cnt时间
    8. 数据中有相同的sys_cnt时间，且step开始时间或者结束时间正好等于该时间
    """
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_StarsIterRecCalculator")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF_1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    DATA_PATH = os.path.join(PROF_DEVICE_DIR, 'data')
    FILE_LIST = {
        DataTag.FFTS_PMU: [],
        DataTag.STARS_LOG: [],
    }
    SAMPLE_CONFIG = {StrConstant.SAMPLE_CONFIG_PROJECT_PATH: DIR_PATH}

    @classmethod
    def setUpClass(cls) -> None:
        clear_dt_project(cls.DIR_PATH)
        os.makedirs(cls.DATA_PATH)
        path_sqlite = os.path.join(cls.PROF_DEVICE_DIR, 'sqlite')
        os.makedirs(path_sqlite)
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        cls.create_pmu_data()
        cls.create_task_data()

    @classmethod
    def tearDownClass(cls) -> None:
        clear_dt_project(cls.DIR_PATH)
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    @classmethod
    def create_pmu_data(cls):
        """
        pmu end syscnt:
            [0, 10, 20,..., 80] -> 9个数据老化
            [90, 256, 266,..., 346],
            [512, 522, ..., 602],
            [768, 778, ..., 858],
            [1024, 1024,..., 1024], -> 10个1024
            [1280, ..., 1370],
            [1536, ..., 1626],
            [1792, ..., 1882],
            [2048, ..., 2138],
            [2304, ..., 2394]
        """
        all_data = bytearray()
        for file_slice in range(10):  # pmu数据共10个slice, 100个数据, 总共12800字节
            for i in range(10):
                data = bytearray(StarsIterRecCalculator.PMU_LOG_SIZE)
                if file_slice == 4:
                    data[121] = file_slice  # 添加10个相同的sys cnt
                else:
                    data[120:122] = [i * 10, file_slice]
                all_data += data
        # 每个文件所占据字节数, 同一个数据会分布在多个文件中
        data_bytes = [1100, 2400, 2450, 2500, 4000, 6000, 7000, 9000, 11000, 12800]
        for file_slice in range(1, 10):  # slice_0 老化
            file_path = os.path.join(cls.DATA_PATH, 'ffts_profile.data.0.slice_{}'.format(file_slice))
            cls.FILE_LIST.get(DataTag.FFTS_PMU, []).append(file_path)
            with FdOpen(file_path, operate='wb') as fw:
                fw.write(all_data[data_bytes[file_slice - 1]:data_bytes[file_slice]])

    @classmethod
    def create_task_data(cls):
        """
        task syscnt:
            [0, 10, 20,..., 80, 90] -> 10个数据老化
            [256, 266,..., 346],
            [512, 522, ..., 602],
            [768, 778, ..., 858],
            [1024, 1032,..., 1114],
            [1280, ..., 1370],
            [1536, ..., 1626],
            [1792, ..., 1882],
            [2048, ..., 2138],
            [2304, ..., 2394]
        """
        all_data = bytearray()
        for file_slice in range(10):  # task数据共10个slice, 100个数据, 总共6400字节
            for i in range(10):
                data = bytearray(StarsIterRecCalculator.STARS_LOG_SIZE)
                data[8:10] = [i * 10, file_slice]
                all_data += data
        # 每个文件所占据字节数, 同一个数据会分布在多个文件中
        data_bytes = [600, 1400, 2000, 2040, 2050, 2060, 2070, 3000, 5000, 6400]
        for file_slice in range(1, 10):  # slice_0 老化
            file_path = os.path.join(cls.DATA_PATH, 'stars_soc.data.0.slice_{}'.format(file_slice))
            cls.FILE_LIST.get(DataTag.STARS_LOG, []).append(file_path)
            with FdOpen(file_path, operate='wb') as fw:
                fw.write(all_data[data_bytes[file_slice - 1]:data_bytes[file_slice]])

    def test_parse_should_return_3_pmu_cnt_and_2_task_cnt_when_step_start_90_and_step_end_266(self):
        step_trace = StepTraceDto()
        step_trace.step_start = 90
        step_trace.step_end = 266
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(0, iter_rec_calculator._pmu_offset)
            self.assertEqual(3, iter_rec_calculator._pmu_cnt)
            self.assertEqual(0, iter_rec_calculator._task_offset)
            self.assertEqual(2, iter_rec_calculator._task_cnt)

    def test_parse_should_return_2_pmu_offset_1_task_offset_when_step_start_266_and_step_end_601(
        self
    ):
        step_trace = StepTraceDto()
        step_trace.step_start = 266
        step_trace.step_end = 601
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(2, iter_rec_calculator._pmu_offset)
            self.assertEqual(18, iter_rec_calculator._pmu_cnt)
            self.assertEqual(1, iter_rec_calculator._task_offset)
            self.assertEqual(18, iter_rec_calculator._task_cnt)

    def test_parse_should_return_41_pmu_offset_31_task_offset_when_step_start_1025_and_step_end_2129(self):
        step_trace = StepTraceDto()
        step_trace.step_start = 1025
        step_trace.step_end = 2129
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(41, iter_rec_calculator._pmu_offset)
            self.assertEqual(39, iter_rec_calculator._pmu_cnt)
            self.assertEqual(31, iter_rec_calculator._task_offset)
            self.assertEqual(48, iter_rec_calculator._task_cnt)

    def test_parse_should_return_31_pmu_offset_30_task_offset_when_step_start_1024_and_step_end_3000(self):
        step_trace = StepTraceDto()
        step_trace.step_start = 1024
        step_trace.step_end = 3000
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(31, iter_rec_calculator._pmu_offset)
            self.assertEqual(60, iter_rec_calculator._pmu_cnt)
            self.assertEqual(30, iter_rec_calculator._task_offset)
            self.assertEqual(60, iter_rec_calculator._task_cnt)

    def test_parse_should_return_91_pmu_offset_90_task_offset_when_step_start_89_and_step_end_3000(self):
        step_trace = StepTraceDto()
        step_trace.step_start = 89
        step_trace.step_end = 3000
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(0, iter_rec_calculator._pmu_offset)
            self.assertEqual(91, iter_rec_calculator._pmu_cnt)
            self.assertEqual(0, iter_rec_calculator._task_offset)
            self.assertEqual(90, iter_rec_calculator._task_cnt)

    def test_parse_should_return_20_pmu_cnt_11_task_cnt_when_step_start_768_and_step_end_1024(self):
        step_trace = StepTraceDto()
        step_trace.step_start = 768
        step_trace.step_end = 1024
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.parse()
            self.assertEqual(21, iter_rec_calculator._pmu_offset)
            self.assertEqual(20, iter_rec_calculator._pmu_cnt)
            self.assertEqual(20, iter_rec_calculator._task_offset)
            self.assertEqual(11, iter_rec_calculator._task_cnt)

    def test_ms_run_should_parse_and_save_data(self):
        iter_range = IterationRange(NumberConstant.INVALID_MODEL_ID, 2, 5)
        self.SAMPLE_CONFIG.update(
            {StrConstant.PARAM_ITER_ID: iter_range}
        )
        step_trace = StepTraceDto()
        step_trace.step_start = 266
        step_trace.step_end = 601
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.TsTrackModel.get_step_syscnt_range', return_value=step_trace), \
                mock.patch(NAMESPACE + '.HwtsIterModel.clear_table'):
            iter_rec_calculator = StarsIterRecCalculator(self.FILE_LIST, self.SAMPLE_CONFIG)
            iter_rec_calculator.ms_run()
