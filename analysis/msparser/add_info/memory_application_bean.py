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

from msparser.add_info.add_info_bean import AddInfoBean


class MemoryApplicationBean(AddInfoBean):
    """
    memory application bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._node_id = data[6]
        self._ptr = data[7]
        self._memory_size = data[8]
        self._total_memory_size = data[9]
        self._used_memory_size = data[10]
        self._device_type = data[11]
        self._device_id = data[12]

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def ptr(self: any) -> str:
        """
        for ptr
        """
        return str(self._ptr)

    @property
    def memory_size(self: any) -> str:
        """
        for memory size
        """
        return str(self._memory_size)

    @property
    def total_memory_size(self: any) -> str:
        """
        for total memory size
        """
        return str(self._total_memory_size)

    @property
    def used_memory_size(self: any) -> str:
        """
        for used memory size
        """
        return str(self._used_memory_size)

    @property
    def device_type(self: any) -> str:
        """
        for device type
        """
        return str(self._device_type)

    @property
    def device_id(self: any) -> int:
        """
        for device id
        """
        return self._device_id
