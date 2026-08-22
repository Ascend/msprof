# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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


@dataclass
class _HcclOpBase:
    """HcclOps / KfcOps 公共基类，统一定义共享字段及其默认值"""

    model_id: int = Constant.DEFAULT_INVALID_VALUE
    index_id: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    op_type: str = Constant.NA
    start: int = Constant.DEFAULT_VALUE
    end: int = Constant.DEFAULT_VALUE
    connection_id: int = Constant.DEFAULT_INVALID_VALUE
    relay: int = Constant.DEFAULT_INVALID_VALUE
    retry: int = Constant.DEFAULT_INVALID_VALUE
    data_type: str = Constant.NA
    alg_type: str = Constant.NA
    count: int = Constant.DEFAULT_INVALID_VALUE
    group_name: str = Constant.NA
    kfc_connection_ids: str = str(Constant.DEFAULT_INVALID_VALUE)  # 用于存储从hcclOp取到的数据
    kfc_connection_id: int = Constant.DEFAULT_INVALID_VALUE  # 用于存储处理后的kfcConnectionId
    rank_size: int = Constant.DEFAULT_INVALID_VALUE
    iter_id: int = Constant.DEFAULT_VALUE


@dataclass
class KfcOps(_HcclOpBase):
    # for aicpu task identification
    stream_id: int = Constant.DEFAULT_VALUE
    task_id: int = Constant.DEFAULT_VALUE
    context_id: int = Constant.DEFAULT_VALUE
    batch_id: int = Constant.DEFAULT_VALUE


@dataclass
class HcclOps(_HcclOpBase):
    thread_id: int = Constant.DEFAULT_INVALID_VALUE
    task_type: str = Constant.NA


@dataclass
class HcclTask:
    model_id: int = Constant.DEFAULT_INVALID_VALUE
    index_id: int = Constant.DEFAULT_INVALID_VALUE
    group_name: str = Constant.NA
    plane_id: int = Constant.DEFAULT_VALUE
    timestamp: int = Constant.DEFAULT_VALUE
    duration: int = Constant.DEFAULT_VALUE
    stream_id: int = Constant.DEFAULT_VALUE
    task_id: int = Constant.DEFAULT_VALUE
    context_id: int = Constant.DEFAULT_VALUE
    batch_id: int = Constant.DEFAULT_VALUE
    hccl_name: str = Constant.NA
    is_master: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    op_type: str = Constant.NA
    task_type: str = Constant.NA
    local_rank: int = Constant.DEFAULT_INVALID_VALUE
    remote_rank: int = Constant.DEFAULT_INVALID_VALUE
    transport_type: str = Constant.NA
    data_type: str = Constant.NA
    link_type: str = Constant.NA
    size: int = Constant.DEFAULT_INVALID_VALUE
    bandwidth: int = Constant.DEFAULT_INVALID_VALUE
    notify_id: int = Constant.DEFAULT_INVALID_VALUE
    rdma_type: str = Constant.NA
    thread_id: int = Constant.DEFAULT_INVALID_VALUE
    rank_size: int = Constant.DEFAULT_INVALID_VALUE
    op_id: int = Constant.DEFAULT_INVALID_VALUE
    iter_id: int = Constant.DEFAULT_VALUE


@dataclass
class ReportData:
    op_type: str = Constant.NA
    occurrences: int = Constant.DEFAULT_VALUE
    total_time: int = Constant.DEFAULT_VALUE
    min: int = Constant.DEFAULT_VALUE
    avg: int = Constant.DEFAULT_VALUE
    max: int = Constant.DEFAULT_VALUE
    ratio: float = Constant.DEFAULT_VALUE
