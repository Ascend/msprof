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

import logging
from typing import Tuple

from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from msparser.cluster.meta_parser import MetaParser


class CriticalPathParser(MetaParser):
    """
    critical path parser
    """

    TS = 'ts'
    ES = 'es'
    DUR = 'dur'
    TID = 'tid'
    NAME = 'op_name'
    TASK_TYPE = 'task_type'
    # time interval(us)
    TIME_INTERVAL = 1

    def __init__(self, compute_op_events: list, hccl_op_events: dict) -> None:
        self.compute_op_events = compute_op_events
        self.hccl_op_events = hccl_op_events
        self.events = []

    @staticmethod
    def get_event_dict_by_pid_tid(timeline_data: list) -> dict:
        event_dict = {}
        for event in timeline_data:
            identify = f"{event.get(CriticalPathParser.TID)}_{event.get(CriticalPathParser.TASK_TYPE)}"
            if identify not in event_dict:
                event_dict[identify] = [event]
            else:
                event_dict[identify].append(event)
        return event_dict

    @staticmethod
    def get_pre_event_in_same_stream(cur_event: dict, event_dict: dict) -> dict:
        cur_identify = f"{cur_event.get(CriticalPathParser.TID)}_{cur_event.get(CriticalPathParser.TASK_TYPE)}"
        cur_stream_event = event_dict.get(cur_identify)
        sorted_cur_stream_event = sorted(
            cur_stream_event, key=lambda s: float(s.get(CriticalPathParser.TS)), reverse=True
        )
        cur_idx = len(sorted_cur_stream_event)
        for idx, event in enumerate(sorted_cur_stream_event):
            if event.get(CriticalPathParser.TS) == cur_event.get(CriticalPathParser.TS):
                cur_idx = idx
                break
        pre_idx = cur_idx + 1
        return sorted_cur_stream_event[pre_idx] if pre_idx < len(sorted_cur_stream_event) else {}

    @staticmethod
    def get_pre_event_in_different_stream(event_list: list, cur_event: dict) -> dict:
        pre_idx = None
        for idx, event in enumerate(event_list):
            if float(event.get(CriticalPathParser.ES)) <= cur_event.get(CriticalPathParser.TS) and event.get(
                CriticalPathParser.TS
            ) != cur_event.get(CriticalPathParser.TS):
                pre_idx = idx
                break
        return event_list[pre_idx] if pre_idx else {}

    @staticmethod
    def get_event_start_end_time(event: dict) -> tuple:
        event_start_time = float(event.get(CriticalPathParser.TS))
        event_end_time = float(event.get(CriticalPathParser.ES))
        return event_start_time, event_end_time

    @staticmethod
    def parse_op_list(op_list: list, topk_type: str) -> tuple:
        sorted_op_list = sorted(op_list, key=lambda s: float(s.get(topk_type)), reverse=True)
        op_num = len(sorted_op_list)
        op_time = sum(float(op.get(CriticalPathParser.DUR)) for op in sorted_op_list)
        return sorted_op_list, op_num, op_time

    @staticmethod
    def filter_method(op: dict) -> bool:
        filtered_op_names = ['Receive', 'Send', 'send', 'receive']
        return all(name not in op.get(CriticalPathParser.NAME) for name in filtered_op_names)

    @classmethod
    def event_type_analysis(cls, critical_path: list, top_k: int = 5) -> dict:
        top_type = "serial_time"
        op_dict = {Constant.TASK_TYPE_AI_CORE: [], Constant.TASK_TYPE_AI_CPU: [], Constant.TASK_TYPE_COMMUNICATION: []}
        for event in critical_path:
            op_task_type = event.get(CriticalPathParser.TASK_TYPE)
            if op_task_type in op_dict:
                op_dict.get(op_task_type).append(event)

        analysis_result = {}
        for op_type in op_dict:
            sorted_op_list, op_num, op_time = cls.parse_op_list(op_dict.get(op_type), top_type)

            # Filter out Receive and Send operators
            if op_type == Constant.TASK_TYPE_COMMUNICATION:
                topk_op = [op for i, op in zip(range(top_k), filter(cls.filter_method, sorted_op_list))]
            else:
                topk_op = sorted_op_list[0:top_k]

            analysis_result[op_type] = {"op_num": op_num, "op_time": op_time, "topk_op": topk_op}
        return analysis_result

    @classmethod
    def get_event_serial_parallel_time(cls, cur_event: dict, intersection_event_list: list) -> Tuple[float, float]:
        """Calculate the serial time and parallel time of an event."""
        sorted_intersection_event = sorted(
            intersection_event_list, key=lambda s: float(s[CriticalPathParser.TS]), reverse=False
        )
        individual_time_list = []
        while len(sorted_intersection_event) > 0:
            individual_event = sorted_intersection_event.pop(0)
            individual_event_ts, individual_event_te = cls.get_event_start_end_time(individual_event)
            removed_event_list = []
            for event in sorted_intersection_event:
                event_ts, event_te = cls.get_event_start_end_time(event)
                # Delete events hidden in individual_event time
                if event_te <= individual_event_te and event != individual_event and event not in removed_event_list:
                    removed_event_list.append(event)
                if event_ts < individual_event_te < event_te:
                    individual_event_te = event_te
                    removed_event_list.append(event)
                if event_ts >= individual_event_te:
                    break
            for event in removed_event_list:
                sorted_intersection_event.remove(event)
            individual_time_list.append([individual_event_ts, individual_event_te])
        parallel_time = 0.0
        cur_ts, cur_te = cls.get_event_start_end_time(cur_event)
        for time_record in individual_time_list:
            ts = cur_ts if time_record[0] <= cur_ts else time_record[0]
            te = cur_te if time_record[1] >= cur_te else time_record[1]
            parallel_time += te - ts if te - ts >= 0 else 0
        serial_time = float(cur_event.get(CriticalPathParser.DUR)) - parallel_time
        return serial_time, parallel_time

    def get_critical_path(self) -> list:
        """Critical Path Analysis"""
        sorted_event_by_start_time = sorted(
            self.events, key=lambda s: float(s.get(CriticalPathParser.TS)), reverse=False
        )
        sorted_event_by_end_time = sorted(self.events, key=lambda s: float(s.get(CriticalPathParser.ES)), reverse=True)
        first_event = sorted_event_by_start_time[0]
        last_event = sorted_event_by_end_time[0]
        critical_path_event_list = []
        cur_event = last_event

        event_dict = self.get_event_dict_by_pid_tid(self.events)
        while cur_event.get(CriticalPathParser.TS) != first_event.get(CriticalPathParser.TS):
            critical_path_event_list.append(cur_event)
            pre_event = self.get_pre_event_in_same_stream(cur_event, event_dict)

            if not pre_event:
                pre_event = self.get_pre_event_in_different_stream(sorted_event_by_end_time, cur_event)
            pre_event_end_time = float(pre_event.get(CriticalPathParser.ES))
            if float(cur_event.get(CriticalPathParser.TS)) - pre_event_end_time <= CriticalPathParser.TIME_INTERVAL:
                cur_event = pre_event
            else:
                pre_event = self.get_pre_event_in_different_stream(sorted_event_by_end_time, cur_event)
                cur_event = pre_event
            if not cur_event:
                break
            if cur_event.get(CriticalPathParser.TS) == first_event.get(CriticalPathParser.TS):
                critical_path_event_list.append(cur_event)
        sorted_critical_path = sorted(
            critical_path_event_list, key=lambda s: float(s.get(CriticalPathParser.TS)), reverse=False
        )
        return sorted_critical_path

    def get_time_intersection_event(self, cur_event: dict) -> list:
        """Find all events that have intersections with cur_event."""
        interval_event_list = []
        cur_ts, cur_te = self.get_event_start_end_time(cur_event)
        for event in self.events:
            event_ts, event_te = self.get_event_start_end_time(event)
            is_interaction = event != cur_event and event_ts < cur_te and event_te > cur_ts and event_ts < event_te
            if is_interaction:
                interval_event_list.append(event)
        return interval_event_list

    def event_execution_type_analysis(self, critical_path: list) -> list:
        """ "get execution type of event in critical path"""
        execution_type_analysis_result = []
        for event in critical_path:
            if event.get(CriticalPathParser.TASK_TYPE) != Constant.TASK_TYPE_COMMUNICATION:
                continue
            intersection_event_list = self.get_time_intersection_event(event)
            serial_time, parallel_time = self.get_event_serial_parallel_time(event, intersection_event_list)
            event["serial_time"] = serial_time
            event["parallel_time"] = parallel_time
            execution_type_analysis_result.append(event)
        return execution_type_analysis_result

    def run(self) -> tuple:
        self.parse_compute_ops()
        self.parse_hccl_ops()
        return self.parse()

    def parse_compute_ops(self) -> None:
        """Parse the timestamp of Ge Ops"""
        for compute_op_event in self.compute_op_events:
            self.events.append(
                {
                    CriticalPathParser.NAME: compute_op_event.op_name,
                    CriticalPathParser.TASK_TYPE: compute_op_event.task_type,
                    CriticalPathParser.TID: compute_op_event.stream_id,
                    CriticalPathParser.TS: compute_op_event.start_time / NumberConstant.CONVERSION_TIME,
                    CriticalPathParser.ES: compute_op_event.end_time / NumberConstant.CONVERSION_TIME,
                    CriticalPathParser.DUR: compute_op_event.duration_time / NumberConstant.CONVERSION_TIME,
                }
            )

    def parse_hccl_ops(self) -> None:
        """Parse the timestamp of Hccl Ops"""
        for hccl_op, bundle in self.hccl_op_events.items():
            hccl_op_dict = {
                CriticalPathParser.NAME: hccl_op,
                CriticalPathParser.TASK_TYPE: Constant.TASK_TYPE_COMMUNICATION,
                CriticalPathParser.TID: bundle.tasks[0].stream_id,
                CriticalPathParser.TS: bundle.start,
                CriticalPathParser.ES: bundle.end,
            }
            hccl_op_dict[CriticalPathParser.DUR] = format(
                hccl_op_dict[CriticalPathParser.ES] - hccl_op_dict[CriticalPathParser.TS], '.4f'
            )
            self.events.append(hccl_op_dict)

    def parse(self) -> tuple:
        """Get Top HCCL ops Through Critical Path Analysis"""
        logging.info(
            "Start to critical path analysis ! Total ops num: %d, hccl ops num: %d",
            len(self.events),
            len(self.hccl_op_events),
        )
        if not self.events:
            logging.error("Fail to get compute op and hccl op info, critical path parser is interrupted")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        critical_path_event = self.get_critical_path()
        event_execution_type_analysis_data = self.event_execution_type_analysis(critical_path_event)
        op_type_analysis = self.event_type_analysis(event_execution_type_analysis_data)
        hccl_result = op_type_analysis.get(Constant.TASK_TYPE_COMMUNICATION)
        logging.info(
            "With critical path analysis, total ops num: %d, hccl ops num: %d ",
            len(critical_path_event),
            hccl_result.get('op_num'),
        )

        top_communication_op = set()
        for idx, communication_op in enumerate(hccl_result.get("topk_op")):
            logging.info(
                "Top %d hccl op: %s serial time(us): %f op dur time: %s ",
                idx,
                communication_op.get(CriticalPathParser.NAME),
                communication_op.get('serial_time'),
                communication_op.get(CriticalPathParser.DUR),
            )
            top_communication_op.add(communication_op.get(CriticalPathParser.NAME))
        return tuple(top_communication_op)
