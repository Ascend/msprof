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
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class OriginMissionDto(metaclass=InstanceCheckMeta):
    """
    Dto for ccu Mission data
    """

    stream_id: int = None
    task_id: int = None
    lp_instr_id: int = None
    setckebit_instr_id: int = None
    rel_id: int = None
    start_time: float = None
    end_time: float = None
    time_type: str = None
    lp_start_time: float = None
    lp_end_time: float = None
    setckebit_start_time: float = None
    rel_end_time: float = None


@dataclass
class OriginSetCKBV62Dto(metaclass=InstanceCheckMeta):
    """
    Dto for chip v6_2 SetCKB mission data
    """

    task_id: int = None
    setckb_instr_id: int = None
    loop_id: int = None
    loop_cnt: int = None
    setckb_ckbinfo: int = None
    setckb_syscnt: int = None
    udf_profiling_data: str = None
    udf_flag: int = None
    syscnt_type: int = None
    lp_flg: int = None
    die_id: int = None


@dataclass
class OriginLoopGroupV62Dto(metaclass=InstanceCheckMeta):
    """
    Dto for chip v6_2 LoopGroup mission data
    """

    task_id: int = None
    loopgroup_instr_id: int = None
    loopgroup_start_syscnt: int = None
    loopgroup_stop_syscnt: int = None
    die_id: int = None
