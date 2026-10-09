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

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StaticOpMemBean(StructDecoder):
    """
    Operator memory bean under static graph scenarios
    """

    NODE_INDEX_END_MAX = 4294967294
    CORRECT_NODE_INDEX_END_MAX = 4294967295

    def __init__(self: any, *args) -> None:
        filed = args[0]
        self._op_mem_size = filed[3]
        self._op_name = filed[4]
        self._life_start = filed[5]
        self._life_end = filed[6]
        self._total_alloc_mem = filed[7]
        self._dyn_op_name = filed[8]
        self._graph_id = filed[9]

    @property
    def op_mem_size(self: any) -> int:
        """
        Operator memory size
        """
        return self._op_mem_size / NumberConstant.BYTES_TO_KB

    @property
    def op_name(self: any) -> int:
        """
        Operator name
        """
        return self._op_name

    @property
    def life_start(self: any) -> int:
        """
        Serial number of operator memory used
        """
        return self._life_start

    @property
    def life_end(self: any) -> int:
        """
        Serial number of operator memory used
        """
        if self._life_end == self.NODE_INDEX_END_MAX:
            return self.CORRECT_NODE_INDEX_END_MAX
        return self._life_end

    @property
    def total_alloc_mem(self: any) -> int:
        """
        Static graph total allocate memory
        """
        return self._total_alloc_mem

    @property
    def dyn_op_name(self: any) -> int:
        """
        0: invalid, other: dynamic op name of root
        """
        return self._dyn_op_name

    @property
    def graph_id(self: any) -> int:
        """
        Graph id
        """
        return self._graph_id
