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

import struct

from common_func.constant import Constant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeFusionOpInfoBean(StructDecoder):
    """
    ge fusion op info bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._fusion_name = Constant.DEFAULT_VALUE
        self._input_mem_size = Constant.DEFAULT_VALUE
        self._output_mem_size = Constant.DEFAULT_VALUE
        self._weight_mem_size = Constant.DEFAULT_VALUE
        self._workspace_mem_size = Constant.DEFAULT_VALUE
        self._total_mem_size = Constant.DEFAULT_VALUE
        self._fusion_op_num = Constant.DEFAULT_VALUE
        self._fusion_op = Constant.DEFAULT_VALUE
        self._hash_flag = Constant.NO_HASH_DICT_FLAG

    @property
    def hash_flag(self: any) -> int:
        """
        for task id
        """
        return self._hash_flag

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def fusion_name(self: any) -> str:
        """
        for stream id
        """
        return self._fusion_name

    @property
    def input_mem_size(self: any) -> str:
        """
        for sys cnt
        """
        return str(self._input_mem_size)

    @property
    def output_mem_size(self: any) -> str:
        """
        for sys cnt
        """
        return str(self._output_mem_size)

    @property
    def weight_mem_size(self: any) -> str:
        """
        for sys cnt
        """
        return str(self._weight_mem_size)

    @property
    def workspace_mem_size(self: any) -> str:
        """
        for sys cnt
        """
        return str(self._workspace_mem_size)

    @property
    def total_mem_size(self: any) -> str:
        """
        for sys cnt
        """
        return str(self._total_mem_size)

    @property
    def fusion_op_num(self: any) -> int:
        """
        for sys cnt
        """
        return self._fusion_op_num

    @property
    def fusion_op(self: any) -> str:
        """
        for sys cnt
        """
        return ",".join(str(i) for i in list(self._fusion_op) if i != 0)

    def is_hash_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._hash_flag == Constant.HASH_DICT_FLAG

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode fusion info binary data
        :param binary_data:
        :return:
        """
        pre_fusion_data = struct.unpack_from(
            StructFmt.BYTE_ORDER_CHAR + StructFmt.GE_FUSION_PRE_FMT, binary_data[: StructFmt.GE_FUSION_PRE_SIZE]
        )
        self._hash_flag = pre_fusion_data[3]
        fmt = "HHI8B120s14Q8B"
        if self.is_hash_type():
            fmt = "HHI8BQ112B14Q8B"
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the acl data
        :param args: acl bin data
        :return: True or False
        """
        self._fusion_data = args[0]
        self._model_id = self._fusion_data[2]
        self._fusion_name = self._fusion_data[11]
        self._input_mem_size = self._fusion_data[12]
        self._output_mem_size = self._fusion_data[13]
        self._weight_mem_size = self._fusion_data[14]
        self._workspace_mem_size = self._fusion_data[15]
        self._total_mem_size = self._fusion_data[16]
        self._fusion_op_num = self._fusion_data[17]
        self._fusion_op = self._fusion_data[18:26]
        if self.is_hash_type():
            self._fusion_name = self._fusion_data[11]
            self._input_mem_size = self._fusion_data[124]
            self._output_mem_size = self._fusion_data[125]
            self._weight_mem_size = self._fusion_data[126]
            self._workspace_mem_size = self._fusion_data[127]
            self._total_mem_size = self._fusion_data[128]
            self._fusion_op_num = self._fusion_data[129]
            self._fusion_op = self._fusion_data[130:138]
