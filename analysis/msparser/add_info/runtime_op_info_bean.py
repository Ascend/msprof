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
import struct

from common_func.constant import Constant
from common_func.ms_constant.ge_enum_constant import GeTaskType
from msparser.add_info.add_info_bean import AddInfoBean
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.ge.ge_tensor_base_bean import GeTensorBaseBean


class RuntimeOpInfoBean(AddInfoBean):
    """
    Runtime Op Info Bean Body
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._model_id = data[6]
        self._device_id = data[7]
        self._stream_id = data[8]
        self._task_id = data[9]
        self._task_type = data[10]
        self._block_num = data[11]
        self._node_id = data[12]
        self._op_type = data[13]
        self._hash_id = data[14]
        # reserve * 1
        self._op_flag = data[16]
        self._tensor_num = data[17]

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def device_id(self: any) -> int:
        """
        for device id
        """
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        """
        for stream id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        for task id
        """
        return self._task_id

    @property
    def task_type(self: any) -> str:
        """
        for task type
        """
        task_type_dict = GeTaskType.member_map()
        if self._task_type not in task_type_dict:
            logging.error("Unsupported task_type %d", self._task_type)
            return str(self._task_type)
        return task_type_dict.get(self._task_type).name

    @property
    def op_type(self: any) -> str:
        """
        for op type
        """
        return str(self._op_type)

    @property
    def hash_id(self: any) -> str:
        """
        for attr hashId
        """
        return str(self._hash_id) if self._hash_id else Constant.NA

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def op_flag(self: any) -> int:
        """
        for op flag
        """
        return self._op_flag

    @property
    def block_num(self: any) -> int:
        """
        for block num
        get lower 16bit data of 32bit
        """
        return self._block_num & 65535

    @property
    def mix_block_num(self: any) -> int:
        """
        for mix block num
        get the product of block num and higher 16bit
        """
        return (self._block_num & 65535) * (self._block_num >> 16)

    @property
    def tensor_num(self: any) -> str:
        """
        for tensor num
        """
        return self._tensor_num


class RuntimeOpInfo256Bean(RuntimeOpInfoBean, GeTensorBaseBean):
    """
    Runtime Op Info Bean (length 256)
    """

    TENSOR_LEN = 11
    TENSOR_PER_LEN = 18

    def __init__(self: any, *args) -> None:
        RuntimeOpInfoBean.__init__(self, *args)
        GeTensorBaseBean.__init__(self)
        data = args[0]
        self._deal_with_tensor_data(data[self.TENSOR_PER_LEN :], self.tensor_num, self.TENSOR_LEN)


class RuntimeTensorBean(GeTensorBaseBean):
    """
    runtime tensor bean
    """

    TENSOR_LEN = 11
    TENSOR_PER_LEN = 0

    def decode(self: any, binary_data: bytes, additional_fmt: str, tensor_num: int) -> any:
        parse_data = struct.unpack_from(StructFmt.BYTE_ORDER_CHAR + tensor_num * additional_fmt, binary_data)
        self._deal_with_tensor_data(parse_data[self.TENSOR_PER_LEN :], tensor_num, self.TENSOR_LEN)
        return self
