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
import struct
from abc import abstractmethod

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.level_type_constant import LevelDataType
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_valid_original_data
from common_func.file_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msparser.interface.iparser import IParser


class DataParser(IParser):
    """
    stars base model class. Used to operate db.
    """

    def __init__(self: any, sample_config: dict) -> None:
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        :param bean_class:
        :param bean_data:
        :return:
        """
        return bean_class.decode(bean_data)

    @staticmethod
    def group_aging_file(api_file_list: list) -> dict:
        file_dict = {}
        for file in api_file_list:
            if file.startswith('aging'):
                file_dict.setdefault('aging_file', []).append(file)
            elif file.startswith('unaging'):
                file_dict.setdefault('unaging_file', []).append(file)
            else:
                logging.warning('Invalid file name: %s', format(file))
        for data_list in file_dict.values():
            data_list.sort(key=lambda x: int(x.split("_")[-1]))
        return file_dict

    @staticmethod
    def check_magic_num(data: bytes):
        magic_num, level = struct.unpack("=HH", data[:4])
        if magic_num != NumberConstant.MAGIC_NUM or not LevelDataType(level):
            logging.error("'5A5A' check Failed when parsing data.")
            raise ValueError("An exception occurred when parsing the format data. Some data may be lost.")

    @abstractmethod
    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """

    @abstractmethod
    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """

    def parse_plaintext_data(self: any, file_list: list, format_func: any) -> list:
        """
        parse plaintext data
        :return:
        """
        result_data = []
        for file_name in file_list:
            file_path = self.get_file_path_and_check(file_name)
            logging.info("start to process file: %s", file_name)
            with FileOpen(file_path, "r") as hash_dict:
                data_lines = hash_dict.file_reader.readlines(Constant.MAX_READ_FILE_BYTES)
                result_data.extend(format_func(data_lines))
        return result_data

    def parse_bean_data(self: any, file_list: list, format_size: int, bean_class: any, **kwargs: any) -> list:
        """
        parse bean file
        :param file_list:
        :param format_size:
        :param bean_class:
        :param format_func:
        :return:
        """
        format_func = kwargs.get('format_func', lambda x: x)
        check_func = kwargs.get('check_func', lambda x: None)
        result_data = []
        _offset_calculator = OffsetCalculator(file_list, format_size, self._project_path)
        for file_name in file_list:
            if not is_valid_original_data(file_name, self._project_path):
                continue
            FileManager.add_complete_file(self._project_path, file_name)
            file_path = self.get_file_path_and_check(file_name)
            logging.info("start to process the file: %s", file_name)
            with FileOpen(file_path, "rb") as file:
                all_log_bytes = _offset_calculator.pre_process(file.file_reader, os.path.getsize(file_path))
                for bean_data in Utils.chunks(all_log_bytes, format_size):
                    check_func(bean_data)
                    _fusion_bean_data = self.read_binary_data(bean_class, bean_data)
                    result_data.append(format_func(_fusion_bean_data))
        return result_data

    def get_file_path_and_check(self: any, file_name: str) -> str:
        """
        get file path and check it
        """
        file_path = PathManager.get_data_file_path(self._project_path, file_name)
        check_file_readable(file_path)
        return file_path
