# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
import struct

from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CCUMissionV62BaseBean(StructDecoder):
    """
    Base bean for chip v6_2 ccu mission data.
    """

    WORD_COUNT = 8
    SYSCNT_RESTORE_SCALE = 4  # Hardware stores syscnt right-shifted by two bits.

    @staticmethod
    def _get_bits(value: int, high: int, low: int) -> int:
        return (value >> low) & ((1 << (high - low + 1)) - 1)

    @staticmethod
    def _merge_u64(low_word: int, high_word: int) -> int:
        return (high_word << 32) | low_word

    @staticmethod
    def _unpack_words(bin_data: bytes) -> tuple:
        return struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_MISSION_V62_FMT, bin_data)


class CCUSetCKBV62Bean(CCUMissionV62BaseBean):
    """
    SetCKB profiling bean for chip v6_2.
    """

    def __init__(self: any) -> None:
        super().__init__()
        self._stream_id = None
        self._task_id = None
        self._setckb_instr_id = None
        self._loop_id = None
        self._loop_cnt = None
        self._setckb_ckbinfo = None
        self._setckb_syscnt = None
        self._udf_profiling_data = None
        self._udf_flag = None
        self._syscnt_type = None
        self._lp_flg = None
        self._pfl_type = None

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def loop_id(self: any) -> int:
        return self._loop_id

    @property
    def setckb_instr_id(self: any) -> int:
        return self._setckb_instr_id

    @property
    def loop_cnt(self: any) -> int:
        return self._loop_cnt

    @property
    def setckb_ckbinfo(self: any) -> int:
        return self._setckb_ckbinfo

    @property
    def setckb_syscnt(self: any) -> int:
        return self._setckb_syscnt

    @property
    def udf_profiling_data(self: any) -> int:
        return self._udf_profiling_data

    @property
    def udf_flag(self: any) -> int:
        return self._udf_flag

    @property
    def syscnt_type(self: any) -> int:
        return self._syscnt_type

    @property
    def lp_flg(self: any) -> int:
        return self._lp_flg

    @property
    def pfl_type(self: any) -> int:
        return self._pfl_type

    def decode(self: any, bin_data: bytes) -> any:
        words = self._unpack_words(bin_data)
        if self.construct_bean(words):
            return self
        return {}

    def construct_bean(self: any, *args: tuple) -> bool:
        words = args[0]
        if not words or len(words) != self.WORD_COUNT:
            logging.error("CCU setckb v6_2 data struct is incomplete, please check the file.")
            return False
        self._stream_id = 0
        self._task_id = words[0]
        self._loop_cnt = self._get_bits(words[1], 31, 19)
        self._loop_id = self._get_bits(words[1], 18, 9)
        self._udf_flag = self._get_bits(words[1], 4, 4)
        self._syscnt_type = self._get_bits(words[1], 3, 2)
        self._lp_flg = self._get_bits(words[1], 1, 1)
        self._pfl_type = self._get_bits(words[1], 0, 0)
        self._setckb_ckbinfo = self._get_bits(words[2], 31, 16)
        self._setckb_instr_id = self._get_bits(words[2], 15, 0)
        self._setckb_syscnt = self._merge_u64(words[3], words[4]) * self.SYSCNT_RESTORE_SCALE
        self._udf_profiling_data = self._merge_u64(words[5], words[6])
        return True


class CCULoopGroupV62Bean(CCUMissionV62BaseBean):
    """
    LoopGroup profiling bean for chip v6_2.
    """

    def __init__(self: any) -> None:
        super().__init__()
        self._stream_id = None
        self._task_id = None
        self._loopgroup_instr_id = None
        self._loopgroup_start_syscnt = None
        self._loopgroup_stop_syscnt = None
        self._pfl_type = None

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def loopgroup_instr_id(self: any) -> int:
        return self._loopgroup_instr_id

    @property
    def loopgroup_start_syscnt(self: any) -> int:
        return self._loopgroup_start_syscnt

    @property
    def loopgroup_stop_syscnt(self: any) -> int:
        return self._loopgroup_stop_syscnt

    @property
    def pfl_type(self: any) -> int:
        return self._pfl_type

    def decode(self: any, bin_data: bytes) -> any:
        words = self._unpack_words(bin_data)
        if self.construct_bean(words):
            return self
        return {}

    def construct_bean(self: any, *args: tuple) -> bool:
        words = args[0]
        if not words or len(words) != self.WORD_COUNT:
            logging.error("CCU loopgroup v6_2 data struct is incomplete, please check the file.")
            return False
        self._stream_id = 0
        self._task_id = words[0]
        self._loopgroup_instr_id = self._get_bits(words[1], 31, 16)
        self._pfl_type = self._get_bits(words[1], 0, 0)
        self._loopgroup_start_syscnt = self._merge_u64(words[2], words[3]) * self.SYSCNT_RESTORE_SCALE
        self._loopgroup_stop_syscnt = self._merge_u64(words[4], words[5]) * self.SYSCNT_RESTORE_SCALE
        return True


class CCUMissionV62Bean(CCUMissionV62BaseBean):
    def decode(self, binary_data):
        words = self._unpack_words(binary_data)
        pfl_type = words[1] & 1
        bean = CCUSetCKBV62Bean() if pfl_type else CCULoopGroupV62Bean()
        return bean if bean.construct_bean(words) else None
