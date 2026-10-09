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

from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class AcsqTaskV6(StructDecoder):
    """
    class used to decode acsq task v6
    """

    def __init__(self: any, *args: any) -> None:
        args = args[0]
        # total 16 bit, get lower 6 bit
        self._func_type = Utils.get_func_type(args[0])
        # total 16 bit, get 6 to 9 bit
        self._cnt = bin(args[0] & 960)[2:6].zfill(6)
        # total 16 bit, get high 6 bit
        self._task_type = args[0] >> 10
        # 唯一ID合并stream id和task id
        self._stream_id = args[2]
        self._task_id = StarsCommon.set_unique_task_id(args[2], args[3])
        self._sys_cnt = args[4]
        # [acsq_id, acc_id] is total 16 bit, acc_id is the lower 6 bit
        self._acc_id = args[6] & 63
        # acsq_id is higher 10 bit where only lower 7 bit is valid
        self._acsq_id = (args[6] >> 6) & 127

    @property
    def func_type(self: any) -> str:
        """
        get func type
        :return: func type
        """
        return self._func_type

    @property
    def task_type(self: any) -> int:
        """
        get task type
        :return: task type
        """
        return self._task_type

    @property
    def stream_id(self: any) -> int:
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @stream_id.setter
    def stream_id(self: any, stream_id: int):
        """
        set stream id
        """
        self._stream_id = stream_id

    @property
    def task_id(self: any) -> int:
        """
        get task id
        :return: task id
        """
        return self._task_id

    @property
    def sys_cnt(self: any) -> float:
        """
        get task sys cnt
        :return: sys cnt
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)

    @property
    def acc_id(self: any) -> int:
        """
        get acc id
        :return: acc_id
        """
        return self._acc_id
