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
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TaskTrackDto(metaclass=InstanceCheckMeta):
    batch_id: int = None
    data_len: int = None
    device_id: int = None
    level: str = None
    stream_id: int = None
    struct_type: str = None
    task_id: int = None
    task_type: str = None
    thread_id: int = None
    timestamp: float = None
    kernel_name: str = None
    model_id: int = None


@dataclass
class TaskTrackV2Dto(metaclass=InstanceCheckMeta):
    device_id: int = None
    stream_id: int = None
    task_id: int = None
    batch_id: int = None
    model_id: int = None
