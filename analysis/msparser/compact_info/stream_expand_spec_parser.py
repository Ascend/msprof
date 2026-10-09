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
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.compact_info.stream_expand_spec_model import StreamExpandSpecModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class StreamExpandSpecParser(MsMultiProcess):
    """
    parsing stream expand data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self._file_list = file_list.get(DataTag.STREAM_EXPAND, [])
        self.project_path = sample_config.get("result_dir", "")
        self.calculate = OffsetCalculator(self._file_list, StructFmt.STREAM_EXPAND_FMT_SIZE, self.project_path)
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._stream_expand_data = []

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing stream expand info data
        """
        status = NumberConstant.ERROR
        stream_expand_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(stream_expand_file):
            return status
        _file_size = os.path.getsize(stream_expand_file)
        try:
            with FileOpen(stream_expand_file, "rb") as stream_info_f:
                stream_expand_info_data = self.calculate.pre_process(stream_info_f.file_reader, _file_size)
                struct_nums = _file_size // StructFmt.STREAM_EXPAND_FMT_SIZE
                struct_data = struct.unpack(
                    StructFmt.BYTE_ORDER_CHAR + StructFmt.STREAM_EXPAND_FMT * struct_nums, stream_expand_info_data
                )
                # 获取二进制文件中第7个字段: expandStatus，是否为stream扩容场景，1为扩容场景，0为非扩容场景
                self._stream_expand_data.append([struct_data[6]])
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

    def save(self: any) -> None:
        """
        save data`
        """
        if not self._stream_expand_data:
            return
        with StreamExpandSpecModel(self.project_path) as model:
            model.flush(self._stream_expand_data)

    def _original_data_handler(self: any, file_name: str) -> None:
        logging.info("start parsing Stream expand info data file: %s", file_name)
        status = self.read_binary_data(file_name)
        FileManager.add_complete_file(self.project_path, file_name)
        if status:
            logging.error('Insert Stream expand info data error.')
        logging.info("Create Stream expand info data finished!")
