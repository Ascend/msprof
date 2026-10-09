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
class MsprofTxDto(metaclass=InstanceCheckMeta):
    category: int = 0
    dur_time: int = 0
    end_time: int = 0
    event_type: int = 0
    message: str = Constant.NA
    message_type: int = 0
    payload_type: int = 0
    payload_value: int = 0
    pid: int = 0
    start_time: int = 0
    tid: int = 0


@dataclass
class MsprofTxExDto(metaclass=InstanceCheckMeta):
    pid: int = 0
    tid: int = 0
    event_type: str = Constant.NA
    start_time: int = 0
    dur_time: int = 0
    mark_id: int = 0
    domain: str = Constant.NA
    message: str = Constant.NA
