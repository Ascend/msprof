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

from common_func.ms_constant.level_type_constant import LevelDataType
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CompactInfoBean(StructDecoder):
    """
    compact info bean
    """

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._level = data[1]
        self._struct_type = data[2]
        self._thread_id = data[3]
        self._data_len = data[4]
        self._timestamp = data[5]

    @property
    def level(self: any) -> str:
        """
        level
        """
        return LevelDataType(self._level).name.lower()

    @property
    def struct_type(self: any) -> str:
        """
        type
        """
        return str(self._struct_type)

    @property
    def thread_id(self: any) -> int:
        """
        thread id
        """
        return self._thread_id

    @property
    def data_len(self: any) -> int:
        """
        data length
        """
        return self._data_len

    @property
    def timestamp(self: any) -> int:
        """
        timestamp
        """
        return self._timestamp
