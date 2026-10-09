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

from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class InterSoc(StructDecoder):
    """
    class used to decode inter soc transmission
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        self._func_type = Utils.get_func_type(filed[0])
        self._sys_cnt = filed[3]
        # 7 is 0b0000111 keep 4 lower bits which represent mata_bw_level
        self._mata_bw_level = filed[8] & 7
        # 2040 is 0b11111111000 keep 3 higher bits which represent buffer_bw_level
        self._buffer_bw_level = (filed[8] & 2040) >> 3

    @property
    def func_type(self: any) -> str:
        """
        get func type
        :return: func type
        """
        return self._func_type

    @property
    def mata_bw_level(self: any) -> int:
        """
        get mata bandwidth level
        :return: mata bandwidth level
        """
        return self._mata_bw_level

    @property
    def l2_buffer_bw_level(self: any) -> int:
        """
        get buffer bandwidth level
        :return: buffer bandwidth level
        """
        return self._buffer_bw_level

    @property
    def acc_type(self: any) -> str:
        """
        for acc type
        l2 buffer bw level is the higher 6 bits of the byte.
        64512 is 0b1111110000000000 keep the higher 6 bits which represent acc_type
        """
        return bin(self._data[0] & 64512)[2:8].zfill(6)

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self._sys_cnt

    @property
    def sys_time(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)
