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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from msmodel.l2_cache.l2_cache_parser_model import L2CacheParserModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.l2_cache import L2CacheDataBean


class L2CacheParser(IParser, MsMultiProcess):
    """
    l2 cache data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._model = L2CacheParserModel(self._project_path, [DBNameConstant.TABLE_L2CACHE_PARSE])
        self._l2_cache_events = []
        for event in self.sample_config.get("l2CacheTaskProfilingEvents", "").split(","):
            self._l2_cache_events.append(event.strip().lower())
        self._l2_cache_data = []

    @staticmethod
    def _check_file_complete(file_path: str) -> int:
        _file_size = os.path.getsize(file_path)
        if _file_size and _file_size % StructFmt.L2_CACHE_DATA_SIZE != 0:
            logging.error("l2 cache data is not complete, and the file name is %s", os.path.basename(file_path))
        return _file_size

    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """
        if self._l2_cache_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self._l2_cache_data)
            self._model.finalize()

    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """
        l2_cache_files = self._file_list.get(DataTag.L2CACHE)
        for _file in l2_cache_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)

            _file_size = L2CacheParser._check_file_complete(_file_path)
            if not _file_size:
                return
            with FileOpen(_file_path, 'rb') as _l2_cache_file:
                _all_l2_cache_data = _l2_cache_file.file_reader.read(_file_size)
                for _index in range(_file_size // StructFmt.L2_CACHE_DATA_SIZE):
                    l2_cache_data_bean = L2CacheDataBean()
                    l2_cache_data_bean.decode(
                        _all_l2_cache_data[
                            _index * StructFmt.L2_CACHE_DATA_SIZE : (_index + 1) * StructFmt.L2_CACHE_DATA_SIZE
                        ]
                    )
                    self._l2_cache_data.append(
                        [
                            l2_cache_data_bean.task_type,
                            l2_cache_data_bean.stream_id,
                            l2_cache_data_bean.task_id,
                            ",".join(l2_cache_data_bean.events_list),
                        ]
                    )
            FileManager.add_complete_file(self._project_path, _file_path)

    def ms_run(self: any) -> None:
        """
        main entry
        """
        if not self._file_list:
            return
        if self._file_list.get(DataTag.L2CACHE) and self._check_l2_cache_event_valid():
            logging.info(
                "start parsing l2 cache data, files: %s, l2 cache events: %s",
                str(self._file_list.get(DataTag.L2CACHE)),
                ",".join(self._l2_cache_events),
            )
            self.parse()
            self.save()

    def _check_l2_cache_event_valid(self: any) -> bool:
        if not self._l2_cache_events:
            return False
        if len(self._l2_cache_events) > Constant.L2_CACHE_ITEM:
            logging.error("Option --L2_cache_events number should less than %s.", Constant.L2_CACHE_ITEM)
            return False
        if not set(self._l2_cache_events).issubset(Constant.L2_CACHE_EVENTS):
            logging.error("Option --L2_cache_events value should be in %s", Constant.L2_CACHE_EVENTS)
            return False
        return True
