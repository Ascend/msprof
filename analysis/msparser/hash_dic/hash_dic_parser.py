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
from common_func.file_manager import FileOpen
from common_func.ms_constant.level_type_constant import LevelDataType
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from msmodel.ge.ge_hash_model import GeHashModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class HashDicParser(IParser, MsMultiProcess):
    """
    hash data parser
    """

    COLON = ":"
    UNDERLINE = "_"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = GeHashModel(self._project_path, [DBNameConstant.TABLE_GE_HASH, DBNameConstant.TABLE_TYPE_HASH])
        self._hash_data = {'ge_hash': [], 'type_hash': []}

    def parse(self: any) -> None:
        """
        parse function
        """
        hash_files = self._file_list.get(DataTag.HASH_DICT, [])
        for _file in hash_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)
            logging.info("start parsing hash data file: %s", _file)
            if _file.split(".")[2] == 'hash_dic':
                self._read_ge_hash_data(_file_path)
            if _file.split(".")[2] == 'type_info_dic':
                self._read_type_hash_data(_file_path)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if not any(self._hash_data.values()):
            return
        with self._model as _model:
            _model.insert_data_to_db(DBNameConstant.TABLE_GE_HASH, self._hash_data.get('ge_hash', []))
            _model.insert_data_to_db(DBNameConstant.TABLE_TYPE_HASH, self._hash_data.get('type_hash', []))

    def ms_run(self: any) -> None:
        """
        entrance for hash parser
        :return:
        """
        if not self._file_list.get(DataTag.HASH_DICT, []):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _read_ge_hash_data(self: any, file_path: str) -> None:
        with FileOpen(file_path, "r") as _file:
            data_lines = _file.file_reader.readlines(Constant.MAX_READ_FILE_BYTES)
        for line in data_lines:
            if self.COLON not in line:
                logging.warning("GE hash data is invalid: %s", line)
                continue
            key, value = line.strip().split(self.COLON, 1)
            if key.isdigit():
                self._hash_data['ge_hash'].append([key, value])

    def _read_type_hash_data(self: any, file_path: str) -> None:
        with FileOpen(file_path, "r") as _file:
            data_lines = _file.file_reader.readlines(Constant.MAX_READ_FILE_BYTES)
        for line in data_lines:
            if self.COLON not in line or self.UNDERLINE not in line:
                logging.warning("Type hash data is invalid: %s", line)
                continue
            key, value = line.strip().split(self.COLON, 1)
            level, key = key.split(self.UNDERLINE)
            if key.isdigit() and level.isdigit() and int(level) in LevelDataType.member_map():
                self._hash_data['type_hash'].append([key, value, LevelDataType(int(level)).name.lower()])
