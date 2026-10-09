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

from common_func.ms_constant.level_type_constant import LevelDataType
from profiling_bean.struct_info.struct_decoder import StructDecoder


class EventDataBean(StructDecoder):
    """
    EVENT bean data for the data parsing by acl parser.
    """

    def __init__(self: any, *args) -> None:
        filed = args[0]
        self._struct_type = filed[2]
        self._level = filed[1]
        self._thread_id = filed[3]
        self._request_id = filed[4]
        self._timestamp = filed[5]
        self._item_id = filed[7]

    @property
    def struct_type(self: any) -> str:
        """
        event data type
        :return: event type
        """
        return str(self._struct_type)

    @property
    def request_id(self: any) -> int:
        """
        event data request id
        :return: event request id
        """
        return self._request_id

    @property
    def timestamp(self: any) -> int:
        """
        event data time
        :return: event time
        """
        return self._timestamp

    @property
    def thread_id(self: any) -> int:
        """
        event data thread id
        :return: event thread id
        """
        return self._thread_id

    @property
    def item_id(self: any) -> int:
        """
        event data item id
        :return: event item id
        """
        return self._item_id

    @property
    def level(self: any) -> str:
        """
        event data level type
        :return: event level type
        """
        return LevelDataType(self._level).name.lower()
