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

# pylint: disable=duplicate-code

import logging
import os
import struct

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.tscpu_model import TscpuModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.hardware.mdc_tscpu import MdcTscpuDecoder
from profiling_bean.hardware.tscpu import TscpuDecoder
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingTSData(MsMultiProcess):
    """
    class for parsing task schedule data
    """

    BYTE_ORDER_CHAR = '='
    HEADER_NUMBER = 2880154539
    HEADER_SIZE = 4
    PERF_PMU_DATA_SIZE = 7
    MDC_PERF_PMU_DATA_SIZE = 5
    PMU_START = int('0x0', 16)
    PMU_END = int('0xec', 16)
    INT_MAX = 2**65 - 1
    MDC_DATA_LENGTH = 180

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.project_path = sample_config.get("result_dir")
        self._file_list = file_list.get(DataTag.TSCPU, [])
        self._model = TscpuModel(self.project_path, DBNameConstant.DB_NAME_TSCPU, [DBNameConstant.TABLE_TSCPU_ORIGIN])
        self.calculate = OffsetCalculator(self._file_list, StructFmt.TSCPU_FMT_SIZE, self.project_path)
        self.ts_data = []
        self.replayid = '0'
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @staticmethod
    def _generate_ts_info(decoder: any) -> dict:
        ts_info = {
            "perf_backtrace": decoder.perf_backtrace,
            "pc": decoder.pc,
            "timestamp": decoder.timestamp,
            "pmu_data": decoder.pmu_data,
        }
        ts_info["perf_backtrace"] = Utils.generator_to_list(i for i in ts_info.get("perf_backtrace", []) if i != 0)
        lp_ = Utils.generator_to_list(hex(i) for i in ts_info.get("perf_backtrace", [])[1::2])
        lp_ = Utils.generator_to_list(str(i) for i in lp_)
        fr_ = Utils.generator_to_list(str(hex(i)) for i in ts_info.get("perf_backtrace", [])[0::2])
        ts_info["callstack"] = ' <- '.join(Utils.generator_to_list(','.join(x) for x in zip(lp_, fr_)))
        ts_info['pmu_event_type'] = ts_info.get("pmu_data", [])[0::2]
        ts_info["pc"] = str(hex(ts_info.get("pc", "")))
        ts_info["func_name"] = str(ts_info.get("pc", ""))
        return ts_info

    @staticmethod
    def _generate_ts_info_in_mdc(decoder: any) -> dict:
        ts_info = {
            "perf_backtrace": decoder.perf_backtrace,
            "pc": decoder.pc,
            "timestamp": decoder.timestamp,
            "pmu_data": decoder.pmu_data,
        }
        ts_info["perf_backtrace"] = Utils.generator_to_list(i for i in ts_info.get("perf_backtrace", []) if i != 0)
        ts_info["perf_backtrace"] = Utils.generator_to_list(i for i in ts_info.get("perf_backtrace", []) if i != 0)
        perf_backtrace = Utils.generator_to_list(hex(i) for i in ts_info.get("perf_backtrace", [])[1::2])
        lp_ = Utils.generator_to_list(str(i) for i in perf_backtrace)
        fr_ = Utils.generator_to_list(str(hex(i)) for i in ts_info.get("perf_backtrace", [])[0::2])
        ts_info["callstack"] = ' <- '.join(Utils.generator_to_list(','.join(x) for x in zip(lp_, fr_)))
        ts_info['pmu_event_type'] = ts_info.get("pmu_data", [])[0::2]

        ts_info["pc"] = str(hex(ts_info.get("pc", '')))
        ts_info["func_name"] = str(ts_info.get("pc", ''))
        return ts_info

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data file
        """
        file_path = PathManager.get_data_file_path(self.project_path, binary_data_path)
        if not os.path.exists(file_path):
            return
        _file_size = os.path.getsize(file_path)
        try:
            with FileOpen(file_path, "rb") as file_:
                self._do_read_binary_data(file_.file_reader, _file_size)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def read_mdc_binary_data(self: any, binary_data_path: str) -> None:
        """
        read mdc binary data file
        """
        file_path = PathManager.get_data_file_path(self.project_path, binary_data_path)
        if not os.path.exists(file_path):
            return
        _file_size = os.path.getsize(file_path)
        try:
            with FileOpen(file_path, "rb") as file_:
                self._do_read_mdc_binary_data(file_.file_reader, _file_size)

        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def start_parsing_data_file(self: any) -> None:
        """
        start parsing data file
        """
        try:
            for file_name in self._file_list:
                self._do_parse_tscpu(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save
        """
        if self.ts_data and self._model:
            self._model.init()
            self._model.flush(self.ts_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        run function
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as tscpu_err:
            logging.error(str(tscpu_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _open_mdc_binary_data(self: any, binary_data_path: str) -> any:
        binary_data_file = PathManager.get_data_file_path(self.project_path, binary_data_path)
        try:
            with FileOpen(binary_data_file, "rb") as file_:
                return self._is_mdc_binary_data(file_.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
            return False

    def _is_mdc_binary_data(self: any, binary_file: any) -> bool:
        """
        check the mdc binary data
        """
        while True:
            header = binary_file.read(self.HEADER_SIZE)
            if not header:
                break
            # L is long type, 4 bytes
            if struct.unpack(self.BYTE_ORDER_CHAR + 'L', header)[0] != self.HEADER_NUMBER:
                return False
            binary_file.read(self.MDC_DATA_LENGTH)
        return True

    def _insert_ts_data_in_mdc(self: any, decoder: any) -> None:
        if decoder.header == self.HEADER_NUMBER:
            ts_info = self._generate_ts_info_in_mdc(decoder)
            pmu_event_count = ts_info.get("pmu_data", [])[1::2]
            flag = NumberConstant.SUCCESS
            for index in pmu_event_count:
                if index > self.INT_MAX:
                    logging.error('Invalid pmu event count: %s', index)
                    flag = NumberConstant.ERROR
            for pmu_vent in ts_info.get('pmu_event_type', []):
                if not self.PMU_START <= pmu_vent <= self.PMU_END:
                    logging.error('Invalid pmu event: %s', pmu_vent)
                    flag = NumberConstant.ERROR
            if flag == NumberConstant.ERROR:
                return
            ts_info['pmu_event_type'] = Utils.generator_to_list(hex(i) for i in ts_info.get('pmu_event_type', []))
            for i in range(self.MDC_PERF_PMU_DATA_SIZE):
                self.ts_data.append(
                    (
                        self.replayid,
                        ts_info.get("timestamp", 0),
                        ts_info.get("pc", "").replace('L', ''),
                        ts_info.get("callstack", ""),
                        ts_info.get('pmu_event_type', [])[i],
                        pmu_event_count[i],
                        ts_info.get("func_name", ""),
                    )
                )

    def _do_parse_tscpu(self: any, file_name: str) -> None:
        if is_valid_original_data(file_name, self.project_path):
            logging.info("start parsing tscpu data file: %s", file_name)
            if self._open_mdc_binary_data(file_name):
                self.calculate = OffsetCalculator(self._file_list, StructFmt.MDC_TSCPU_FMT_SIZE, self.project_path)
                self.read_mdc_binary_data(file_name)
            else:
                self.read_binary_data(file_name)
            FileManager.add_complete_file(self.project_path, file_name)
            logging.info("Create TsCPU DB finished!")

    def _insert_ts_data(self: any, decoder: any) -> None:
        if decoder.header == self.HEADER_NUMBER:
            ts_info = self._generate_ts_info(decoder)
            flag = NumberConstant.SUCCESS
            for i in ts_info.get('pmu_event_type', []):
                if not self.PMU_START <= i <= self.PMU_END:
                    logging.error('Invalid pmu event: %s', i)
                    flag = NumberConstant.ERROR
            pmu_event_count = ts_info.get("pmu_data", [])[1::2]
            for i in pmu_event_count:
                if i > self.INT_MAX:
                    logging.error('Invalid pmu event count: %s', i)
                    flag = NumberConstant.ERROR
            if flag == NumberConstant.ERROR:
                return
            ts_info['pmu_event_type'] = Utils.generator_to_list(hex(i) for i in ts_info.get('pmu_event_type', []))
            self.ts_data.extend(
                Utils.generator_to_list(
                    (
                        self.replayid,
                        ts_info.get("timestamp", 0),
                        ts_info.get("pc", "").replace('L', ''),
                        ts_info.get("callstack", ""),
                        ts_info.get('pmu_event_type', [])[i],
                        pmu_event_count[i],
                        ts_info.get("func_name", ""),
                    )
                    for i in range(self.PERF_PMU_DATA_SIZE)
                )
            )

    def _do_read_binary_data(self: any, file: any, file_size: int) -> None:
        tscpu_data = self.calculate.pre_process(file, file_size)
        for _index in range(file_size // StructFmt.TSCPU_FMT_SIZE):
            if tscpu_data[_index * StructFmt.TSCPU_FMT_SIZE : _index * StructFmt.TSCPU_FMT_SIZE + 4]:
                decoder = TscpuDecoder.decode(
                    tscpu_data[_index * StructFmt.TSCPU_FMT_SIZE : (_index + 1) * StructFmt.TSCPU_FMT_SIZE]
                )
                self._insert_ts_data(decoder)
            else:
                break

    def _do_read_mdc_binary_data(self: any, file: any, file_size: int) -> None:
        tscpu_data = self.calculate.pre_process(file, file_size)
        for _index in range(file_size // StructFmt.MDC_TSCPU_FMT_SIZE):
            if tscpu_data[_index * StructFmt.MDC_TSCPU_FMT_SIZE : _index * StructFmt.MDC_TSCPU_FMT_SIZE + 4]:
                decoder = MdcTscpuDecoder.decode(
                    tscpu_data[_index * StructFmt.MDC_TSCPU_FMT_SIZE : (_index + 1) * StructFmt.MDC_TSCPU_FMT_SIZE]
                )
                self._insert_ts_data_in_mdc(decoder)
            else:
                break
