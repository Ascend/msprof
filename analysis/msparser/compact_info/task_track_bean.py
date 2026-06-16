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
from enum import Enum

from msparser.compact_info.compact_info_bean import CompactInfoBean


class DevType(Enum):
    NPU = 0
    DPU = 1


class TaskTrackBean(CompactInfoBean):
    """
    task track bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._dev_type = (data[6] >> 12) & 0xF
        self._device_id = data[6] & 0xFFF
        self._stream_id = data[7]
        self._task_id = data[8]
        self._batch_id = data[9]
        self._task_type = data[10]
        self._kernel_name = data[11]

    @property
    def is_dpu(self: any) -> bool:
        """
        Used to distinguish the device type(DPU:1/NPU:0) of tasks.
        """
        return self._dev_type == DevType.DPU.value

    @property
    def device_id(self: any) -> int:
        """
        task track device_id
        """
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        """
        task track stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        task track task_id
        """
        return self._task_id

    @property
    def batch_id(self: any) -> int:
        """
        task track batch_id
        """
        return self._batch_id

    @property
    def task_type(self: any) -> str:
        """
        task track task_type
        """
        return str(self._task_type)

    @property
    def kernel_name(self: any) -> str:
        """
        task track kernel_name
        """
        return str(self._kernel_name)

    @batch_id.setter
    def batch_id(self: any, batch_id) -> None:
        """
        task track batch_id
        """
        self._batch_id = batch_id


class TaskTrackChip6Bean(TaskTrackBean):
    """
    task track bean for chip v6
    task id use 32 bit, batch id is always 0
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._task_id = data[9] << 16 | data[8]
        self._batch_id = 0


class DPUTaskTrackBean(CompactInfoBean):
    """
    dpu start log bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._dev_type = (data[6] >> 12) & 0xF
        self._device_id = data[6] & 0xFFF
        self._stream_id = data[7]
        self._task_id = data[8]
        self._task_type = data[9]
        self._start_time = data[11]

    @property
    def device_id(self: any) -> int:
        """
        dpu task device_id
        """
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        """
        dpu task stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        dpu task task_id
        """
        return self._task_id

    @property
    def task_type(self: any) -> str:
        """
        dpu task task_type
        """
        return str(self._task_type)

    @property
    def start_time(self: any) -> int:
        """
        dpu task start_time
        """
        return self._start_time
