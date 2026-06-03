# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
from abc import ABC
from collections import OrderedDict
from typing import List

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.dpu.dpu_task_model import DPUTaskViewModel
from profiling_bean.db_dto.dpu_track_dto import DPUTrackDto
from viewer.interface.base_viewer import BaseViewer


class DPUViewer(BaseViewer, ABC):
    """
    class for get dpu data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {"dpu": DPUTaskViewModel}
        self.tid = InfoConfReader().get_json_tid_data()

    def _build_trace_data(self: any, track: DPUTrackDto, is_hccl: bool = False) -> tuple:
        start_time = InfoConfReader().trans_into_local_time(
            InfoConfReader().time_from_host_syscnt(track.start_time, NumberConstant.MICRO_SECOND),
            use_us=True,
            is_host=True,
        )
        # duration (us)
        duration = InfoConfReader().get_host_duration((track.end_time - track.start_time), NumberConstant.MICRO_SECOND)

        args = OrderedDict(
            [
                ('Thread Id', track.thread_id),
                ("Physic Stream Id", track.stream_id),
                ("Task Id", track.task_id),
            ]
        )

        if is_hccl:
            bandwidth = 0
            if duration > 0:
                bandwidth = track.data_size * NumberConstant.COMMUNICATION_B_to_GB / (duration * NumberConstant.US_TO_S)
            args.setdefault("OP Type", track.op_type)
            args.setdefault("AI CPU Device Id", track.npu_device_id)
            args.setdefault("AI CPU Task Id", track.aicpu_task_id)
            args.setdefault("Group Name", f"{track.group_name}({track.group_name_id})")
            args.setdefault("Plane Id", track.plane_id)
            args.setdefault("Notify Id", track.notify_id)
            args.setdefault("Duration Estimated(us)", track.duration_estimated)
            args.setdefault("Rank Size", track.rank_size)
            args.setdefault("Src Rank", track.local_rank)
            args.setdefault("Dst Rank", track.remote_rank)
            args.setdefault("Transport Type", track.transport_type)
            args.setdefault("Size(Byte)", track.data_size)
            args.setdefault("Bandwidth(GB/s)", bandwidth)
            args.setdefault("Data Type", track.data_type)
            args.setdefault("Link Type", track.link_type)
            args.setdefault("Rdma Type", track.rdma_type)
            args.setdefault("Role", track.role)
            args.setdefault("Ccl Tag", track.ccl_tag)
            args.setdefault("Work Flow Mode", track.work_flow_mode)
            args.setdefault("Stage", track.stage)
        else:
            args.setdefault("Task Type", track.task_type)

        return track.op_name, track.dpu_device_id, track.stream_id, start_time, duration, args

    def get_column_trace_data(self, task_track: List[DPUTrackDto], hccl_track: List[DPUTrackDto]) -> list:
        trace_data = []
        for track in task_track:
            trace_data.append(self._build_trace_data(track, is_hccl=False))
        for track in hccl_track:
            trace_data.append(self._build_trace_data(track, is_hccl=True))

        return trace_data

    def get_time_timeline_header(self: any, task_track: List[DPUTrackDto], hccl_track: List[DPUTrackDto]) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        device_stream_map = {}
        for item in task_track + hccl_track:
            device_stream_map.setdefault(item.dpu_device_id, set()).add(item.stream_id)

        header = []
        for device_id, stream_ids in device_stream_map.items():
            header.append(["process_name", device_id, self.tid, TraceViewHeaderConstant.PROCESS_DPU])
            for stream_id in stream_ids:
                header.append(["thread_name", device_id, stream_id, f'Stream {stream_id}'])
                header.append(["thread_sort_index", device_id, stream_id, stream_id])
        return header

    def get_trace_timeline(self: any, dpu_data: tuple) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not dpu_data:
            logging.error("Get dpu data failed.")
            return []
        column_trace_data = self.get_column_trace_data(*dpu_data)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(*dpu_data))
        result.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, column_trace_data)
        )
        return result
