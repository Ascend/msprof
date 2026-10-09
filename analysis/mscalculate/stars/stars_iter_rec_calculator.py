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

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from common_func.file_manager import FileOpen
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class StarsIterRecCalculator(IParser, MsMultiProcess):
    """
    通过二分搜索计算stars hwts_rec
    与原有模式的差异：
        1. 原有模式遍历整个task或者pmu文件，统计每个迭代的cnt和offset；
            该类通过需要统计的一个或多个step的开始和结束syscnt时间，二分搜索获取该时间范围内的cnt和offset，只记录一条数据
        2. 原有模式不需要重复统计，二分搜索由于一次只统计一条需要的数据，每次跑都需要重新计算，所以从parser模块移到了calculator模块
    二分搜索计算step时间段内的数据量cnt，以及之前的数据量offset：
        1. 获取step开始时间和结束时间（可以包含多个step）
        2. 根据step开始时间，二分搜索小于该时间的数据量，记为offset
        3. 根据step结束时间，二分搜索小于等于该时间的数据量，再减去2中计算的offset，计算出step时间段内的所有数据量cnt
    """

    PMU_LOG_SIZE = 128
    STARS_LOG_SIZE = 64

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self._pmu_offset = 0
        self._pmu_cnt = 0
        self._task_offset = 0
        self._task_cnt = 0
        self._file_list = file_list
        # 老化遗留的字节数, 比如一个完整的数据结构分布在slice0和slice1, 但slice0老化, 在slice1中遗留的该数据字节数
        self._aging_offset_bytes = 0
        # 文件中总的数据量
        self._total_data_cnt = 0
        # 文件字节数前缀和, 去除了老化遗留的字节数
        self._file_bytes_prefix_sum = []

    def ms_run(self: any) -> None:
        """
        multiprocess to parse stars task and pmu data
        :return: None
        """
        if self._file_list and not ProfilingScene().is_all_export():
            self.parse()
            self.save()

    def parse(self: any) -> None:
        """
        parse ffts profiler data
        :return:
        """
        with TsTrackModel(
            self._project_path, DBNameConstant.DB_STEP_TRACE, [ProfilingScene().get_step_table_name()]
        ) as trace:
            step_time = trace.get_step_syscnt_range(self._iter_range)
        pmu_files = self._file_list.get(DataTag.FFTS_PMU, [])
        if Utils.get_aicore_type(self.sample_config) != StrConstant.AIC_SAMPLE_BASED_MODE and pmu_files:
            self._pmu_cnt, self._pmu_offset = self.get_cnt_and_offset(pmu_files, self.PMU_LOG_SIZE, step_time)
        task_files = self._file_list.get(DataTag.STARS_LOG, [])
        if task_files:
            self._task_cnt, self._task_offset = self.get_cnt_and_offset(task_files, self.STARS_LOG_SIZE, step_time)

    def get_cnt_and_offset(self: any, files: list, struct_size: int, step_time: StepTraceDto) -> tuple:
        files.sort(key=lambda x: int(x.split("_")[-1]))
        self.compute_file_bytes(files, struct_size)
        offset = self.binary_search_before_target(step_time.step_start, files, struct_size)
        cnt = self.binary_search_before_and_contain_target(step_time.step_end, files, struct_size) - offset
        return cnt, offset

    def binary_search_before_target(self: any, target: int, files: list, struct_size: int) -> int:
        left = 0
        right = self._total_data_cnt + 1  # +1 是为了取到最后一个值
        while right - left > 1:
            mid = (left + right) // 2  # 2分: 取中间数,  由于right >= 2, 所以mid >= 1
            if target <= self.get_data_sys_cnt_from_files(mid, files, struct_size):
                right = mid
            else:
                left = mid
        return left

    def binary_search_before_and_contain_target(self: any, target: int, files: list, struct_size: int) -> int:
        left = 0
        right = self._total_data_cnt + 1  # +1 是为了取到最后一个值
        while right - left > 1:
            mid = (left + right) // 2  # 2分: 取中间数,  由于right >= 2, 所以mid >= 1
            if target < self.get_data_sys_cnt_from_files(mid, files, struct_size):
                right = mid
            else:
                left = mid
        return left

    def get_data_sys_cnt_from_files(self: any, index: int, files: list, struct_size: int) -> int:
        """
        从多个slice文件中读取第i(index)个数据, index >= 1:
            1. 考虑到老化场景，以及一个数据分别在多个slice文件中的可能性，需要先计算老化遗留的字节数和文件字节数前缀和
            2. 二分搜索文件字节数前缀和，计算第i个数据的第一个字节在哪个文件中，假设在slice_j中
            3. 计算第i个数据的第一个字节在slice_j中的偏移量offset_bytes
            4. 循环从多个连续的slice文件中读取第i个数据，直至读满struct_size字节数
        """
        offset_byte = (index - 1) * struct_size
        read_file_idx = self.binary_search_file_slice(offset_byte + 1)  # offset_byte + 1是开始的字节
        # 校正偏移量offset_byte
        if read_file_idx == 0:
            # 对于第一个文件，可能有老化遗留的字节数
            offset_byte += self._aging_offset_bytes
        else:
            # 第二个文件开始，offset_byte要减去前面所有文件的字节数，代表该文件的偏移字节数
            offset_byte -= self._file_bytes_prefix_sum[read_file_idx - 1]
        remain_read_size = struct_size
        data = bytes()
        # 一个数据可能分布在多个文件，所以要循环读取，直到读满对应字节数
        while remain_read_size > 0 and read_file_idx < len(files):
            file_path = PathManager.get_data_file_path(self._project_path, files[read_file_idx])
            with FileOpen(file_path, 'rb') as reader:
                reader.file_reader.seek(offset_byte)
                data += reader.file_reader.read(remain_read_size)
            remain_read_size = struct_size - len(data)
            read_file_idx += 1
            offset_byte = 0
        # get sys cnt
        if struct_size == self.PMU_LOG_SIZE:
            # pmu data end sys cnt is [120, 128) bytes
            return int.from_bytes(data[120:128], byteorder='little', signed=False)
        else:
            # stars data syscnt is [8, 16) bytes
            return int.from_bytes(data[8:16], byteorder='little', signed=False)

    def binary_search_file_slice(self: any, target: int) -> int:
        left = -1
        right = len(self._file_bytes_prefix_sum) - 1
        while right - left > 1:
            mid = (left + right) // 2  # 2分: 取中间数, 由于right >= 1, 所以mid >= 0
            if target <= self._file_bytes_prefix_sum[mid]:
                right = mid
            else:
                left = mid
        return right

    def compute_file_bytes(self: any, files: list, struct_size: int):
        # 如果3个slice文件大小分别是10 bytes, 30 bytes, 20 bytes, 第一个文件的老化遗留字节数是4 bytes,
        # 那么 self._file_bytes_prefix_sum = [6, 36, 56]
        all_file_size = 0
        self._file_bytes_prefix_sum = [0] * len(files)
        for i, file in enumerate(files):
            all_file_size += os.path.getsize(PathManager.get_data_file_path(self._project_path, file))
            self._file_bytes_prefix_sum[i] = all_file_size
        self._aging_offset_bytes = all_file_size % struct_size
        for i, file_bytes in enumerate(self._file_bytes_prefix_sum):
            self._file_bytes_prefix_sum[i] = file_bytes - self._aging_offset_bytes
        self._total_data_cnt = all_file_size // struct_size

    def save(self: any) -> None:
        """
        save data into database
        :return: None
        """
        iter_info = [
            [
                self._iter_range.iteration_start,
                self._iter_range.model_id,
                self._iter_range.iteration_start,
                self._task_cnt,
                self._task_offset,
                self._pmu_cnt,
                self._pmu_offset,
            ]
        ]
        with HwtsIterModel(self._project_path) as hwts_iter_model:
            hwts_iter_model.clear_table()
            hwts_iter_model.flush(iter_info, DBNameConstant.TABLE_HWTS_ITER_SYS)
