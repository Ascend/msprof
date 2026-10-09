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
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TimeSectionDto(metaclass=InstanceCheckMeta):
    duration_time: int = Constant.DEFAULT_INVALID_VALUE
    end_time: int = Constant.DEFAULT_INVALID_VALUE
    index_id: int = Constant.DEFAULT_INVALID_VALUE
    model_id: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = None
    overlap_time: int = Constant.DEFAULT_INVALID_VALUE
    start_time: int = Constant.DEFAULT_INVALID_VALUE
    stream_id: int = Constant.DEFAULT_INVALID_VALUE
    task_id: int = Constant.DEFAULT_INVALID_VALUE
    task_type: str = None


class CommunicationTimeSection(TimeSectionDto):
    def __init__(self):
        super().__init__()
