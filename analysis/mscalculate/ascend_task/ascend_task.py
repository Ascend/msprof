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

from dataclasses import dataclass
from common_func.ms_constant.number_constant import NumberConstant


@dataclass
class HostTask:
    """
    This class represents a runtime host task that adapts to all chips.
    """

    model_id: int
    index_id: int
    stream_id: int
    task_id: int
    context_id: int
    batch_id: int
    task_type: str
    kernel_name: str
    device_id: int
    host_timestamp: int
    connection_id: int


@dataclass
class DeviceTask:
    """
    This class represents a device task that adapts to all chips.
    """

    stream_id: int
    task_id: int
    context_id: int
    timestamp: int
    duration: int
    task_type: str
    batch_id: int = NumberConstant.DEFAULT_BATCH_ID


@dataclass
class TopDownTask:
    """
    this class represents an top-down task which
    contains complete ascend software and hardware info.
    The instance of this class represents the smallest
    schedulable unit running on a device.
    """

    model_id: int
    index_id: int
    stream_id: int
    task_id: int
    context_id: int
    batch_id: int
    start_time: int
    duration: int
    host_task_type: str
    device_task_type: str
    connection_id: int


@dataclass
class TaskTypeDto:
    """
    this class represents step trace task type task
    """

    timestamp: int
    stream_id: int
    task_id: int
    task_type: str
    task_state: int
    batch_id: int = 0
