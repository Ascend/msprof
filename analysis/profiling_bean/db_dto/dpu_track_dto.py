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

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class DPUTrackDto(metaclass=InstanceCheckMeta):
    npu_device_id: int = Constant.DEFAULT_INVALID_VALUE
    dpu_device_id: int = Constant.DEFAULT_INVALID_VALUE
    start_time: int = Constant.DEFAULT_INVALID_VALUE
    end_time: int = Constant.DEFAULT_INVALID_VALUE
    stream_id: int = Constant.DEFAULT_INVALID_VALUE
    aicpu_task_id: int = Constant.DEFAULT_INVALID_VALUE
    task_id: int = Constant.DEFAULT_INVALID_VALUE
    task_type: str = Constant.NA
    ccl_tag: str = Constant.NA
    data_type: str = Constant.NA
    dst_addr: str = Constant.DEFAULT_INVALID_VALUE
    duration_estimated: int = Constant.DEFAULT_INVALID_VALUE
    group_name: str = Constant.NA
    group_name_id: str = Constant.NA
    link_type: str = Constant.NA
    local_rank: int = Constant.DEFAULT_INVALID_VALUE
    notify_id: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    op_type: str = Constant.NA
    plane_id: int = Constant.DEFAULT_INVALID_VALUE
    rank_size: int = Constant.DEFAULT_INVALID_VALUE
    rdma_type: str = Constant.NA
    remote_rank: int = Constant.DEFAULT_INVALID_VALUE
    role: str = Constant.NA
    data_size: int = Constant.DEFAULT_INVALID_VALUE
    src_addr: str = Constant.DEFAULT_INVALID_VALUE
    stage: str = Constant.DEFAULT_INVALID_VALUE
    thread_id: int = Constant.DEFAULT_INVALID_VALUE
    transport_type: str = Constant.NA
    work_flow_mode: str = Constant.NA
