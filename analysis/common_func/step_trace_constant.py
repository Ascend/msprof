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

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant


class StepTraceConstant:
    """
    Constant for Step trace
    """

    MODEL_START_TAG = 0
    MODEL_END_TAG = 1
    FP_TAG = 2
    BP_TAG = 3
    ITER_END_TAG = 4
    ALL_REDUCE_START = 10000
    GET_NEXT_START_TAG = 20000
    STEP_START_TAG = 60000
    STEP_END_TAG = 60001

    STEP_START = "step_start"
    STEP_END = "step_end"
    MODEL_ID = "model_id"
    STREAM_ID = "stream_id"
    ALL_REDUCE = "all_reduce"
    REDUCE_START = "reduce_start"
    REDUCE_END = "reduce_end"
    TRAINING_TRACE = "training_trace"
    TAG_ID = "tag_id"
    FORWARD_PROPAGATION = "fp"
    BACK_PROPAGATION = "bp"
    TIME_STAMP = "timestamp"
    ITER_ID = "iter_id"
    INDEX_ID = "index_id"
    GET_NEXT = "get_next"

    ITER_TIME = "iter_time"
    FORWARD_TO_BACK = "fp_bp"
    ITERATION_REFRESH = "iteration_refresh"
    DATA_AUGMENTATION = "data_aug"

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return StepTraceConstant.__name__

    @staticmethod
    def syscnt_to_micro() -> int:
        """
        syscnt to micro multiplication factor
        :return: int
        """
        return NumberConstant.MICRO_SECOND / InfoConfReader().get_freq(StrConstant.HWTS)
