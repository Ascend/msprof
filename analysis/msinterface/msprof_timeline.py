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

import json
import logging
import os
from decimal import Decimal

from common_func.common import error
from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.singleton import singleton
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.profiling_scene import ProfilingScene
from profiling_bean.db_dto.step_trace_dto import IterationRange
from viewer.association.host_connect_device import HostToDevice
from viewer.training.step_trace_viewer import StepTraceViewer


@singleton
class MsprofTimeline:
    """
    This class is used to export a summary timeline json file.
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any) -> None:
        self._iter_range = None
        self._model_id = NumberConstant.DEFAULT_MODEL_ID
        self._result_dir = None
        self._export_data_list = []
        self._iteration_time = (float('-inf'), float('inf'))
        self._default_sort_index = TraceViewHeaderConstant.DEFAULT_LAYER_SORT_START
        self.connect_list = {}

    @classmethod
    def get_timeline_header(cls: any, pid: str, pid_sort_index: int) -> list:
        """
        get timeline header
        """
        header = [["process_sort_index", pid, InfoConfReader().get_json_tid_data(), pid_sort_index]]
        process_index = TraceViewManager.metadata_event(header)
        return process_index

    @classmethod
    def filter_msprof_timeline(cls: any, json_list: list) -> list:
        """
        filter msprof timeline data, and partition data by pid.
        """
        pid_process_name = dict()
        pid_json_data = dict()
        filtered_data_list = []
        for data_dict in json_list:
            pid = data_dict.get(StrConstant.TRACE_HEADER_PID, TraceViewHeaderConstant.DEFAULT_PID_VALUE)
            if data_dict.get(StrConstant.TRACE_HEADER_NAME) == "process_name":
                pid_process_name[pid] = data_dict.get(StrConstant.TRACE_HEADER_ARGS, {}).get(
                    StrConstant.TRACE_HEADER_NAME, ""
                )
            if pid_process_name.get(pid) not in TraceViewHeaderConstant.MSPROF_TIMELINE_FILTER_LIST:
                pid_json_data.setdefault(pid, []).append(data_dict)
        for key in pid_process_name:
            if pid_process_name.get(key) not in TraceViewHeaderConstant.MSPROF_TIMELINE_FILTER_LIST:
                filtered_data_list.append([key, pid_process_name.get(key), pid_json_data.get(key)])
        return filtered_data_list

    @classmethod
    def modify_timeline_info(
        cls: any, process_name: str, layer_info: TraceViewHeaderConstant.LayerInfo, format_pid: int, value: dict
    ) -> None:
        """
        modify timeline info based on layer_info
        """
        value[StrConstant.TRACE_HEADER_PID] = format_pid
        if value.get(StrConstant.TRACE_HEADER_NAME) == "process_name":
            value.setdefault(StrConstant.TRACE_HEADER_ARGS, {})[StrConstant.TRACE_HEADER_NAME] = (
                layer_info.component_layer
            )

        if (
            value.get(StrConstant.TRACE_HEADER_NAME) == "thread_name"
            and process_name == TraceViewHeaderConstant.PROCESS_STEP_TRACE
        ):
            value.setdefault(StrConstant.TRACE_HEADER_ARGS, {})[StrConstant.TRACE_HEADER_NAME] = (
                f'{process_name}({value.get(StrConstant.TRACE_HEADER_ARGS, {}).get(StrConstant.TRACE_HEADER_NAME, "")})'
            )

        if cls.is_cann_ai_stack_data(layer_info, value):
            # if is cann data, remove device_id
            value[StrConstant.TRACE_HEADER_PID] = format_pid
            level = value.get(StrConstant.TRACE_HEADER_ARGS, {}).get(StrConstant.API_EVENT_HEADER_LEVEL)
            if not level:
                prefix = process_name
            else:
                prefix = StrConstant.LEVEL_MAP.get(level, level.capitalize())
            value[StrConstant.TRACE_HEADER_NAME] = f'{prefix}@{value.get(StrConstant.TRACE_HEADER_NAME, "")}'

    @classmethod
    def get_layer_label_and_sort(cls: any, pid: int, layer_info: TraceViewHeaderConstant.LayerInfo) -> list:
        """
        get layer_label layer_sort headers
        """
        general_layer = cls.get_label_name_by_pid(pid, layer_info.general_layer)
        label_header = [["process_labels", pid, InfoConfReader().get_json_tid_data(), general_layer]]
        sort_header = [["process_sort_index", pid, InfoConfReader().get_json_tid_data(), layer_info.sort_index]]
        process_label = TraceViewManager.metadata_event(label_header)
        process_sort = TraceViewManager.metadata_event(sort_header)
        return process_label + process_sort

    @classmethod
    def get_label_name_by_pid(cls, pid: int, label_name: str):
        device_id = TraceViewManager.get_device_id_from_format_pid(pid)
        return label_name if device_id == TraceViewManager.HOST_ID_FOR_PID else label_name + " " + str(device_id)

    @classmethod
    def is_cann_ai_stack_data(cls: any, layer_info: TraceViewHeaderConstant.LayerInfo, value: dict) -> bool:
        """
        return whether the data is cann ai stack data
        """
        return (
            value.get("ph") == "X"
            and layer_info.component_layer == TraceViewHeaderConstant.COMPONENT_LAYER_CANN
            and layer_info.general_layer == TraceViewHeaderConstant.GENERAL_LAYER_CPU
        )

    def init_export_data(self: any) -> None:
        self._export_data_list = []

    def add_export_data(self: any, export_data: json, data_type: str) -> None:
        """
        index events in bulk json
        :param data_type: data type
        :param export_data: data to export
        :param data_type: data_type
        :return: None
        """
        if not export_data:
            return

        try:
            if isinstance(export_data, list):
                self.add_sort_index(export_data)
                self.add_connect_json_line(export_data, data_type)
                if data_type not in ["api"]:
                    start_time, end_time = self.get_start_end_time()
                    export_data = filter(
                        lambda value: value["ph"] in TraceViewHeaderConstant.NOT_FILTER_PHASE
                        or self.is_in_iteration(value, start_time, end_time),
                        export_data,
                    )
                self._export_data_list.extend(export_data)
        except (TypeError, ValueError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            error(self.FILE_NAME, err)

    def add_connect_json_line(self: any, json_list: list, data_type: str) -> None:
        """
        add connect line with task time
        :param json_list: json list
        :param data_type: data_type
        """
        for connect_obj in self.connect_list.values():
            connect_obj.add_connect_line(json_list, data_type)

    def add_sort_index(self: any, json_list: list) -> None:
        """
        add sort index and header
        :param json_list: json list
        """
        if isinstance(json_list, list):
            filtered_data_list = self.filter_msprof_timeline(json_list)
            json_list.clear()
            for filtered_data in filtered_data_list:
                process_name = filtered_data[1]
                json_data = filtered_data[2]
                pid = filtered_data[0]
                # get the msprof timeline layer info
                layer_info = self.get_layer_info(process_name)
                format_pid = TraceViewManager.get_format_pid(pid, layer_info)
                for value in json_data:
                    self.modify_timeline_info(process_name, layer_info, format_pid, value)
                json_list.extend(json_data)
                json_list.extend(self.get_layer_label_and_sort(format_pid, layer_info))

    def export_all_data(self: any) -> list:
        """
        get bulk data
        :return: json for timeline
        """
        data = EmptyClass()
        if not ProfilingScene().is_all_export():
            data = StepTraceViewer.get_one_iter_timeline_data(self._result_dir, self._iter_range)
        if not isinstance(data, EmptyClass):
            data_list = data
            if isinstance(data_list, list) and data_list:
                self.add_sort_index(data_list)
                data_list.extend(self._export_data_list)
                return data_list
        if not self._export_data_list:
            return []
        return self._export_data_list

    def is_in_iteration(self: any, json_value: dict, start_time: Decimal, end_time: Decimal) -> bool:
        """
        check if in iteration
        """
        # Show all data without iteration time
        if not self._iteration_time:
            return True
        time_start = Decimal(
            str(json_value.get(TraceViewHeaderConstant.TRACE_HEADER_TS, NumberConstant.DEFAULT_START_TIME))
        )
        time_dur = Decimal(
            str(json_value.get(TraceViewHeaderConstant.TRACE_HEADER_DURATION, NumberConstant.DEFAULT_START_TIME))
        )
        time_end = time_start + time_dur

        return start_time <= time_start < end_time or time_start < start_time < time_end

    def set_connection_list(self, result_dir: str):
        # 入参设置result_dir 避免依赖成员变量
        host_to_device_obj = HostToDevice(result_dir)
        self.connect_list["host_to_device"] = host_to_device_obj

    def set_iteration_info(self: any, result_dir: str, iter_range: IterationRange) -> None:
        """
        get iteration time
        """
        self._result_dir = result_dir
        self._iter_range = iter_range
        self._model_id = iter_range.model_id
        if ProfilingScene().is_all_export():
            start_time, _ = InfoConfReader().get_collect_time()
            self._iteration_time = (float(start_time), float('inf'))  # 结束时间设置为无穷大
        else:
            start_time, end_time = MsprofIteration(result_dir).get_iter_interval(
                iter_range, NumberConstant.MICRO_SECOND
            )
            start_time = Decimal(InfoConfReader().trans_into_local_time(start_time, use_us=True))
            end_time = Decimal(InfoConfReader().trans_into_local_time(end_time, use_us=True))
            self._iteration_time = (float(start_time), float(end_time))

    def get_layer_info(self: any, process_name: str) -> TraceViewHeaderConstant.LayerInfo:
        """
        get msprof timeline layer info based on map
        """
        layer_info = TraceViewHeaderConstant.LAYER_INFO_MAP.get(process_name, "")
        if layer_info:
            return layer_info
        else:
            self._default_sort_index += 1
            return TraceViewHeaderConstant.LayerInfo(
                process_name, TraceViewHeaderConstant.GENERAL_LAYER_NPU, self._default_sort_index
            )

    def get_start_end_time(self: any):
        return self._iteration_time
