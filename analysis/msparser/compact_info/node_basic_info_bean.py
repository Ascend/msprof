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

from common_func.ms_constant.ge_enum_constant import GeTaskType
from msparser.compact_info.compact_info_bean import CompactInfoBean


class NodeBasicInfoBean(CompactInfoBean):
    """
    Node Basic Info Bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._node_id = data[6]
        self._task_type = data[7]
        self._op_type = data[8]
        self._block_num = data[9]
        self._op_flag = data[10]
        self._mix_block_num = self.mix_block_num

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
