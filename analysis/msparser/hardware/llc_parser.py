# pylint: disable=duplicate-code
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

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.llc_model import LlcModel
from msparser.data_struct_size_constant import StructFmt
from msparser.versioned_struct_parser import VersionedStructParser
from msparser.versioned_struct_parser import VersionedStructSpec
from profiling_bean.prof_enum.data_tag import DataTag


def decode_llc_v1(record: tuple) -> tuple:
    timestamp, count, event_id, l3t_id = record
    timestamp = InfoConfReader().get_absolute_time_by_sampling_timestamp(timestamp)
    return timestamp, count, event_id, l3t_id


def decode_llc_v2(record: tuple) -> tuple:
    _, count, event_id, l3t_id, timestamp = record
    return timestamp, count, event_id, l3t_id


LLC_STRUCT_SPECS = {
    "1.0": VersionedStructSpec(StructFmt.LLC_FMT, StructFmt.LLC_FMT_SIZE, decode_llc_v1),
    "2.0": VersionedStructSpec(StructFmt.LLC_FMT_V2, StructFmt.LLC_FMT_SIZE_V2, decode_llc_v2),
}


class NonMiniLLCParser(MsMultiProcess):
    """
    parsing LLC data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.device_id = self.sample_config.get("device_id", "0")
        self._file_list = file_list.get(DataTag.LLC, [])
        self.project_path = self.sample_config.get("result_dir", "")
        self._model = LlcModel(
            self.project_path,
            DBNameConstant.DB_LLC,
            [DBNameConstant.TABLE_LLC_ORIGIN, DBNameConstant.TABLE_LLC_EVENTS, DBNameConstant.TABLE_LLC_METRICS],
        )
        self._struct_parser = VersionedStructParser("LLC", LLC_STRUCT_SPECS)
        self.calculate = OffsetCalculator(self._file_list, self._struct_parser.spec.size, self.project_path)
        self.origin_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str) -> None:
        """
        parsing llc data and insert into llc.db
        :param file_name:
        :return: None
        """
        _file_path = PathManager.get_data_file_path(self.project_path, file_name)
        _file_size = os.path.getsize(_file_path)
        try:
            with FileOpen(_file_path, "rb") as llc_file:
                self._read_binary_helper(llc_file.file_reader, _file_size)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err)
        finally:
            pass

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in self._file_list:
                self._original_data_handler(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def save(self: any) -> None:
        """
        save llc data to db
        :return: None
        """
        if self.origin_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.create_events_trigger(self.sample_config.get("llc_profiling", ""))
            self._model.flush(self.origin_data)
            self._model.insert_metrics_data()
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        Entry for LLC binaries parse.
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as llc_err:
            logging.error(str(llc_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _read_binary_helper(self: any, llc_file: any, _file_size: int) -> None:
        llc_data = self.calculate.pre_process(llc_file, _file_size)
        for timestamp, count, event_id, l3t_id in self._struct_parser.iter_decoded_records(llc_data):
            self.origin_data.append((self.device_id, timestamp, count, event_id, l3t_id))

    def _original_data_handler(self: any, file_name: str) -> None:
        if is_valid_original_data(file_name, self.project_path):
            logging.info("start parsing llc data file: %s", file_name)
            self.read_binary_data(file_name)
            if not ChipManager().is_chip_v3():
                FileManager.add_complete_file(self.project_path, file_name)
            logging.info("Create LLC DB finished!")
