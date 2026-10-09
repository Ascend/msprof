#!/usr/bin/python3
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StarsQosBean(StructDecoder):
    """
    bean for stars qos data
    """

    def __init__(self: any, *args) -> None:
        field = args[0]
        self._die_id = field[0] >> 10
        self._sys_cnt = field[3]
        self._qos_bw_data = field[5:]

    @property
    def die_id(self: any) -> int:
        return self._die_id

    @property
    def timestamp(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)

    @property
    def qos_bw_data(self: any) -> list:
        return list(self._qos_bw_data)
