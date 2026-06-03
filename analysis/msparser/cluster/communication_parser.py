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
from msparser.cluster.meta_parser import HcclAnalysisTool
from msparser.cluster.meta_parser import MetaParser


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
    def get_communication_bandwidth_info_type(event):
        """
        只适用于transport_type为SDMA且event.name为"Memcpy"，"Reduce_Inline"，
        对应communication.json里面的"Communication Bandwidth Info"的key值，目前有5种：HCCS，PCIE，SIO，SDMA，RDMA，
        其中，SDMA的数据为PCIE, HCCS, SIO的和
        """
        if event.link_type == StrConstant.HCCS_SW:
            return StrConstant.HCCS  # HCCS_SW, 特殊的HCCS
        elif event.link_type in [StrConstant.PCIE, StrConstant.HCCS, StrConstant.SIO]:
            return event.link_type
        else:  # 如果link_type上报了RESERVED或者出现INVALID_TYPE，归为SDMA
            return StrConstant.SDMA

    @staticmethod
    def get_master_plane_id(events: list) -> int:
        """
        hccl data use master's plane_id for ffts+;
        Should be changed with hccl's algorithms!
        now judged by "is_master"
        """
        for event in events:
            if event.is_master == 1:
                return event.plane_id
        logging.error("Fail to get master events info, communication parser is interrupted")
        raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

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
        for rank_id in op_events:
            self.op_info.get(hccl_name).setdefault(rank_id, {})
            if not op_events.get(rank_id):
                logging.error("Fail to get no.%s rank events info, communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            events = op_events.get(rank_id)
            if events:
                logging.info("Start to get no.%s rank events info", str(rank_id))
                self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_TIME_INFO] = self.op_time_parser(events)
                self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_TIME_INFO][OpAnalysisType.START_TIME] = (
                    float(
                        InfoConfReader().trans_into_local_time(
                            min(
                                (event for event in events if event.is_master == 1), key=lambda x: x.timestamp
                            ).timestamp
                        )
                    )
                )
                # choose all stream for Bandwidth analysis parser
                self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_BANDWIDTH_INFO] = self.op_bandwidth_parser(
                    events
                )
            else:
                logging.error("Fail to get no.%s rank events info, communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

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
                        (
                            total_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_SIZE_MB]
                            / NumberConstant.COMMUNICATION_MB_to_GB
                        )
                        / (
                            total_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS]
                            / NumberConstant.CONVERSION_TIME
                        ),
                        4,
                    )
            else:
                HcclAnalysisTool.analyze_bandwidth_info(total_dict, transport_type)

    def op_time_parser(self, events: list) -> dict:
        """
        time info parser
        """
        # in case there exists keys that never use, init dict first
        values = [value for key, value in OpAnalysisType.__dict__.items() if '__' not in key]
        op_time_dict = HcclAnalysisTool.init_dict(values)
        wait_flag = True
        idx = 0
        # only choose master stream for op time analysis parser
        master_plane_id = CommunicationParser.get_master_plane_id(events)
        master_events = [event for event in events if event.plane_id == master_plane_id]
        if not master_events:
            logging.error("Fail to get master events info, communication parser is interrupted")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        op_name = master_events[0].op_name
        rdma_transit_op_num = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        if not HcclAnalysisTool.is_send_or_recv_op(master_events, idx):
            rdma_transit_op_num = NumberConstant.RDMA_WITH_BARRIER_TASK_NUM
        while idx < len(master_events):
            event = master_events[idx]
            if CommunicationParser.is_transit_sdma_event(event):
                wait_flag = False
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += (
                    HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS
                )
            if event.rdma_type == 'RDMA_SEND_PAYLOAD':
                payload_cnt = HcclAnalysisTool.find_consecutive_payload_tasks_count(master_events, idx)
                rdma_transit_result = HcclAnalysisTool.calculate_consecutive_payload_tasks_info(
                    master_events, idx, payload_cnt, rdma_transit_op_num
                )
                if not rdma_transit_result:
                    idx += payload_cnt
                    continue
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += rdma_transit_result[0]
                idx += rdma_transit_op_num + payload_cnt - 1
                wait_flag = False
                continue
            if event.hccl_name == StrConstant.NOTIFY_WAIT:
                wait_time = HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS
                if wait_flag:
                    op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME] += wait_time
                op_time_dict[OpAnalysisType.WAIT_TIME] += wait_time
            idx += 1
        latest_event = max(master_events, key=lambda x: x.timestamp + x.duration)
        earliest_event = min(master_events, key=lambda x: x.timestamp)
        op_time_dict[OpAnalysisType.ELAPSE_TIME] = (
            latest_event.timestamp + latest_event.duration - earliest_event.timestamp
        ) / NumberConstant.NS_TO_MS
        op_time_dict[OpAnalysisType.IDLE_TIME] = (
            op_time_dict[OpAnalysisType.ELAPSE_TIME]
            - op_time_dict[OpAnalysisType.TRANSIT_TIME]
            - op_time_dict[OpAnalysisType.WAIT_TIME]
        )
        HcclAnalysisTool.update_time_ratio(op_time_dict, op_name)
        return op_time_dict

    def op_bandwidth_parser(self, events: list) -> dict:
        """
        Bandwidth info parser
        """
        op_bandwidth_dict = HcclAnalysisTool.init_bandwidth_dict()
        idx = 0
        rdma_transit_op_num = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        if not HcclAnalysisTool.is_send_or_recv_op(events, idx):
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
                if event.rdma_type == 'RDMA_SEND_PAYLOAD':
                    idx = self._calculate_rdma_bw(op_bandwidth_dict, plane_id_tasks, idx, rdma_transit_op_num)
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
            HcclAnalysisTool.get_value(event.size, "size") / NumberConstant.COMMUNICATION_B_to_MB,
            HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS,
        )

    def _calculate_rdma_bw(self, op_bandwidth_dict, plane_id_tasks, idx, rdma_transit_op_num):
        event = plane_id_tasks[idx]
        payload_cnt = HcclAnalysisTool.find_consecutive_payload_tasks_count(plane_id_tasks, idx)
        rdma_transit_result = HcclAnalysisTool.calculate_consecutive_payload_tasks_info(
            plane_id_tasks, idx, payload_cnt, rdma_transit_op_num
        )
        if not rdma_transit_result:
            idx += payload_cnt
            return idx
        HcclAnalysisTool.update_bandwidth_record(
            op_bandwidth_dict, event.transport_type, rdma_transit_result[1], rdma_transit_result[0]
        )
        idx += rdma_transit_op_num + payload_cnt - 1
        return idx
