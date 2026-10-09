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
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CoreInfoChip6:
    AI_CUBE = "aic"
    AI_VECTOR0 = "aiv0"
    AI_VECTOR1 = "aiv1"

    def __init__(self: any, core_name: str) -> None:
        parts = core_name.split("_")
        self._core_name = core_name
        self._group_id = parts[2]
        self._core_type = parts[3]
        self.file_list = []

    @property
    def core_name(self: any) -> str:
        """
        get core name
        """
        return self._core_name

    @property
    def group_id(self: any) -> int:
        """
        get group id
        """
        return int(self._group_id[-1])

    @property
    def core_type(self: any) -> str:
        """
        get core type
        """
        return self._core_type


class BiuPerfInstructionBean(StructDecoder):
    def __init__(self: any, *args: any):
        biu_perf_data = args[0]
        self._sys_cnt = biu_perf_data[0]
        # total 16 bit, ctrl type get 11 to 15, events get 0 to 11
        self._ctrl_type = (biu_perf_data[1] >> 12) & 0xF
        self._events = biu_perf_data[1] & 0xFFF

    @property
    def ctrl_type(self: any) -> int:
        """
        get ctrl type
        :return: ctrl_type
        """
        return self._ctrl_type

    @property
    def events(self: any) -> int:
        """
        get events
        :return: events
        """
        return self._events

    @property
    def sys_cnt(self: any) -> int:
        """
        get task sys cnt
        :return: sys cnt
        """
        return self._sys_cnt
