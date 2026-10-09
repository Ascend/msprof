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

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeHostBean(StructDecoder):
    """
    bean for Ge host data
    """

    def __init__(self: any, *args: any) -> None:
        _data = args[0]
        self._magic_num = _data[0]
        self._data_tag = _data[1]
        self._thread_id = _data[2]
        self._op_type = str(_data[3])
        self._event_type = str(_data[4])
        self._start_time = _data[5]
        self._end_time = _data[6]
        self._op_name = str(_data[7])

    @property
    def thread_id(self: any) -> int:
        """
        thread id
        :return: None
        """
        return self._thread_id

    @property
    def op_type(self: any) -> str:
        """
        object of event
        :return: None
        """
        return self._op_type

    @property
    def event_type(self: any) -> str:
        """
        event type
        :return: event
        """
        return self._event_type

    @property
    def start_time(self: any) -> int:
        """
        start time for the event
        :return: start time
        """
        return self._start_time

    @property
    def end_time(self: any) -> int:
        """
        end time for the event
        :return: end time
        """
        return self._end_time

    @property
    def op_name(self: any) -> str:
        """
        op name
        :return: None
        """
        return self._op_name

    def check_data_complete(self: any) -> None:
        """
        check data for ge op execute bean
        :return: complete or not
        """
        return self._magic_num == NumberConstant.MAGIC_NUM and self._data_tag == 27
