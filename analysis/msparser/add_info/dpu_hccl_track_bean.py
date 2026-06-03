# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from msparser.add_info.add_info_bean import AddInfoBean


class DPUHcclTrackBean(AddInfoBean):
    """
    DPU hccl information bean data for the data parsing by acl parser.
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        filed = args[0]
        self._item_id = filed[6]
        self._ccl_tag = filed[7]
        self._group_name = filed[8]
        self._local_rank = filed[9]
        self._remote_rank = filed[10]
        self._rank_size = filed[11]
        self._stage = filed[12]
        self._notify_id = filed[13]
        self._start_time = filed[14]
        self._duration_estimated = filed[15]
        self._src_addr = filed[16]
        self._dst_addr = filed[17]
        self._data_size = filed[18]
        self._task_id = filed[19]
        self._aicpu_task_id = filed[20]
        self._stream_id = filed[21]
        self._plane_id = filed[22]
        self._npu_device_id = filed[23]
        self._dev_type = (filed[24] >> 12) & 0xF
        self._dpu_device_id = filed[24] & 0xFFF
        self._op_type = filed[25]
        self._data_type = filed[26]
        self._link_type = filed[27]
        self._transport_type = filed[28]
        self._rdma_type = filed[29]
        self._role = filed[30]
        self._work_flow_mode = filed[31]

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
    def stage(self: any) -> str:
        """
        communicate algorithm stage
        """
        return str(self._stage)

    @property
    def notify_id(self: any) -> str:
        """
        notify id
        """
        return str(self._notify_id)

    @property
    def start_time(self: any) -> int:
        """
        start time
        """
        return self._start_time

    @property
    def duration_estimated(self: any) -> float:
        """
        duration estimated
        """
        return float(self._duration_estimated)

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
    def data_size(self: any) -> int:
        """
        data size
        """
        return self._data_size

    @property
    def task_id(self: any) -> int:
        """
        task id
        """
        return self._task_id

    @property
    def aicpu_task_id(self: any) -> int:
        """
        ai cpu task id
        """
        return self._aicpu_task_id

    @property
    def stream_id(self: any) -> int:
        """
        stream id
        """
        return self._stream_id

    @property
    def plane_id(self: any) -> int:
        """
        plane id
        """
        return self._plane_id

    @property
    def npu_device_id(self: any) -> int:
        """
        npu device id
        """
        return self._npu_device_id

    @property
    def is_dpu(self: any) -> bool:
        """
        Used to distinguish the device type(DPU:1/NPU:0) of tasks.
        """
        return bool(self._dev_type)

    @property
    def dpu_device_id(self: any) -> int:
        """
        device id
        """
        return self._dpu_device_id

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

    @property
    def role(self: any) -> str:
        """
        role
        """
        return str(self._role)

    @property
    def work_flow_mode(self: any) -> str:
        """
        mode of the work flow
        """
        return str(self._work_flow_mode)
