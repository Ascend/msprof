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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from msmodel.hardware.hccs_model import HccsModel
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingHCCSData(MsMultiProcess):
    """
    parsing HCCS data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.HCCS, [])
        self.project_path = sample_config.get("result_dir", "")
        self._model = HccsModel(
            self.project_path,
            DBNameConstant.DB_HCCS,
            [DBNameConstant.TABLE_HCCS_ORIGIN, DBNameConstant.TABLE_HCCS_EVENTS],
        )
        self.device_id = self.sample_config.get("device_id", "0")
        self.origin_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str) -> None:
        """
        parsing hccs data and insert into hccs.db
        """
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        try:
            with FileOpen(file_path) as _file:
                _file.file_reader.readline(Constant.MAX_READ_LINE_BYTES)  # Skip the file header.
                self._read_binary_helper(_file.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, self.project_path):
                    logging.info("start parsing HCCS data file: %s", file_name)
                    self.read_binary_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
                    logging.info("Create HCCS DB finished!")
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.origin_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.origin_data)
            self._model.insert_metrics(self.device_id)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        Entrance of HCCS parser.
        :return:None
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as hccs_err:
            logging.error(str(hccs_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _read_binary_helper(self: any, _file: any) -> None:
        while True:
            one_slice = _file.readline(Constant.MAX_READ_LINE_BYTES)
            if one_slice:
                timestamp, tx_count, rx_count = one_slice.split()
                timestamp = timestamp.replace(':', '')
                self.origin_data.append((self.device_id, timestamp, tx_count, rx_count))
            else:
                break
