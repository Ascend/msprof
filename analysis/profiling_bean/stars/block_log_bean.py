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

from common_func.utils import Utils
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class BlockLogBean(StructDecoder):
    """
    class used to decode AIC/AIV block log
    """

    def __init__(self: any, *args: any) -> None:
        args = args[0]
        # total 16 bit, get lower 6 bit
        self._func_type = Utils.get_func_type(args[0])
        # total 16 bit, get 6 to 9 bit
        self._cnt = bin(args[0] & 960)[2:6].zfill(6)
        # total 16 bit, get high 6 bit
        self._task_type = args[0] >> 10
        self._stream_id = args[2]
        self._task_id = StarsCommon.set_unique_task_id(args[2], args[3])
        self._sys_cnt = args[4]
        self._cxt_type = args[5]
        self._mst = args[6] & 1
        self._mix = bin(args[6] & 2)[2]
        self._core_type = args[8] & 1
        self._core_id = args[9]
        self._block_id = args[12]

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
        return self._sys_cnt

    @property
    def mst(self: any) -> int:
        """
        valid only for mix_aic/mix_aiv, 1: main block, 0: slave block；
        :return: mst
        """
        return self._mst

    @property
    def mix(self: any) -> int:
        """
        Indicates whether the SQE is a traditional SQE or a MIX SQE. 0: SQE, 1: MIX SQE
        """
        return self._mix

    @property
    def core_type(self: any) -> int:
        """
        get core type
        :return: core type
        """
        return self._core_type

    @property
    def core_id(self: any) -> int:
        """
        get core id
        :return: core id
        """
        return self._core_id

    @property
    def block_id(self: any) -> int:
        """
        get block id
        :return: block id
        """
        return self._block_id
