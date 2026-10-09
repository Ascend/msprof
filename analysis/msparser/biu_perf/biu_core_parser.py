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

import logging
import os
from abc import abstractmethod

from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.file_manager import FileOpen
from framework.offset_calculator import OffsetCalculator
from profiling_bean.biu_perf.core_info_bean import CoreInfo
from profiling_bean.biu_perf.cycles_bean import CyclesBean
from profiling_bean.biu_perf.flow_bean import FlowBean


class BiuCoreParser:
    """
    The base parser of BiuCubeParser and BiuVectorParser
    """

    # byte
    CUBE_SIZE = 96
    VECTOR_SIZE = 64

    # the location of flow and cycles
    CUBE_CYCLES_INDEX_RANGE = [[40, 48], [56, 64], [72, 80], [88, 96]]
    CUBE_FLOW_INDEX_RANGE = [[32, 40], [48, 56], [64, 72], [80, 88]]

    VECTOR_CYCLES_INDEX_RANGE = [32, 64]

    MONITOR_LENGTH = 32

    FLOW_MAX_CYCLES = 4096

    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        self._file_list = core_info.file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.core_info = core_info
        self.cycles_data = []
        self.flow_data = []
        self.last_flow_datum = FlowBean.decode(b'\x00' * self.MONITOR_LENGTH)
        self.last_cycles_datum = CyclesBean.decode(b'\x00' * self.MONITOR_LENGTH)

    def process_file(self: any) -> None:
        """
        process biu perf file
        return: None
        """
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        for file_name in self._file_list:
            file_path = PathManager.get_data_file_path(self._project_path, file_name)
            self.parse_binary_file(file_path)

    @abstractmethod
    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """

    def add_cycles_data(self: any, cycles_bean: any) -> None:
        """
        get list from monitor cycles bean
        return: None
        """

        if self.core_info.core_type == CoreInfo.AI_CUBE:
            delta_vector_cycles = 0
            delta_cube_cycles = self.calculate_delta_cycles(cycles_bean.cube_cycles, self.last_cycles_datum.cube_cycles)
        else:
            delta_vector_cycles = self.calculate_delta_cycles(
                cycles_bean.vector_cycles, self.last_cycles_datum.vector_cycles
            )
            delta_cube_cycles = 0

        self.cycles_data.append(
            [
                delta_vector_cycles,
                self.calculate_delta_cycles(cycles_bean.scalar_cycles, self.last_cycles_datum.scalar_cycles),
                delta_cube_cycles,
                self.calculate_delta_cycles(cycles_bean.lsu1_cycles, self.last_cycles_datum.lsu1_cycles),
                self.calculate_delta_cycles(cycles_bean.lsu2_cycles, self.last_cycles_datum.lsu2_cycles),
                self.calculate_delta_cycles(cycles_bean.lsu3_cycles, self.last_cycles_datum.lsu3_cycles),
                cycles_bean.timestamp,
                self.core_info.core_id,
                self.core_info.group_id,
                self.core_info.core_type,
            ]
        )
        self.last_cycles_datum = cycles_bean

    def calculate_delta_cycles(self: any, current_value: int, last_value: int) -> int:
        # wrap_around
        if current_value < last_value:
            return current_value + self.FLOW_MAX_CYCLES - last_value
        return current_value - last_value

    def add_flow_data(self: any, flow_bean: any) -> None:
        """
        get list from monitor flow bean
        return: None
        """
        self.flow_data.append(
            [
                flow_bean.stat_rcmd_num,
                flow_bean.stat_wcmd_num,
                flow_bean.stat_rlat_raw,
                flow_bean.stat_wlat_raw,
                flow_bean.stat_flux_rd,
                flow_bean.stat_flux_wr,
                flow_bean.stat_flux_rd_l2,
                flow_bean.stat_flux_wr_l2,
                flow_bean.timestamp,
                flow_bean.l2_cache_hit,
                self.core_info.core_id,
                self.core_info.group_id,
                self.core_info.core_type,
            ]
        )


class BiuCubeParser(BiuCoreParser):
    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        super().__init__(sample_config, core_info)
        self._offset_calculator = OffsetCalculator(self._file_list, self.CUBE_SIZE, self._project_path)

    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """
        with FileOpen(file_path, 'rb') as file_reader:
            all_bytes = self._offset_calculator.pre_process(file_reader.file_reader, os.path.getsize(file_path))

        for chunk in Utils.chunks(all_bytes, self.CUBE_SIZE):
            cycles_chunk, flow_chunk = self.split_monitor_data(chunk)
            cycles_bean = CyclesBean.decode(cycles_chunk)
            flow_bean = FlowBean.decode(flow_chunk)

            # timestamp of ai cube monitor 1 is invalid, so it is replaced by monitor 0
            cycles_bean.timestamp = flow_bean.timestamp
            self.add_cycles_data(cycles_bean)
            self.add_flow_data(flow_bean)

    def split_monitor_data(self: any, chunk: any) -> tuple:
        """
        split monitor data
        params: chunk
        return: tuple of monitor_cycles_chunk and monitor0_chunk
        """
        if len(chunk) != self.CUBE_SIZE:
            logging.error("The length of AI cube core chunk is not equal to %d", self.CUBE_SIZE)
            return [], []

        flow_chunk = bytes()
        for start_index, end_index in self.CUBE_CYCLES_INDEX_RANGE:
            flow_chunk += chunk[start_index:end_index]

        biucycles_chunk = bytes()
        for start_index, end_index in self.CUBE_FLOW_INDEX_RANGE:
            biucycles_chunk += chunk[start_index:end_index]

        return flow_chunk, biucycles_chunk

    def get_monitor_data(self: any) -> tuple:
        """
        get monitor data
        return: tuple of monitor_cycles_data and monitor0_data
        """
        self.process_file()
        return self.cycles_data, self.flow_data


class BiuVectorParser(BiuCoreParser):
    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        super().__init__(sample_config, core_info)
        self._offset_calculator = OffsetCalculator(self._file_list, self.VECTOR_SIZE, self._project_path)

    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """
        with FileOpen(file_path, 'rb') as file_reader:
            all_bytes = self._offset_calculator.pre_process(file_reader.file_reader, os.path.getsize(file_path))

        for chunk in Utils.chunks(all_bytes, self.VECTOR_SIZE):
            cycles_chunk = chunk[self.VECTOR_CYCLES_INDEX_RANGE[0] : self.VECTOR_CYCLES_INDEX_RANGE[1]]
            cycles_bean = CyclesBean.decode(cycles_chunk)
            self.add_cycles_data(cycles_bean)

    def get_monitor_data(self: any) -> list:
        """
        get monitor data
        """
        self.process_file()
        return self.cycles_data
