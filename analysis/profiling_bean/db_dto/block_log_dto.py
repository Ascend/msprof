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
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class BlockLogDto(metaclass=InstanceCheckMeta):
    """
    Dto for stars block log data
    """

    stream_id: int
    task_id: int
    block_id: int
    context_id: int
    start_time: int
    duration: float
    device_task_type: str
    core_type: int
    core_id: int
    batch_id: int = NumberConstant.DEFAULT_BATCH_ID
    subtask_id: int = NumberConstant.DEFAULT_GE_CONTEXT_ID
