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


class AicoreSample(StructDecoder):
    """
    class used to decode ai_core task
    """

    def __init__(self: any, *args: any) -> None:
        self._timestamp = None
        filed = args[0]
        self._mode = filed[0]
        self._event_count = filed[5:13]
        self._task_cyc = filed[13]
        self._timestamp = filed[14]
        self._count_num = filed[15]
        self._core_id = filed[16]

    @property
    def mode(self: any) -> str:
        """
        get mode
        :return: mode
        """
        return self._mode

    @property
    def event_count(self: any) -> int:
        """
        get event count
        :return: event count
        """
        return self._event_count

    @property
    def task_cyc(self: any) -> int:
        """
        get task cyc
        :return: task cyc
        """
        return self._task_cyc

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def count_num(self: any) -> int:
        """
        get count_num
        :return: count_num
        """
        return self._count_num

    @property
    def core_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._core_id


class AicoreSampleV6(AicoreSample):
    def __init__(self: any, *args: any) -> None:
        super().__init__(*args)
        self._timestamp = None
        filed = args[0]
        self._mode = filed[0]
        self._event_count = filed[5:15]
        self._task_cyc = filed[15]
        self._timestamp = filed[16]
        self._count_num = filed[17]
        self._core_id = filed[18]
