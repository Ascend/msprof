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

from msparser.add_info.add_info_bean import AddInfoBean


class FusionAddInfoBean(AddInfoBean):
    """
    ge fusion info bean
    """

    FUSION_LEN = 13

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._node_id = data[6]
        self._fusion_op_num = data[7]
        self._input_mem_size = data[8]
        self._output_mem_size = data[9]
        self._weight_mem_size = data[10]
        self._workspace_mem_size = data[11]
        self._total_mem_size = data[12]
        self._fusion_op_id = []
        for fusion_index in range(self.FUSION_LEN, self.FUSION_LEN + self._fusion_op_num):
            self._fusion_op_id.append(str(data[fusion_index]))

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def fusion_op_num(self: any) -> int:
        """
        for fusion op num
        """
        return self._fusion_op_num

    @property
    def input_mem_size(self: any) -> str:
        """
        for input memory size
        """
        return str(self._input_mem_size)

    @property
    def output_mem_size(self: any) -> str:
        """
        for output memory size
        """
        return str(self._output_mem_size)

    @property
    def weight_mem_size(self: any) -> str:
        """
        for weight memory size
        """
        return str(self._weight_mem_size)

    @property
    def workspace_mem_size(self: any) -> str:
        """
        for workspace memory size
        """
        return str(self._workspace_mem_size)

    @property
    def total_mem_size(self: any) -> str:
        """
        for total memory size
        """
        return str(self._total_mem_size)

    @property
    def fusion_op_id(self: any) -> str:
        """
        for fusion op id
        """
        return ','.join(self._fusion_op_id)
