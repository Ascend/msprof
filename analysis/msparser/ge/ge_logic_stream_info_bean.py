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


class GeLogicStreamInfoBean(StructDecoder):
    """
    ge logic stream info bean
    """

    def __init__(self: any, *args: any) -> None:
        data = args[0]
        self._logic_stream_id = data[6]
        physic_stream_num = data[7]
        start_index = 8  # physic_stream_id 从第八个开始存储
        self._physic_logic_stream_id = []
        for index in range(start_index, start_index + physic_stream_num):
            self._physic_logic_stream_id.append([data[index], self._logic_stream_id])

    @property
    def logic_stream_id(self: any) -> int:
        """
        for logic stream id
        """
        return self._logic_stream_id

    @property
    def physic_logic_stream_id(self: any) -> list:
        """
        for physic stream id
        """
        return self._physic_logic_stream_id
