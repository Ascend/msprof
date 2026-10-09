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
import struct


class ByteInterpreter:
    """
    create decoder to decode byte
    input_byte_size: the number of byte
    output_bit_size_list: a list that contains the number of bit.
    input_byte_size * 8 equals sum(output_bit_size_list), such as 3 * 8 = [12, 12]
    """

    def __init__(self, input_byte_size, output_bit_size_list):
        self.input_byte_size = input_byte_size
        self.output_bit_size_list = output_bit_size_list
        self.check()

    def check(self):
        """
        check input_byte_num * 8 equals sum(output_bit_num_list)
        :return:None
        """
        if self.input_byte_size * 8 != sum(self.output_bit_size_list):
            err = "number of bit corresponding to input_byte_num should be equal to sum of output_bit_len_list"
            logging.error(err)
            raise ValueError(err)

    def decode_byte(self, byte_data):
        """
        decode byte data
        byte_data: such as b'\x01\x00\x10'
        output_list: a list which of size equals size of output_bit_num_list
        Example:
        input_byte_size = 3
        output_bit_size_list = [12, 12]
        byte_data = b'\x01\x00\x10'
        return [1, 256]
        """
        each_byte_value = struct.unpack_from("B" * self.input_byte_size, byte_data)
        whole_value = sum(each_byte_value[i] << i * 8 for i in range(self.input_byte_size))

        output_list = []
        offset = 0
        and_op_value = 0
        for bit_size in self.output_bit_size_list:
            and_op_value = (1 << bit_size + offset) - 1 - and_op_value
            output_value = (whole_value & and_op_value) >> offset
            offset += bit_size
            output_list.append(output_value)
        return output_list
