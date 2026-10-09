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
import re
from functools import partial

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.file_name_manager import FileNameManagerConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.file_manager import check_file_readable
from common_func.path_manager import PathManager
from msmodel.ai_cpu.data_preparation_model import DataPreparationModel
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class DataPreparationParser(DataParser, MsMultiProcess):
    """
    class used to calculate data preparation
    """

    DTAT_QUEUE_NUMBER_VALUE_START_INDEX = 1
    DTAT_QUEUE_START_TIME_INDEX = 2
    DTAT_QUEUE_END_TIME_INDEX = 3
    DATA_QUEUE_TABLE_ELEMENTS = 5
    HOST_QUEUE_TABLE_ELEMENTS = 5
    DATA_PREPARATION_TAG_TO_TABLE = {
        DataTag.DATA_QUEUE: DBNameConstant.TABLE_DATA_QUEUE,
        DataTag.HOST_QUEUE: DBNameConstant.TABLE_HOST_QUEUE,
    }
    HOST_DATASET_NOT_SINK_MODE = 0
    HOST_DATASET_SINK_MODE = 1

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._data = {}
        self._host_queue_mode = Constant.DEFAULT_INVALID_VALUE
        self._tag_to_parser = {DataTag.DATA_QUEUE: self._parse_data_queue, DataTag.HOST_QUEUE: self._parse_host_queue}

    @staticmethod
    def format_host_queue_raw_data(mode: int, data_lines: list) -> list:
        if not data_lines:
            return []
        result = [None] * len(data_lines)
        for index, line in enumerate(data_lines):
            data = [mode] * DataPreparationParser.HOST_QUEUE_TABLE_ELEMENTS
            data[:-1] = list(map(int, line.strip().split(" ")[:-1]))
            result[index] = data
        return result

    @staticmethod
    def _read_data_queue(file_path: str) -> list:
        check_file_readable(file_path)
        with FileOpen(file_path, 'rb') as file:
            line = str(
                file.file_reader.read().replace(b'\n\x00', b' ___ ').replace(b'\x00', b' ___ '), encoding='utf-8'
            )
            lines = list(filter(None, line.split(" ___ ")))
        return lines

    def parse(self: any) -> None:
        """
        parse data preparation raw data
        :return: None
        """
        try:
            for tag in DataPreparationParser.DATA_PREPARATION_TAG_TO_TABLE:
                files = self._file_list.get(tag, [])
                if not files:
                    continue
                self._parse_data(files, tag)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if not self._data:
            logging.warning("No data preparation data, data list is empty!")
            return
        with DataPreparationModel(self._project_path, list(self._data.keys())) as model:
            model.flush_all(self._data)

    def ms_run(self: any) -> None:
        """
        parse data preparation and save it to db.
        :return:None
        """
        if self._file_list:
            self.parse()
            self.save()
            logging.info("Parsing data preparation has finished.")

    def _parse_data(self: any, file_list: list, tag: DataTag) -> None:
        parser = self._tag_to_parser.get(tag)
        if not parser:
            return
        data_list = parser(file_list)
        if data_list:
            self._data.setdefault(DataPreparationParser.DATA_PREPARATION_TAG_TO_TABLE.get(tag), data_list)

    def _check_host_queue_mode(self: any, file_list: list) -> None:
        for file in file_list:
            if re.compile(FileNameManagerConstant.DATA_PREPARATION_DEVICE_QUEUE).match(file):
                self._host_queue_mode = DataPreparationParser.HOST_DATASET_SINK_MODE
                return
            if re.compile(FileNameManagerConstant.DATA_PREPARATION_DATASET_ITERATION).match(file):
                self._host_queue_mode = DataPreparationParser.HOST_DATASET_NOT_SINK_MODE
                return

    def _parse_host_queue(self: any, file_list: list) -> list:
        file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._check_host_queue_mode(file_list)
        return self.parse_plaintext_data(
            file_list, partial(DataPreparationParser.format_host_queue_raw_data, self._host_queue_mode)
        )

    def _parse_data_queue(self: any, file_list: list) -> list:
        data_list = []
        for _file in file_list:
            file_path = PathManager.get_data_file_path(self._project_path, _file)
            data_list.extend(self._parse_data_queue_file(file_path))
        return data_list

    def _parse_data_queue_file(self: any, file_path: str) -> list:
        """
        read json data
        :param file_path:
        :return:
        """
        data_list = self._read_data_queue(file_path)
        if not data_list:
            return []
        result_list = [None] * len(data_list)
        try:
            for index, data in enumerate(data_list):
                res_data = [0] * self.DATA_QUEUE_TABLE_ELEMENTS
                # split and pick each value after the colon
                res_data[:-1] = re.split('[:,]', data)[1::2]
                res_data[self.DTAT_QUEUE_NUMBER_VALUE_START_INDEX :] = list(
                    map(int, res_data[self.DTAT_QUEUE_NUMBER_VALUE_START_INDEX :])
                )
                res_data[-1] = res_data[self.DTAT_QUEUE_END_TIME_INDEX] - res_data[self.DTAT_QUEUE_START_TIME_INDEX]
                result_list[index] = res_data
        except (ValueError, IndexError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        return result_list
