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

from common_func.decode_tool.decode_tool import DecodeTool
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CyclesBean(StructDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        self._vector_cycles = filed[0]
        self._scalar_cycles = filed[1]
        self._cube_cycles = filed[2]
        self._lsu1_cycles = filed[3]
        self._lsu2_cycles = filed[4]
        self._lsu3_cycles = filed[5]
        self._timestamp_lsb0 = filed[7]
        self._timestamp_lsb1 = filed[8]
        self._timestamp_msb0 = filed[14]
        self._timestamp_msb1 = filed[15]
        self._timestamp_msb2 = filed[20]
        self.timestamp = str(
            self._timestamp_lsb0
            + (self._timestamp_lsb1 << 12)
            + (self._timestamp_msb0 << 12 * 2)
            + (self._timestamp_msb1 << 12 * 3)
            + (self._timestamp_msb2 << 12 * 4)
        )

    @property
    def vector_cycles(self: any) -> int:
        """
        get vector cycles
        :return: vector cycles
        """
        return self._vector_cycles

    @property
    def scalar_cycles(self: any) -> int:
        """
        get scalar cycles
        :return: scalar cycles
        """
        return self._scalar_cycles

    @property
    def cube_cycles(self: any) -> int:
        """
        get cube cycles
        :return: cube cycles
        """
        return self._cube_cycles

    @property
    def lsu1_cycles(self: any) -> int:
        """
        get lsu1 cycles
        :return: lsu1 cycles
        """
        return self._lsu1_cycles

    @property
    def lsu2_cycles(self: any) -> int:
        """
        get lsu2 cycles
        :return: lsu2 cycles
        """
        return self._lsu2_cycles

    @property
    def lsu3_cycles(self: any) -> int:
        """
        get lsu3 cycles
        :return: lsu3 cycles
        """
        return self._lsu3_cycles

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary dato to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        fmt = cls.get_fmt()
        decode_tool = DecodeTool()
        return cls((decode_tool.decode_byte(fmt, binary_data)))
