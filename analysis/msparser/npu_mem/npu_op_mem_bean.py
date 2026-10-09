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

from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class NpuOpMemDataBean(StructDecoder):
    """
    Npu op mem data bean for the data parsing by npu op mem parser
    """

    def __init__(self: any) -> None:
        self._magic_number = None
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._addr = None
        self._size = None
        self._node_id = None
        self._total_allocate_memory = None
        self._total_reserve_memory = None
        self._device_id = None
        self._device_type = None

    @property
    def magic_number(self: any) -> str:
        """
        :return: magic_number
        """
        return self._magic_number

    @property
    def level(self: any) -> str:
        """
        :return: level
        """
        return self._level

    @property
    def type(self: any) -> str:
        """
        :return: type
        """
        return self._type

    @property
    def thread_id(self: any) -> str:
        """
        :return: thread_id
        """
        return self._thread_id

    @property
    def data_len(self: any) -> str:
        """
        :return: data_len
        """
        return self._data_len

    @property
    def timestamp(self: any) -> str:
        """
        :return: timestamp
        """
        return self._timestamp

    @property
    def addr(self: any) -> str:
        """
        :return: addr
        """
        return self._addr

    @property
    def size(self: any) -> int:
        """
        :return: size
        """
        return self._size

    @property
    def node_id(self: any) -> int:
        """
        :return: node_id
        """
        return self._node_id

    @property
    def total_allocate_memory(self: any) -> int:
        """
        :return: total_allocate_memory
        """
        return self._total_allocate_memory

    @property
    def total_reserve_memory(self: any) -> int:
        """
        :return: total_reserve_memory
        """
        return self._total_reserve_memory

    @property
    def device_id(self: any) -> int:
        """
        :return: device_id
        """
        return self._device_id

    @property
    def device_type(self: any) -> int:
        """
        :return: device_type
        """
        return self._device_type

    def npu_op_mem_decode(self: any, bin_data: any) -> any:
        """
        decode the npu op mem bin data
        :param bin_data: npu op mem bin data
        :return: instance of npu op mem
        """
        if self.construct_bean(struct.unpack(StructFmt.MEMORY_OP_FMT, bin_data)):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        """
        refresh the npu op mem data
        :param args: npu op mem bin data
        :return: True or False
        """
        _npu_op_mem_data = args[0]
        if _npu_op_mem_data:
            self._magic_number = _npu_op_mem_data[0]
            self._level = _npu_op_mem_data[1]
            self._type = _npu_op_mem_data[2]
            self._thread_id = _npu_op_mem_data[3]
            self._data_len = _npu_op_mem_data[4]
            self._timestamp = _npu_op_mem_data[5]

            self._addr = _npu_op_mem_data[6]
            self._size = _npu_op_mem_data[7]
            self._node_id = _npu_op_mem_data[8]
            self._total_allocate_memory = _npu_op_mem_data[9]
            self._total_reserve_memory = _npu_op_mem_data[10]
            self._device_id = _npu_op_mem_data[11]
            self._device_type = _npu_op_mem_data[12]
            return True
        logging.error("NPU op mem data struct is incomplete, please check the npu op mem file.")
        return False
