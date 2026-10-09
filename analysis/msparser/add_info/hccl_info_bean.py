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

from common_func.ms_constant.level_type_constant import LevelDataType
from profiling_bean.struct_info.struct_decoder import StructDecoder


class HcclInfoBean(StructDecoder):
    """
    hccl information bean data for the data parsing by acl parser.
    """

    def __init__(self: any, *args) -> None:
        filed = args[0]
        self._level = filed[1]
        self._struct_type = filed[2]
        self._thread_id = filed[3]
        self._data_len = filed[4]
        self._timestamp = filed[5]
        self._item_id = filed[6]
        self._ccl_tag = filed[7]
        self._group_name = filed[8]
        self._local_rank = filed[9]
        self._remote_rank = filed[10]
        self._rank_size = filed[11]
        self._work_flow_mode = filed[12]
        self._plane_id = filed[13]
        self._context_id = filed[14]
        self._notify_id = filed[15]
        self._stage = filed[16]
        self._role = filed[17]
        self._duration_estimated = filed[18]
        self._src_addr = filed[19]
        self._dst_addr = filed[20]
        self._size = filed[21]
        self._op_type = filed[22]
        self._data_type = filed[23]
        self._link_type = filed[24]
        self._transport_type = filed[25]
        self._rdma_type = filed[26]

    @property
    def struct_type(self: any) -> str:
        """
        hccl information type
        :return: hccl information type
        """
        return str(self._struct_type)

    @property
    def level(self: any) -> str:
        """
        hccl information level
        """
        return LevelDataType(self._level).name.lower()

    @property
    def thread_id(self: any) -> int:
        """
        hccl information id
        :return: hccl information id
        """
        return self._thread_id

    @property
    def data_len(self: any) -> int:
        """
        hccl information data length
        """
        return self._data_len

    @property
    def timestamp(self: any) -> int:
        """
        hccl information timestamp
        """
        return self._timestamp

    @property
    def item_id(self: any) -> str:
        """
        hccl information id
        :return: hccl information id
        """
        return str(self._item_id)

    @property
    def ccl_tag(self: any) -> str:
        """
        hash number for ccl
        :return: hash number for ccl
        """
        return str(self._ccl_tag)

    @property
    def group_name(self: any) -> str:
        """
        hash number for ccl group
        :return: hash number for ccl group
        """
        return str(self._group_name)

    @property
    def local_rank(self: any) -> int:
        """
        local rank number
        :return: local rank number
        """
        return self._local_rank

    @property
    def remote_rank(self: any) -> int:
        """
        remote rank number
        :return: remote rank number
        """
        return self._remote_rank

    @property
    def rank_size(self: any) -> int:
        """
        hccl information rank size
        """
        return self._rank_size

    @property
    def work_flow_mode(self: any) -> str:
        """
        mode of the work flow
        :return: mode of the work flow
        """
        return str(self._work_flow_mode)

    @property
    def plane_id(self: any) -> int:
        """
        plane id
        """
        return self._plane_id

    @property
    def context_id(self: any) -> int:
        return self._context_id

    @property
    def notify_id(self: any) -> str:
        """
        notify id
        """
        return str(self._notify_id)

    @property
    def stage(self: any) -> str:
        """
        communicate algorithm stage
        """
        return str(self._stage)

    @property
    def role(self: any) -> str:
        """
        role
        """
        return str(self._role)

    @property
    def duration_estimated(self: any) -> int:
        """
        duration_estimated
        """
        return self._duration_estimated

    @property
    def src_addr(self: any) -> str:
        """
        source address
        """
        return str(self._src_addr)

    @property
    def dst_addr(self: any) -> str:
        """
        destination address
        """
        return str(self._dst_addr)

    @property
    def size(self: any) -> int:
        """
        data volume
        """
        return self._size

    @property
    def op_type(self: any) -> str:
        """
        op type
        """
        return str(self._op_type)

    @property
    def data_type(self: any) -> str:
        """
        data type
        """
        return str(self._data_type)

    @property
    def link_type(self: any) -> str:
        """
        link type
        """
        return str(self._link_type)

    @property
    def transport_type(self: any) -> str:
        """
        transport type
        """
        return str(self._transport_type)

    @property
    def rdma_type(self: any) -> str:
        """
        RDMA type
        """
        return str(self._rdma_type)
