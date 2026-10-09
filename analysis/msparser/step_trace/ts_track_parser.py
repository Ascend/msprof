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
from enum import Enum

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from mscalculate.step_trace.create_step_table import StepTableBuilder
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.step_trace.helper.model_with_q_parser import ModelWithQParser
from msparser.step_trace.ts_binary_data_reader.block_num_reader import BlockNumReader
from msparser.step_trace.ts_binary_data_reader.step_trace_reader import StepTraceReader
from msparser.step_trace.ts_binary_data_reader.task_type_reader import TaskTypeReader
from msparser.step_trace.ts_binary_data_reader.ts_memcpy_reader import TsMemcpyReader
from msparser.step_trace.ts_binary_data_reader.task_flip_reader import TaskFlipReader
from profiling_bean.prof_enum.data_tag import DataTag


class TsTrackTag(Enum):
    STEP_TRACE = 10
    TS_MEMCPY = 11
    TS_TASK_TYPE = 12
    TS_TASK_FLIP_OLD = 13
    TS_TASK_FLIP = 14
    TS_BLOCK_NUM = 15
    MODEL_WITH_Q = 61

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


class TstrackParser(DataParser, MsMultiProcess):
    """
    parsing ts track data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, [])
        self._file_list = file_list
        self._model = TsTrackModel(
            self.project_path,
            DBNameConstant.DB_STEP_TRACE,
            [
                DBNameConstant.TABLE_STEP_TRACE,
                DBNameConstant.TABLE_TS_MEMCPY,
                DBNameConstant.TABLE_MODEL_WITH_Q,
                DBNameConstant.TABLE_TASK_TYPE,
                DBNameConstant.TABLE_DEVICE_TASK_FLIP,
                DBNameConstant.TABLE_BLOCK_NUM,
            ],
        )
        self.tag_reader = {
            TsTrackTag.STEP_TRACE: StepTraceReader(),
            TsTrackTag.TS_MEMCPY: TsMemcpyReader(),
            TsTrackTag.MODEL_WITH_Q: ModelWithQParser(),
            TsTrackTag.TS_TASK_TYPE: TaskTypeReader(),
            TsTrackTag.TS_TASK_FLIP: TaskFlipReader(),
            TsTrackTag.TS_BLOCK_NUM: BlockNumReader(),
        }

    def parse_binary_data(self: any, file_list: list, format_size: int, tag_fmt: str) -> None:
        """
        parsing binary data
        """
        _offset_calculator = OffsetCalculator(file_list, format_size, self.project_path)
        file_list.sort(key=lambda x: int(x.split("_")[-1]))
        for file_name in file_list:
            if not is_valid_original_data(file_name, self.project_path):
                continue
            logging.info("start parsing rts data file: %s", file_name)
            FileManager.add_complete_file(self.project_path, file_name)
            file_path = self.get_file_path_and_check(file_name)
            file_size = os.path.getsize(file_path)
            with FileOpen(file_path, 'rb') as file:
                all_log_bytes = _offset_calculator.pre_process(file.file_reader, file_size)
                for bean_data in Utils.chunks(all_log_bytes, format_size):
                    _, tag = struct.unpack_from(tag_fmt, bean_data)
                    if tag not in TsTrackTag.member_map():
                        continue
                    reader = self.tag_reader.get(TsTrackTag(tag))
                    if reader:
                        reader.read_binary_data(bean_data)

    def parse(self: any) -> None:
        """
        start parsing the data
        """
        ts_track_file = sorted(self._file_list.get(DataTag.TS_TRACK, []))
        helper_file = sorted(self._file_list.get(DataTag.HELPER_MODEL_WITH_Q, []))
        self.parse_binary_data(ts_track_file, StructFmt.STEP_TRACE_FMT_SIZE, StructFmt.STEP_HEADER_FMT)
        self.parse_binary_data(helper_file, StructFmt.HELPER_MODEL_WITH_Q_FMT_SIZE, StructFmt.HELPER_HEADER_FMT)

    def save(self: any) -> None:
        """
        save ts track data
        """
        if any(reader.data for reader in self.tag_reader.values()):
            self._model.init()
            for reader in self.tag_reader.values():
                if reader.data:
                    self._model.create_table(reader.table_name)
                    self._model.flush(reader.table_name, reader.data)
            self._model.finalize()

    def parse_and_save(self: any) -> None:
        self.parse()
        self.save()
        StepTableBuilder.run(self.sample_config)

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not self._file_list:
            return
        try:
            self.parse_and_save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
