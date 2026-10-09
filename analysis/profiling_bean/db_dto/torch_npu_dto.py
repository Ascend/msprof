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
class TorchNpuDto(metaclass=InstanceCheckMeta):
    """
    Dto for relationship between torch op and npu kernel
    """

    acl_compile_time: int = None
    acl_end_time: int = None
    acl_start_time: int = None
    acl_tid: int = None
    batch_id: int = None
    context_id: int = None
    op_name: str = None
    stream_id: int = None
    task_id: int = None
    torch_op_pid: int = None
    torch_op_start_time: int = None
    torch_op_tid: int = None
