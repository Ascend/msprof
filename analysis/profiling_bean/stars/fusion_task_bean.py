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

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FusionTaskBean(StructDecoder):
    """
    class used to decode fusion task log
    """

    MAGIC_NUM = 0x6BD3

    def __init__(self: any, *args: any) -> None:
        args = args[0]
        # Word 0 (16bit): func_type[5:0] | cnt[9:6] | sqe_type[15:10]
        self._func_type = Utils.get_func_type(args[0])
        self._cnt = (args[0] >> 6) & 0xF
        self._sqe_type = args[0] >> 10
        # Word 1 (16bit): magic
        self._magic = args[1]
        # Word 2 (32bit) for unique id
        self._stream_id = 0
        self._task_id = args[2]
        # Word 3 (64bit): syscnt
        self._sys_cnt = InfoConfReader().time_from_syscnt(args[3])
        # Word 4 (16bit): fusion_task_type[4:0], mission_id[8:5]
        self._fusion_task_type = args[4] & 0x1F
        self._mission_id = (args[4] >> 5) & 0xF
        # Word 5 (16bit): acc_id
        self._acc_id = args[5]

    @property
    def func_type(self: any) -> str:
        """get func type"""
        return self._func_type

    @property
    def cnt(self: any) -> int:
        """get cnt"""
        return self._cnt

    @property
    def task_type(self: any) -> int:
        """get sqe type (used as task_type for LogBaseParser compatibility)"""
        return self._sqe_type

    @property
    def magic(self: any) -> int:
        """get magic"""
        return self._magic

    @property
    def stream_id(self: any) -> int:
        """get stream id"""
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """get task id"""
        return self._task_id

    @property
    def sys_cnt(self: any) -> float:
        """get sys cnt (converted to time)"""
        return self._sys_cnt

    # one-hot bit → type name: bit0=cpu, bit1=aicpu, bit2=aicore, bit3=ccu, bit4=ccu
    _FUSION_TYPE_BIT_MAP = {0: 'CPU', 1: 'AICPU', 2: 'AICORE', 3: 'CCU', 4: 'CCU'}
    _CCU_DIE_BIT_MAP = {3: 0, 4: 1}

    @property
    def fusion_task_type(self: any) -> str:
        """get fusion task type display name"""
        return self.get_fusion_type_display(self._fusion_task_type)[0]

    @property
    def ccu_die_id(self: any) -> int:
        """get ccu die id, None if not a CCU task"""
        return self.get_fusion_type_display(self._fusion_task_type)[1]

    @classmethod
    def get_fusion_type_display(cls, raw_value: int) -> tuple:
        """
        Convert one-hot raw value to (type_name, ccu_die_id_or_None).
        bit0=cpu, bit1=aicpu, bit2=aicore, bit3=ccu_die0, bit4=ccu_die1
        """
        if not raw_value:
            return Constant.TASK_TYPE_UNKNOWN, None
        bit = raw_value.bit_length() - 1
        type_name = cls._FUSION_TYPE_BIT_MAP.get(bit, Constant.TASK_TYPE_UNKNOWN)
        die_id = cls._CCU_DIE_BIT_MAP.get(bit) if type_name == 'CCU' else None
        return type_name, die_id

    @property
    def mission_id(self: any) -> int:
        """get mission id from fusion_task_type bits[8:5]"""
        return self._mission_id

    @property
    def acc_id(self: any) -> int:
        """get acc id"""
        return self._acc_id
