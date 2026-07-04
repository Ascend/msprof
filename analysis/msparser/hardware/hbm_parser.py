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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.hbm_model import HbmModel
from msparser.data_struct_size_constant import StructFmt
from msparser.versioned_struct_parser import VersionedStructParser
from msparser.versioned_struct_parser import VersionedStructSpec
from profiling_bean.prof_enum.data_tag import DataTag


def decode_hbm_v1(record: tuple) -> tuple:
    timestamp, count, event_id, hbm_id = record
    timestamp = InfoConfReader().get_absolute_time_by_sampling_timestamp(timestamp)
    event_type = 'read' if event_id == 0 else 'write'
    return timestamp, count, event_type, hbm_id


def decode_hbm_v2(record: tuple) -> tuple:
    _, _, event_id, hbm_id, count, timestamp = record
    event_type = 'read' if event_id == 0 else 'write'
    return timestamp, count, event_type, hbm_id


HBM_STRUCT_SPECS = {
    "1.0": VersionedStructSpec(StructFmt.HBM_FMT, StructFmt.HBM_FMT_SIZE, decode_hbm_v1),
    "2.0": VersionedStructSpec(StructFmt.HBM_FMT_V2, StructFmt.HBM_FMT_SIZE_V2, decode_hbm_v2),
}


class ParsingHBMData(MsMultiProcess):
    """
    parsing DDR data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.hbm_data = []
        self.sample_config = sample_config
        self._file_list = file_list.get(DataTag.HBM, [])
        self.project_path = sample_config.get("result_dir", "")
        self._struct_parser = VersionedStructParser("HBM", HBM_STRUCT_SPECS)
        self.calculate = OffsetCalculator(self._file_list, self._struct_parser.spec.size, self.project_path)
        self._model = HbmModel(
            self.project_path, DBNameConstant.DB_HBM, [DBNameConstant.TABLE_HBM_ORIGIN, DBNameConstant.TABLE_HBM_BW]
        )
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str, device_id: str, replay_id: str) -> int:
        """
        parsing hbm data and insert into hbm.db
        """
        status = NumberConstant.ERROR
        hbm_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(hbm_file):
            return status
        _file_size = os.path.getsize(hbm_file)
        try:
            with FileOpen(hbm_file, "rb") as hbm_f:
                hbm_data = self.calculate.pre_process(hbm_f.file_reader, _file_size)
                for item in self._struct_parser.iter_decoded_records(hbm_data):
                    self.hbm_data.append(tuple([device_id, replay_id] + list(item)))
            return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return status

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, self.project_path):
                    self._original_data_handler(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.hbm_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.hbm_data)
            self._model.insert_bw_data(self.sample_config.get("hbm_profiling_events", "").split(","))
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as hbm_err:
            logging.error(str(hbm_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _original_data_handler(self: any, file_name: str) -> None:
        device_id = self.sample_config.get("device_id", "0")
        logging.info("start parsing HBM data file: %s", file_name)
        status = self.read_binary_data(file_name, device_id, '0')  # replay is is 0
        FileManager.add_complete_file(self.project_path, file_name)
        if status:
            logging.error('Insert HBM bandwidth data error.')
        logging.info("Create HBM DB finished!")
