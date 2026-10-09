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

from common_func.constant import Constant
from common_func.utils import Utils
from profiling_bean.ge.ge_tensor_base_bean import GeTensorBaseBean


class GeTensorBean(GeTensorBaseBean):
    """
    ge tensor info bean
    """

    TENSOR_LEN = 11
    TENSOR_PER_LEN = 8

    def __init__(self: any) -> None:
        super().__init__()
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._index_num = Constant.DEFAULT_VALUE
        self._stream_id = Constant.DEFAULT_VALUE
        self._task_id = Constant.DEFAULT_VALUE
        self._batch_id = Constant.DEFAULT_VALUE
        self._tensor_num = Constant.DEFAULT_VALUE
        self._tensor_type = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def index_num(self: any) -> int:
        """
        for task id
        """
        return self._index_num

    @property
    def batch_id(self: any) -> int:
        """
        for batch id
        """
        return self._batch_id

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
    def tensor_num(self: any) -> int:
        """
        for tensor num
        """
        return self._tensor_num

    @property
    def tensor_type(self: any) -> int:
        """
        for tensor type
        """
        return self._tensor_type

    @property
    def timestamp(self: any) -> int:
        """
        for timestamp
        """
        return self._timestamp

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode ge tensor binary data
        :param binary_data:
        :return:
        """
        fmt = self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the ge tensor data
        :param args: ge tensor bean data
        :return: True or False
        """
        self._fusion_data = args[0]
        self._data_tag = self._fusion_data[1]
        self._model_id = self._fusion_data[2]
        self._index_num = self._fusion_data[3]
        self._stream_id = Utils.get_stream_id(self._fusion_data[4])
        self._task_id = self._fusion_data[5]
        self._batch_id = self._fusion_data[6]
        self._tensor_num = self._fusion_data[7]
        self._timestamp = self._fusion_data[63]

        self._deal_with_tensor_data(self._fusion_data[self.TENSOR_PER_LEN :], self.tensor_num, self.TENSOR_LEN)
