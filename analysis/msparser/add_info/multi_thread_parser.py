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
import struct

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.add_info.multi_thread_model import MultiThreadModel
from msparser.add_info.multi_thread_bean import MultiThreadBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class MultiThreadParser(DataParser, MsMultiProcess):
    """
    parsing multi thread data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._multi_thread_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        multi_thread_files = self._file_list.get(DataTag.MULTI_THREAD, [])
        multi_thread_files = self.group_aging_file(multi_thread_files)
        for file_list in multi_thread_files.values():
            offset_calculator = OffsetCalculator(file_list, struct.calcsize(StructFmt.MULTI_THREAD_FMT),
                                                 self._project_path)
            for _file in file_list:
                if not is_valid_original_data(_file, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(_file)
                logging.info("start parsing multiple thread data file: %s", _file)
                self._read_multi_thread(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save multi thread parser data to db
        :return: None
        """
        if not self._multi_thread_data:
            return
        model = MultiThreadModel(self._project_path)
        with model as _model:
            _model.flush(self.reformat_data())

    def ms_run(self: any) -> None:
        """
        parse multi thread data and save it to db.
        :return:
        """
        if not self._file_list.get(DataTag.MULTI_THREAD, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def reformat_data(self) -> list:
        type_info_data = HashDictData(self._project_path).get_type_hash_dict().get("communication", {})
        return [
            [data.level, type_info_data.get(data.struct_type, data.struct_type), data.thread_id, data.data_len,
             data.timestamp, data.thread_num, data.sub_thread_id]
            for data in self._multi_thread_data
        ]

    def _read_multi_thread(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = struct.calcsize(StructFmt.MULTI_THREAD_FMT)
        with FileOpen(file_path, 'rb') as _multi_thread_file:
            _all_multi_thread_data = offset.pre_process(_multi_thread_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_multi_thread_data[_index * struct_size:(_index + 1) * struct_size]
                self.check_magic_num(data)
                self._multi_thread_data.append(MultiThreadBean.decode(data))
