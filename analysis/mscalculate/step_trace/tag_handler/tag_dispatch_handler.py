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

import collections

from common_func.step_trace_constant import StepTraceConstant
from mscalculate.interface.step_trace_tag_handler import StepTraceTagHandler
from mscalculate.step_trace.tag_handler.iter_info_handler import AllReduceStreamHandler
from mscalculate.step_trace.tag_handler.iter_info_handler import GetNextTagHandler
from mscalculate.step_trace.tag_handler.iter_info_handler import TrainingTraceTagHandler
from mscalculate.step_trace.tag_handler.state_machine.index_tracer import IndexTracker


class DispatchModelHandler(StepTraceTagHandler):
    """
    dispatch record to handler according to model id
    """

    def __init__(self: any) -> None:
        self.next_handler = None
        self.next_handler_group = collections.defaultdict(DispatchIndexHandler)
        self.collect_data = {}

    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        model_id = record.get(StepTraceConstant.MODEL_ID)
        self.set_next(model_id)
        self.next_handler.receive_record(record)

    def get_data(self: any) -> dict:
        """
        return data of this handler
        :return: dict
        """
        for model_id, next_handler in self.next_handler_group.items():
            if next_handler.get_data():
                self.collect_data[model_id] = next_handler.get_data()

        return self.collect_data

    def set_next(self: any, model_id: int) -> None:
        """
        set next handler to receive record
        :param model_id: set next handler according to model id
        :return: bool
        """
        self.next_handler = self.next_handler_group[model_id]


class DispatchIndexHandler(StepTraceTagHandler):
    """
    dispatch record to handler according to index id
    """

    def __init__(self: any) -> None:
        self.next_handler = None
        self.next_handler_group = collections.defaultdict(DispatchIterInfoHandler)
        self.collect_data = collections.defaultdict(dict)

        self.state_machine = IndexTracker(self)

    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        self.process_record(record)
        index_id = self.state_machine.get_index()
        self.set_next(index_id)
        self.next_handler.receive_record(record)

    def get_data(self: any) -> dict:
        """
        return data of this handler
        :return: dict
        """
        index_id = self.state_machine.get_index()

        if index_id in self.collect_data and (
            not (
                self.collect_data.get(StepTraceConstant.STEP_START)
                and self.collect_data.get(StepTraceConstant.STEP_END)
            )
        ):
            self.collect_data.pop(index_id)

        for index, collect_datum in self.collect_data.items():
            collect_datum.update(self.next_handler_group.get(index).get_data())
        return self.collect_data

    def set_next(self: any, index_id: int) -> None:
        """
        set next handler to receive record
        :param index_id: set next handler according to index id
        :return: bool
        """
        self.next_handler = self.next_handler_group[index_id]

    def process_record(self: any, record: dict) -> None:
        """
        get iter start, iter end, index_id from record
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        self.state_machine.process_record(record)


class DispatchIterInfoHandler(StepTraceTagHandler):
    """
    dispatch record to handler according to tag id
    """

    def __init__(self: any) -> None:
        self.next_handler = None
        self.next_handler_group = {
            StepTraceConstant.ALL_REDUCE: AllReduceStreamHandler(),
            StepTraceConstant.TRAINING_TRACE: TrainingTraceTagHandler(),
            StepTraceConstant.GET_NEXT: GetNextTagHandler(),
        }
        self.collect_data = {}

    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        if self.set_next(record.get(StepTraceConstant.TAG_ID)):
            self.next_handler.receive_record(record)

    def get_data(self: any) -> dict:
        """
        return data of this handler
        :return: dict
        """
        for key, next_handler in self.next_handler_group.items():
            self.collect_data[key] = next_handler.get_data()

        return self.collect_data

    def set_next(self: any, tag_id: int) -> bool:
        """
        set next handler to receive record
        :param tag_id: set next handler according to tag id
        :return: bool
        """
        if tag_id >= StepTraceConstant.GET_NEXT_START_TAG:
            self.next_handler = self.next_handler_group.get(StepTraceConstant.GET_NEXT)
            return True

        if tag_id >= StepTraceConstant.ALL_REDUCE_START:
            self.next_handler = self.next_handler_group.get(StepTraceConstant.ALL_REDUCE)
            return True

        if tag_id in (StepTraceConstant.FP_TAG, StepTraceConstant.BP_TAG):
            self.next_handler = self.next_handler_group.get(StepTraceConstant.TRAINING_TRACE)
            return True

        return False

    def clear(self: any) -> None:
        """
        clear next handler
        :return: void
        """
        for next_handler in self.next_handler_group.values():
            next_handler.clear()
