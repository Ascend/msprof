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
from collections import defaultdict

from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import OpAnalysisType
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.section_calculator import SectionCalculator
from msparser.cluster.meta_parser import HcclAnalysisTool
from msparser.cluster.meta_parser import MetaParser
from profiling_bean.db_dto.time_section_dto import TimeSectionDto

_TimeSection = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TimeSectionDto, [])


class CommunicationParser(MetaParser):
    """
    cluster communication data parser
    """

    def __init__(self: any, events_data) -> None:
        self.op_events_dict = events_data
        self.op_info = {}

    @staticmethod
    def combine_size_distribution(part_dist_dict: dict, total_dist_dict: dict):
        for size, size_info in part_dist_dict.items():
            total_dist_dict[size][0] += size_info[0]
            total_dist_dict[size][1] += size_info[1]

    @staticmethod
    def combine_ops_time_info(part_dict: dict, total_dict: dict):
        no_accumulative_list = [
            OpAnalysisType.WAIT_TIME_RATIO,
            OpAnalysisType.SYNCHRONIZATION_TIME_RATIO,
            OpAnalysisType.START_TIME,
        ]
        # first level combine
        for key, value in part_dict.items():
            if key not in no_accumulative_list:
                total_dict[key] += value
        # second level combine
        HcclAnalysisTool.update_time_ratio(total_dict, StrConstant.TOTAL)

    @staticmethod
    def is_transit_sdma_event(event) -> bool:
        # if true, do not consider local copy
        return (
            event.hccl_name in StrConstant.SDMA_TRANSIT_ITEMS
            and event.transport_type == StrConstant.SDMA
            and event.link_type != StrConstant.ON_CHIP
        )

    @staticmethod
    def is_transit_ub_event(event) -> bool:
        return (
            event.hccl_name in StrConstant.UB_TRANSIT_ITEMS
            and event.transport_type in StrConstant.UB_TYPES
            and event.link_type in StrConstant.UB_TYPES
        )

    @staticmethod
    def get_communication_bandwidth_info_type(event):
        """
        只适用于transport_type为SDMA且event.name为"Memcpy"，"Reduce_Inline"，
        对应communication.json里面的"Communication Bandwidth Info"的key值，目前有6种：HCCS，PCIE，SIO，SDMA，RDMA，UB，
        其中，SDMA的数据为PCIE, HCCS, SIO的和；UB单独统计，不并入SDMA
        """
        if event.link_type == StrConstant.HCCS_SW:
            return StrConstant.HCCS  # HCCS_SW, 特殊的HCCS
        elif event.link_type in [StrConstant.PCIE, StrConstant.HCCS, StrConstant.SIO]:
            return event.link_type
        else:  # 如果link_type上报了RESERVED或者出现INVALID_TYPE，归为SDMA
            return StrConstant.SDMA

    def run(self: any) -> dict:
        self.parse()
        self.combine()
        return self.op_info

    def parse(self):
        for hccl_name, op_events in self.op_events_dict.items():
            self.parse_ops(op_events, hccl_name)
        if not self.op_info:
            logging.error("Fail to get op_info in Communication Parser")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    def parse_ops(self: any, op_events: dict, hccl_name: str) -> None:
        """
        time and link info parser for every hccl operators
        """
        self.op_info[hccl_name] = {}
        for rank_id, bundle in op_events.items():
            self.op_info.get(hccl_name).setdefault(rank_id, {})
            if not bundle.tasks:
                logging.error("Fail to get no.%s rank events info, communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            logging.info("Start to get no.%s rank events info", str(rank_id))
            self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_TIME_INFO] = self.op_time_parser(
                bundle.tasks, bundle.op_name, bundle.end - bundle.start, bundle.start
            )
            self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_TIME_INFO][OpAnalysisType.START_TIME] = float(
                InfoConfReader().trans_into_local_time(bundle.start)
            )
            # choose all stream for Bandwidth analysis parser
            self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_BANDWIDTH_INFO] = self.op_bandwidth_parser(
                bundle.tasks, bundle.op_name
            )

    def combine(self):
        """
        conclude all hccl ops to 'total ops'
        """
        self.op_info[StrConstant.TOTAL] = {}
        for hccl_name, hccl_dict in self.op_info.items():
            if hccl_name == StrConstant.TOTAL:
                continue
            for rank_id, rank_dict in hccl_dict.items():
                if rank_id not in self.op_info[StrConstant.TOTAL]:
                    self.op_info[StrConstant.TOTAL][rank_id] = {}
                self.combine_ops_info(rank_dict, self.op_info[StrConstant.TOTAL][rank_id])

    def combine_ops_info(self, rank_dict: dict, total_ops_dict: dict):
        for com_info, com_info_dict in rank_dict.items():
            if com_info == StrConstant.COMMUNICATION_TIME_INFO:
                if com_info not in total_ops_dict:
                    # get public variables from OpAnalysisType
                    values = [value for key, value in OpAnalysisType.__dict__.items() if '__' not in key]
                    total_ops_dict[com_info] = HcclAnalysisTool.init_dict(values)
                self.combine_ops_time_info(com_info_dict, total_ops_dict[com_info])
            if com_info == StrConstant.COMMUNICATION_BANDWIDTH_INFO:
                if com_info not in total_ops_dict:
                    total_ops_dict[com_info] = HcclAnalysisTool.init_bandwidth_dict()
                self.combine_ops_bandwidth_info(com_info_dict, total_ops_dict[com_info])

    def combine_ops_bandwidth_info(self: any, part_dict: dict, total_dict: dict) -> None:
        add_list = [OpBandWidthType.TRANSIT_TIME_MS, OpBandWidthType.TRANSIT_SIZE_MB]
        dict_list = [OpBandWidthType.SIZE_DISTRIBUTION]
        # first level combine
        for transport_type, part_transport_dict in part_dict.items():
            for bandwidth_msg, value in part_transport_dict.items():
                if bandwidth_msg in add_list:
                    total_dict[transport_type][bandwidth_msg] += value
                if transport_type != StrConstant.SDMA and bandwidth_msg in dict_list:
                    self.combine_size_distribution(value, total_dict[transport_type][bandwidth_msg])
        # second level combine
        for transport_type in StrConstant.TRANSIT_TYPE:
            if transport_type == StrConstant.SDMA:
                if total_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS] != 0:
                    total_dict[StrConstant.SDMA][OpBandWidthType.BANDWIDTH_GB_S] = round(
                        (total_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_SIZE_MB] / NumberConstant.MB_TO_GB)
                        / (
                            total_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS]
                            / NumberConstant.CONVERSION_TIME
                        ),
                        4,
                    )
            else:
                HcclAnalysisTool.analyze_bandwidth_info(total_dict, transport_type)

    def op_time_parser(self: any, events: list, op_name: str, duration: int, window_start=None) -> dict:
        """
        Parse communication time by a wall-clock one-drop partition of the master-stream slices:
        - any instant covered by a transit slice (SDMA/UB memcpy, RDMA payload span) counts as transit;
        - otherwise an instant covered by a Notify_Wait counts as wait;
        - wait before the first transit is also reported separately as synchronization;
        - idle is elapse minus transit and wait, so transit + wait + idle == elapse.
        A task is attributed to this op when its end time falls inside the op window, so a busy slice
        may start before the window opens; only the part inside the window counts for this op.
        """
        values = [value for key, value in OpAnalysisType.__dict__.items() if '__' not in key]
        op_time_dict = HcclAnalysisTool.init_dict(values)
        master_events = [event for event in events if event.is_master == 1]
        if not master_events:
            logging.error("Fail to get master events info, communication parser is interrupted")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        rdma_transit_op_num = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        if not HcclAnalysisTool.is_send_or_recv_op(op_name):
            rdma_transit_op_num = NumberConstant.RDMA_WITH_BARRIER_TASK_NUM
        transit_sections = []
        wait_sections = []
        task_dict = defaultdict(list)
        for task in master_events:
            task_dict[task.plane_id].append(task)
        for plane_tasks in task_dict.values():
            # keep the plane's original (logical) task order: RDMA payload-group detection depends on the
            # adjacency of consecutive payload tasks, so a timestamp reorder must not be applied here
            plane_transit, plane_wait = self._collect_plane_time_sections(plane_tasks, op_name, rdma_transit_op_num)
            transit_sections.extend(plane_transit)
            wait_sections.extend(plane_wait)
        elapse_ms = duration / NumberConstant.NS_TO_MS
        # 生产侧按"任务结束时间落在 op 窗口内即归属该 op"上报，wait/transit 切片可能开始于窗口之外。
        # busy 只统计落在 [window_start, window_end] 内的部分（窗外部分归相邻区间/前序 op），
        # 保证 transit + wait + idle == elapse 且 idle >= 0。无窗口信息时不裁剪，保持原行为。
        if window_start is not None:
            window_end = window_start + duration
            transit_sections = self._clip_sections_to_window(transit_sections, window_start, window_end)
            wait_sections = self._clip_sections_to_window(wait_sections, window_start, window_end)
        merged_transit = SectionCalculator.merge_continuous_intervals(transit_sections)
        merged_wait = SectionCalculator.merge_continuous_intervals(wait_sections)
        op_time_dict[OpAnalysisType.TRANSIT_TIME] = self._sections_duration_ms(merged_transit)
        # one-drop rule: each wall-clock instant is owned by exactly one category. When any plane is in
        # transit (memcpy) at an instant, that instant counts as transit, so the wait time is the union of
        # Notify_Wait intervals with the transit-covered parts removed.
        effective_wait = self._subtract_sections(merged_wait, merged_transit)
        op_time_dict[OpAnalysisType.WAIT_TIME] = self._sections_duration_ms(effective_wait)
        if merged_transit:
            sync_sections = self._clip_sections_before(effective_wait, merged_transit[0].start_time)
        else:
            sync_sections = effective_wait
        op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME] = self._sections_duration_ms(sync_sections)
        op_time_dict[OpAnalysisType.ELAPSE_TIME] = elapse_ms
        busy_union_ms = self._sections_duration_ms(
            SectionCalculator.merge_continuous_intervals(transit_sections + wait_sections)
        )
        op_time_dict[OpAnalysisType.IDLE_TIME] = elapse_ms - busy_union_ms
        HcclAnalysisTool.update_time_ratio(op_time_dict, op_name)
        return op_time_dict

    def _collect_plane_time_sections(self, plane_tasks: list, op_name: str, rdma_transit_op_num: int) -> tuple:
        transit_sections = []
        wait_sections = []
        idx = 0
        while idx < len(plane_tasks):
            event = plane_tasks[idx]
            if CommunicationParser.is_transit_sdma_event(event) or CommunicationParser.is_transit_ub_event(event):
                transit_sections.append(self._make_event_time_section(event))
            if event.rdma_type == StrConstant.RDMA_SEND_PAYLOAD:
                payload_cnt = HcclAnalysisTool.find_consecutive_payload_tasks_count(plane_tasks, idx)
                rdma_transit_result = HcclAnalysisTool.calculate_consecutive_payload_tasks_info(
                    plane_tasks, idx, payload_cnt, rdma_transit_op_num, op_name
                )
                if not rdma_transit_result:
                    idx += payload_cnt
                    continue
                last_event = plane_tasks[idx + payload_cnt + rdma_transit_op_num - 2]
                span_section = self._make_span_time_section(event, last_event)
                transit_sections.append(span_section)
                idx += rdma_transit_op_num + payload_cnt - 1
                continue
            if event.hccl_name == StrConstant.NOTIFY_WAIT:
                wait_sections.append(self._make_event_time_section(event))
            idx += 1
        return transit_sections, wait_sections

    @staticmethod
    def _make_event_time_section(event):
        start_time = HcclAnalysisTool.get_value(event.timestamp, "timestamp")
        return _TimeSection(
            start_time=start_time,
            end_time=start_time + HcclAnalysisTool.get_value(event.duration, "duration"),
        )

    @staticmethod
    def _make_span_time_section(first_event, last_event):
        start_time = HcclAnalysisTool.get_value(first_event.timestamp, "timestamp")
        end_time = HcclAnalysisTool.get_value(last_event.timestamp, "timestamp") + HcclAnalysisTool.get_value(
            last_event.duration, "duration"
        )
        return _TimeSection(start_time=start_time, end_time=end_time)

    @staticmethod
    def _sections_duration_ms(sections: list) -> float:
        return sum((item.end_time - item.start_time) for item in sections) / NumberConstant.NS_TO_MS

    @staticmethod
    def _clip_sections_before(sections: list, bound) -> list:
        clipped = []
        for section in sections:
            if section.start_time >= bound:
                continue
            end_time = min(section.end_time, bound)
            if end_time > section.start_time:
                clipped.append(section.replace(end_time=end_time))
        return clipped

    @staticmethod
    def _clip_sections_to_window(sections: list, window_start, window_end) -> list:
        """Keep only the part of each section inside [window_start, window_end]; drop empty ones."""
        clipped = []
        for section in sections:
            start_time = max(section.start_time, window_start)
            end_time = min(section.end_time, window_end)
            if end_time > start_time:
                clipped.append(section.replace(start_time=start_time, end_time=end_time))
        return clipped

    @staticmethod
    def _subtract_sections(base_sections: list, remove_sections: list) -> list:
        """Keep the parts of base_sections not covered by remove_sections.

        Both inputs must be merged (sorted, non-overlapping) continuous intervals, in nanoseconds.
        Two pointers only move forward, so the scan is O(n + m).
        """
        if not remove_sections:
            return base_sections
        remaining = []
        remove_idx = 0
        n_remove = len(remove_sections)
        for section in base_sections:
            cursor = section.start_time
            while remove_idx < n_remove and remove_sections[remove_idx].end_time <= cursor:
                remove_idx += 1
            scan_idx = remove_idx
            while scan_idx < n_remove and remove_sections[scan_idx].start_time < section.end_time:
                remove_section = remove_sections[scan_idx]
                if remove_section.start_time > cursor:
                    remaining.append(
                        section.replace(start_time=cursor, end_time=min(remove_section.start_time, section.end_time))
                    )
                cursor = max(cursor, remove_section.end_time)
                if cursor >= section.end_time:
                    break
                scan_idx += 1
            if cursor < section.end_time:
                remaining.append(section.replace(start_time=cursor, end_time=section.end_time))
        return remaining

    def op_bandwidth_parser(self, events: list, op_name: str) -> dict:
        """
        Bandwidth info parser
        """
        op_bandwidth_dict = HcclAnalysisTool.init_bandwidth_dict()
        idx = 0
        rdma_transit_op_num = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        if not HcclAnalysisTool.is_send_or_recv_op(op_name):
            rdma_transit_op_num = NumberConstant.RDMA_WITH_BARRIER_TASK_NUM
        task_dict = defaultdict(list)
        for task in events:
            task_dict[task.plane_id].append(task)
        for plane_id_tasks in task_dict.values():
            idx = 0
            while idx < len(plane_id_tasks):
                event = plane_id_tasks[idx]
                if CommunicationParser.is_transit_sdma_event(event):
                    self._calculate_sdma_bw(op_bandwidth_dict, event)
                if CommunicationParser.is_transit_ub_event(event):
                    self._calculate_ub_bw(op_bandwidth_dict, event)
                if event.rdma_type == StrConstant.RDMA_SEND_PAYLOAD:
                    idx = self._calculate_rdma_bw(op_bandwidth_dict, plane_id_tasks, idx, rdma_transit_op_num, op_name)
                    continue
                idx += 1
        for transport_type in StrConstant.TRANSIT_TYPE:
            if transport_type == StrConstant.SDMA:
                HcclAnalysisTool.combine_sdma_info(op_bandwidth_dict)
            else:
                HcclAnalysisTool.analyze_bandwidth_info(op_bandwidth_dict, transport_type)
        return op_bandwidth_dict

    def _calculate_sdma_bw(self, op_bandwidth_dict, event):
        bandwidth_info_type = self.get_communication_bandwidth_info_type(event)
        HcclAnalysisTool.update_bandwidth_record(
            op_bandwidth_dict,
            bandwidth_info_type,
            HcclAnalysisTool.get_value(event.size, "size") / NumberConstant.BYTES_TO_MB,
            HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS,
        )

    def _calculate_ub_bw(self, op_bandwidth_dict, event):
        HcclAnalysisTool.update_bandwidth_record(
            op_bandwidth_dict,
            StrConstant.UB,
            HcclAnalysisTool.get_value(event.size, "size") / NumberConstant.BYTES_TO_MB,
            HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS,
        )

    def _calculate_rdma_bw(self, op_bandwidth_dict, plane_id_tasks, idx, rdma_transit_op_num, op_name):
        event = plane_id_tasks[idx]
        payload_cnt = HcclAnalysisTool.find_consecutive_payload_tasks_count(plane_id_tasks, idx)
        rdma_transit_result = HcclAnalysisTool.calculate_consecutive_payload_tasks_info(
            plane_id_tasks, idx, payload_cnt, rdma_transit_op_num, op_name
        )
        if not rdma_transit_result:
            idx += payload_cnt
            return idx
        HcclAnalysisTool.update_bandwidth_record(
            op_bandwidth_dict, event.transport_type, rdma_transit_result[1], rdma_transit_result[0]
        )
        idx += rdma_transit_op_num + payload_cnt - 1
        return idx
