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
from profiling_bean.stars.ffts_log import FftsLogDecoder
from profiling_bean.stars.stars_common import StarsCommon
from msmodel.sqe_type_map import SqeType


class FftsPlusLogDecoder(FftsLogDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        # get lower 6 bit
        self._func_type = Utils.get_func_type(filed[0])
        # get the most significant six bits
        self._task_type = filed[0] >> 10
        self._stars_common = StarsCommon(filed[3], filed[2], filed[4], SqeType.StarsSqeType(self._task_type))
        self._subtask_id = filed[6]
        self._thread_id = filed[8]
        # get lower 8 bit
        self._subtask_type = filed[5] & 255
        # get 5-7 bit
        self._ffts_type = filed[7] >> 13
