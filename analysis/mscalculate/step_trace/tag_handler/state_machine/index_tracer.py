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

from common_func.step_trace_constant import StepTraceConstant
from mscalculate.step_trace.tag_handler.state_machine.iter_state import IterEnd
from mscalculate.step_trace.tag_handler.state_machine.iter_state import ModelEnd
from mscalculate.step_trace.tag_handler.state_machine.iter_state import ModelStart


class IndexTracker:
    """
    track index according to tag of record
    """

    def __init__(self: any, handler: any) -> None:
        self.handler = handler
        self.next_handler_group = handler.next_handler_group
        self.collect_data = handler.collect_data

        self.step_state = {
            StepTraceConstant.MODEL_START_TAG: ModelStart(self),
            StepTraceConstant.MODEL_END_TAG: ModelEnd(self),
            StepTraceConstant.ITER_END_TAG: IterEnd(self),
        }
        self.cur_state = self.step_state.get(StepTraceConstant.MODEL_END_TAG)
        self.index_id = 1

    def process_record(self: any, record: dict) -> None:
        """
        get iter start, iter end, index_id from record
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        self.cur_state.process_record(record)

    def get_index(self: any) -> int:
        """
        get index
        :return: int
        """
        return self.index_id
