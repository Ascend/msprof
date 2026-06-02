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

from collections import namedtuple
from dataclasses import dataclass

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class StepTraceOriginDto(metaclass=InstanceCheckMeta):
    """
    step trace origin DATA dto
    """

    index_id: int = None
    model_id: int = None
    stream_id: int = None
    tag_id: int = None
    task_id: int = None
    timestamp: float = None


@dataclass
class StepTraceDto(metaclass=InstanceCheckMeta):
    """
    step trace dto
    """

    index_id: int = None
    iter_id: int = None
    model_id: int = None
    step_end: int = None
    step_start: int = None


@dataclass
class TrainingTraceDto(metaclass=InstanceCheckMeta):
    """
    Training trace dto
    """

    bp_end: float = None
    data_aug_bound: str = None
    device_id: int = None
    fp_bp_time: float = None
    fp_start: float = None
    grad_refresh_bound: str = None
    iteration_end: float = None
    iteration_id: int = None
    iteration_time: float = None
    model_id: int = None


@dataclass
class MsproftxMarkDto(metaclass=InstanceCheckMeta):
    """
    msprofts ex mark dto
    """

    index_id: int = 0
    timestamp: int = 0
    stream_id: int = 0
    task_id: int = 0
    tag_id: int = 0


Iteration = namedtuple("Iteration", ["model_id", "iteration_id", "iteration_count"])


class IterationRange(Iteration):
    """
    iteration range for model execute.
    """

    MAX_ITERATION_COUNT = 5

    def __repr__(self):
        if self._is_compatibility_required():
            return f'{self.iteration_id}'
        return f'{self.iteration_start}_{self.iteration_end}'

    @property
    def iteration_start(self):
        return self.iteration_id

    @property
    def iteration_end(self):
        return self.iteration_id + self.iteration_count - NumberConstant.DEFAULT_ITER_COUNT

    def get_iteration_range(self):
        return self.iteration_start, self.iteration_end

    def _is_compatibility_required(self):
        return NumberConstant.DEFAULT_ITER_COUNT == self.iteration_count
