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
from dataclasses import dataclass
from enum import Enum

from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.biu_perf.biu_perf_chip6_model import BiuPerfChip6Model
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.biu_perf.biu_perf_bean import BiuPerfInstructionBean
from profiling_bean.biu_perf.biu_perf_bean import CoreInfoChip6
from profiling_bean.prof_enum.data_tag import DataTag


@dataclass
class InstructionData:
    group_id: int
    core_type: str
    block_id: int
    ctrl_type: int
    events: int
    current_syscnt: int


class BiuPerfChip6Parser(DataParser, MsMultiProcess):
    MAX_BLOCK_ID = 65535
    INSTR_EVENTS_LENGTH = 7
    INSTR_END_FLAG = b'\xff\xff\xff\xef'
    INSTR_END_FILLER = b'\xdd\xdd\xdd\xdd'

    class CtrlType(Enum):
        SU = 0
        VEC = 1
        CUBE = 2
        MTE1 = 3
        MTE2 = 4
        MTE3 = 5
        FIXP = 6
        START_STAMP = 14
        STATE = 15

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=bad-super-call
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = BiuPerfChip6Model(
            self._project_path, [DBNameConstant.TABLE_BIU_DATA, DBNameConstant.TABLE_BIU_INSTR_STATUS]
        )
        self._base_syscnt = 0
        self._block_id = BiuPerfChip6Parser.MAX_BLOCK_ID
        self._instruction_data = []
        self._instr_state_data = []
        self._checkpoint_data = []
        self._cur_core_data = []
        self._offset_calculator = None
        self._hwts_freq = InfoConfReader().get_freq(StrConstant.HWTS)
        self._aic_freq = InfoConfReader().get_freq(StrConstant.AIC)

    @staticmethod
    def _get_ctrl_type_name(value):
        try:
            if isinstance(value, str):
                value = int(value)
            return BiuPerfChip6Parser.CtrlType(value).name
        except (ValueError, TypeError):
            return value

    @staticmethod
    def _get_ctrl_type(chunk: bytes) -> int:
        # 取最高4bit，chunk按4字节划分，外层已校验
        return (chunk[-1] >> 4) & 0x0F

    def ms_run(self: any) -> None:
        """
        parse biu perf data and save it to db.
        :return:None
        """
        if self._file_list.get(DataTag.BIU_PERF_CHIP6, []):
            self.parse()
            self.save()

    def parse(self: any) -> None:
        """
        read biu perf data
        :return: None
        """
        biu_perf_file_dict = self._get_file_group_and_core_type()
        for core_info in biu_perf_file_dict.values():
            self._base_syscnt = 0
            self._block_id = BiuPerfChip6Parser.MAX_BLOCK_ID
            self._cur_core_data = []
            self._parse_biu_perf_data(core_info)
            self.parse_status_data()

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if self._instruction_data:
            with self._model as model:
                model.flush(self._instruction_data, DBNameConstant.TABLE_BIU_DATA)
                model.flush(self._instr_state_data + self._checkpoint_data, DBNameConstant.TABLE_BIU_INSTR_STATUS)

    def read_binary_data(self: any, file_name: str, core_info: CoreInfoChip6) -> int:
        """
        parsing biu perf data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size < StructFmt.BIU_PERF_FMT_SIZE:
            logging.error("Biu perf data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as file_reader:
            all_bytes = self._offset_calculator.pre_process(file_reader.file_reader, os.path.getsize(files))

        chunks_list = list(Utils.chunks(all_bytes, StructFmt.BIU_PERF_FMT_SIZE))
        index = 0
        while index < len(chunks_list):
            chunk = chunks_list[index]
            # 检查 chunk 是否为 0xEFFFFFFF,
            if chunk == BiuPerfChip6Parser.INSTR_END_FLAG:
                index += 1
                # 跳过后续的填充数据 0xDDDDDDDD
                while index < len(chunks_list) and chunks_list[index] == BiuPerfChip6Parser.INSTR_END_FILLER:
                    index += 1
                continue
            instr_bean = BiuPerfInstructionBean.decode(chunk)
            if instr_bean.ctrl_type == BiuPerfChip6Parser.CtrlType.START_STAMP.value:
                # timestamp has 4 chunks, need 3 other chunks
                if index + 3 < len(chunks_list) and self._check_timestamp_type(chunks_list, index):
                    timestamp_chunks = chunks_list[index : index + 4]
                    self._init_timestamp_and_block(timestamp_chunks)
                else:
                    logging.error("There are less than 4 timestamp chunks, data is lost.")
                    return NumberConstant.ERROR
                index += 4  # 4 timestamp chunks length
            else:
                self._base_syscnt += self._trans_aic_cycle_to_hwts_cycle(instr_bean.sys_cnt)
                data = [
                    core_info.group_id,
                    core_info.core_type,
                    self._block_id,
                    instr_bean.ctrl_type,
                    instr_bean.events,
                    self._base_syscnt,
                ]
                self._instruction_data.append(data)
                self._cur_core_data.append(data)
                index += 1  # normal chunk length
        return NumberConstant.SUCCESS

    def parse_status_data(self):
        """
        解析状态数据，监控bit 0到6的变化，并记录变成1的时间和变成0的时间差
        """
        status = {
            'last_states': [0] * self.INSTR_EVENTS_LENGTH,  # 记录每个bit的上一次状态
            'last_syscnts': [0] * self.INSTR_EVENTS_LENGTH,  # 记录每个bit的上一次base_syscnt
        }
        for data in self._cur_core_data:
            insrt_data = InstructionData(*data)
            if insrt_data.ctrl_type == BiuPerfChip6Parser.CtrlType.STATE.value:
                self._calculate_status_syscnt(insrt_data, status)
            elif insrt_data.ctrl_type in {item.value for item in BiuPerfChip6Parser.CtrlType}:
                self._checkpoint_data.append(
                    [
                        insrt_data.group_id,
                        insrt_data.core_type,
                        insrt_data.block_id,
                        self._get_ctrl_type_name(insrt_data.ctrl_type),
                        insrt_data.current_syscnt,
                        0,
                        insrt_data.events,
                    ]
                )

    def _calculate_status_syscnt(self, insrt_data, status):
        for instr_index in range(self.INSTR_EVENTS_LENGTH):
            current_status = (insrt_data.events >> instr_index) & 1
            is_status_updated = current_status != status['last_states'][instr_index]
            if is_status_updated and current_status == 1:
                status['last_syscnts'][instr_index] = insrt_data.current_syscnt
            elif is_status_updated and current_status == 0:
                dur = insrt_data.current_syscnt - status['last_syscnts'][instr_index]
                self._instr_state_data.append(
                    [
                        insrt_data.group_id,
                        insrt_data.core_type,
                        insrt_data.block_id,
                        self._get_ctrl_type_name(instr_index),
                        status['last_syscnts'][instr_index],
                        dur,
                        None,
                    ]
                )
            status['last_states'][instr_index] = current_status

    def _init_timestamp_and_block(self, chunks: list):
        """
        拼接4个chunk的低16位，生成64位时间戳
        正确顺序：chunk3的低16位在最高位，chunk0的在最低位
        拼接前两个chunk的16-23位，生成16位block ID
        """
        # 生成64位时间戳
        syscnt = 0
        for chunk in reversed(chunks):  # 逆序处理chunks
            low_16_bits = int.from_bytes(chunk[:2], byteorder='little')
            syscnt = (syscnt << 16) | low_16_bits
        self._base_syscnt = syscnt
        # 生成16位block ID
        self._block_id = (chunks[1][2] << 8) | chunks[0][2]

    def _check_timestamp_type(self, chunks_list, index):
        for i in range(index + 1, index + 4):
            next_chunk = chunks_list[i]
            if self._get_ctrl_type(next_chunk) != BiuPerfChip6Parser.CtrlType.START_STAMP.value:
                return False
        return True

    def _get_file_group_and_core_type(self: any) -> dict:
        biu_perf_all_files = self._file_list.get(DataTag.BIU_PERF_CHIP6, [])
        biu_perf_file_dict = {}
        for _file in biu_perf_all_files:
            parts = _file.split(".")
            if len(parts) < 3:  # file name need 3 parts
                continue
            core_name = parts[1]
            core_info = biu_perf_file_dict.setdefault(core_name, CoreInfoChip6(core_name))
            core_info.file_list.append(_file)
        return biu_perf_file_dict

    def _parse_biu_perf_data(self: any, core_info: CoreInfoChip6):
        self._offset_calculator = OffsetCalculator(core_info.file_list, StructFmt.BIU_PERF_FMT_SIZE, self._project_path)
        for file_name in core_info.file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._handle_original_data(file_name, core_info)

    def _handle_original_data(self: any, file_name: str, core_info: CoreInfoChip6) -> None:
        if self.read_binary_data(file_name, core_info) == NumberConstant.ERROR:
            logging.error('Parse Biu perf data file: %s error.', file_name)
        FileManager.add_complete_file(self._project_path, file_name)

    def _trans_aic_cycle_to_hwts_cycle(self, cycle: int) -> float:
        return cycle * self._hwts_freq / self._aic_freq
