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

# pylint: disable=duplicate-code

import logging

from common_func.ms_constant.ge_enum_constant import GeTaskType
from profiling_bean.struct_info.struct_decoder import StructDecoder


class V5StarsBean(StructDecoder):
    """
    v5 device data for the data parsing.
    """

    def __init__(self: any, *args) -> None:
        filed = args[0]
        self._task_type = filed[0] & int(b'1110')
        self._model_id = filed[1]
        self._task_id = filed[4]
        self._timestamp = (filed[9] << 48) + (filed[8] << 32) + filed[5]
        self._duration = filed[6]
        self._total_cycle = (filed[10] << 32) + filed[7]
        self._pmu_list = filed[11:21]

    @property
    def task_type(self: any) -> str:
        """
        device data task_type
        task type on v5 will be AI_CORE in normal
        """
        task_type_dict = GeTaskType.member_map()
        if self._task_type not in task_type_dict:
            logging.error("Unsupported task_type %d", self._task_type)
            return str(self._task_type)
        return task_type_dict.get(self._task_type).name

    @property
    def model_id(self: any) -> int:
        """
        device data model_id
        """
        return self._model_id

    @property
    def task_id(self: any) -> int:
        """
        device data task_id
        """
        return self._task_id

    @property
    def timestamp(self: any) -> int:
        """
        device data timestamp
        """
        return self._timestamp

    @property
    def duration(self: any) -> int:
        """
        device data duration
        """
        return self._duration

    @property
    def total_cycle(self: any) -> int:
        """
        device data total_cycle
        """
        return self._total_cycle

    @property
    def pmu_list(self: any) -> tuple:
        """
        device data pmu_list
        """
        return self._pmu_list
