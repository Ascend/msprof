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


class MultiThreadBean(StructDecoder):
    """
    multiple thread bean data for the data parsing by acl parser.
    """

    def __init__(self: any, *args) -> None:
        filed = args[0]
        self._level = filed[1]
        self._struct_type = filed[2]
        self._thread_id = filed[3]
        self._data_len = filed[4]
        self._timestamp = filed[5]
        self._thread_num = filed[6]
        self._sub_thread_id = filed[7:32]

    @property
    def struct_type(self: any) -> str:
        """
        multiple thread type
        :return: multiple thread type
        """
        return str(self._struct_type)

    @property
    def level(self: any) -> str:
        """
        multiple thread level
        """
        return LevelDataType(self._level).name.lower()

    @property
    def thread_id(self: any) -> int:
        """
        multiple thread id
        :return: multiple thread id
        """
        return self._thread_id

    @property
    def data_len(self: any) -> int:
        """
        multiple thread data length
        """
        return self._data_len

    @property
    def timestamp(self: any) -> int:
        """
        multiple thread timestamp
        """
        return self._timestamp

    @property
    def thread_num(self: any) -> int:
        """
        multiple thread number
        :return: multiple thread number
        """
        return self._thread_num

    @property
    def sub_thread_id(self: any) -> str:
        """
        multiple thread id
        :return: multiple thread id
        """
        return ",".join(map(str, self._sub_thread_id[: self._thread_num]))
