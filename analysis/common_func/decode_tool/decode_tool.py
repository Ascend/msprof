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

import struct

from common_func.decode_tool.byte_interpreter import ByteInterpreter


class DecodeTool:
    """
    decode byte according to expression. expression consists of symbols that belong to ByteDecoder.
    """

    FORMAT_LIST = [["T", 3, [24]], ["t", 3, [12, 12]]]

    def __init__(self: any) -> None:
        self.decoder_dict = self.extend_interpreter()

    def extend_interpreter(self: any) -> dict:
        """add a new interpreter for SpecialExpression"""
        decoder_dict = {}
        for fmt, input_byte_size, output_bit_size_list in self.FORMAT_LIST:
            decoder_dict.setdefault(fmt, ByteInterpreter(input_byte_size, output_bit_size_list))
        return decoder_dict

    def decode_byte(self: any, mon_fmts: str, several_byte_data: str) -> list:
        """decode byte according to format
        mon_fmts: format
        several_byte_data: byte_data that need to decode
        offset: starting decode location.
        decode_result:  a list which of size equals length of format
        """
        offset = 0
        decode_result = []
        for fmt in mon_fmts:
            if fmt in self.decoder_dict:
                byte_parser = self.decoder_dict.get(fmt)
                each_result = byte_parser.decode_byte(several_byte_data[offset : offset + byte_parser.input_byte_size])
                decode_result.extend(each_result)
                offset += byte_parser.input_byte_size
            else:
                each_result = struct.unpack_from(fmt, several_byte_data, offset)
                decode_result.extend(each_result)
                offset += struct.calcsize(fmt)
        return decode_result
