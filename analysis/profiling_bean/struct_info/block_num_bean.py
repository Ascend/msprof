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
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class BlockNumBean(StructDecoder):
    """
    block num for tiling-down
    """

    def __init__(self: any, *args: any) -> None:
        block_num = args[0]
        self._timestamp = block_num[4]
        self._stream_id = StarsCommon.set_stream_id(block_num[5], block_num[6])
        self._task_id = StarsCommon.set_task_id(block_num[5], block_num[6])
        self._block_num = block_num[7]

    @property
    def stream_id(self: any) -> int:
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def task_id(self: any) -> int:
        """
        get task id
        :return: task id
        """
        return self._task_id

    @property
    def block_num(self: any) -> int:
        """
        get block_num
        :return: block_num
        """
        return self._block_num
