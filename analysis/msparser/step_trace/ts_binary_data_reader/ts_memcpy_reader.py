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

from common_func.db_name_constant import DBNameConstant
from profiling_bean.struct_info.ts_memcpy import TsMemcpy


class TsMemcpyReader:
    """
    class for ts memcpy reader
    """

    def __init__(self: any) -> None:
        self._data = []
        self._table_name = DBNameConstant.TABLE_TS_MEMCPY

    @property
    def data(self: any) -> list:
        """
        get data
        :return: data
        """
        return self._data

    @property
    def table_name(self: any) -> str:
        """
        get table_name
        :return: table_name
        """
        return self._table_name

    def read_binary_data(self: any, file_data: any) -> None:
        """
        read ts memcpy binary data and store them into list
        :param file_data: binary data
        :param index: index
        :return: None
        """
        ts_memcpy_bean = TsMemcpy.decode(file_data)
        if ts_memcpy_bean:
            self.data.append(
                (ts_memcpy_bean.timestamp, ts_memcpy_bean.stream_id, ts_memcpy_bean.task_id, ts_memcpy_bean.task_state)
            )
