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

# pylint: disable=duplicate-code

from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel


class MsprofTxViewer:
    """
    class for get msproftx data
    """

    mark_id_message_dict = {}

    def __init__(self: any, configs: dict, params: dict) -> None:
        self.configs = configs
        self.params = params

    @staticmethod
    def get_device_timeline_header(data: list) -> list:
        """
        to get sequence chrome device trace json header
        :return: header of device trace data list
        """
        if not data:
            return []
        header = [
            [
                "process_name",
                InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(),
                TraceViewHeaderConstant.PROCESS_TASK,
            ]
        ]
        subtask = []
        tid_set = set((item[1], item[2]) for item in data)
        for item in tid_set:
            subtask.append(["thread_name", item[0], item[1], f'Stream {item[1]}'])
            subtask.append(["thread_sort_index", item[0], item[1], item[1]])
        header.extend(subtask)
        return header

    @staticmethod
    def get_time_timeline_header(msproftx_datas: tuple) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        pid_values = set()
        tid_values = set()
        json_tid_data = InfoConfReader().get_json_tid_data()
        for msproftx_data in msproftx_datas:
            pid_values.add(msproftx_data.pid)
            tid_values.add(msproftx_data.tid)
        meta_data = Utils.generator_to_list(
            ["process_name", pid_value, json_tid_data, TraceViewHeaderConstant.PROCESS_MSPROFTX]
            for pid_value in pid_values
        )
        meta_data.extend(
            Utils.generator_to_list(
                ["thread_name", pid_value, tid_value, "Thread {}".format(tid_value)]
                for tid_value in tid_values
                for pid_value in pid_values
            )
        )
        meta_data.extend(
            Utils.generator_to_list(
                ["thread_sort_index", pid_value, tid_value, tid_value]
                for tid_value in tid_values
                for pid_value in pid_values
            )
        )

        return meta_data

    @staticmethod
    def format_tx_timeline_data(msproftx_data: tuple) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        trace_data = []
        for top_down_data in msproftx_data:
            trace_data_args = OrderedDict(
                [
                    ("Category", str(top_down_data.category)),
                    ("Payload_type", top_down_data.payload_type),
                    ("Payload_value", top_down_data.payload_value),
                    ("Message_type", top_down_data.message_type),
                    ("event_type", top_down_data.event_type),
                ]
            )
            trace_data_msproftx = [
                top_down_data.message,
                top_down_data.pid,
                top_down_data.tid,
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(top_down_data.start_time, NumberConstant.MICRO_SECOND),
                    use_us=True,
                    is_host=True,
                ),
                InfoConfReader().get_host_duration(top_down_data.dur_time, NumberConstant.MICRO_SECOND),
                trace_data_args,
            ]
            trace_data.append(trace_data_msproftx)
        return trace_data

    @staticmethod
    def format_tx_ex_timeline_data(msproftx_ex_data: tuple) -> list:
        """
        to format msprof ex data to chrome trace json
        :param msproftx_ex_data: msprof ex data
        :return: timeline_trace list
        """
        trace_data = []
        for data in msproftx_ex_data:
            trace_data_args = OrderedDict(
                [('mark_id', data.mark_id), ('event_type', data.event_type), ('domain', data.domain)]
            )
            trace_data_msproftx_ex = [
                data.message,
                data.pid,
                data.tid,
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data.start_time, NumberConstant.MICRO_SECOND),
                    use_us=True,
                    is_host=True,
                ),
                InfoConfReader().get_host_duration(data.dur_time, NumberConstant.MICRO_SECOND),
                trace_data_args,
            ]
            trace_data.append(trace_data_msproftx_ex)
        return trace_data

    @staticmethod
    def format_tx_summary_data(summary_data: list) -> list:
        return [
            (
                data[0],
                data[1],
                data[2],
                data[3],
                data[4],
                data[5],
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        InfoConfReader().time_from_host_syscnt(data[6], NumberConstant.MICRO_SECOND),
                        use_us=True,
                        is_host=True,
                    )
                ),
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        InfoConfReader().time_from_host_syscnt(data[7], NumberConstant.MICRO_SECOND),
                        use_us=True,
                        is_host=True,
                    )
                ),
                data[8],
                data[9],
                Constant.NA,
                Constant.NA,
            )
            for data in summary_data
        ]

    @staticmethod
    def format_tx_ex_summary_data(summary_data: list) -> list:
        return [
            (
                data[0],
                data[1],
                Constant.NA,
                data[2],
                Constant.NA,
                Constant.NA,
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        InfoConfReader().time_from_host_syscnt(data[3], NumberConstant.MICRO_SECOND),
                        use_us=True,
                        is_host=True,
                    )
                ),
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        InfoConfReader().time_from_host_syscnt(data[4], NumberConstant.MICRO_SECOND),
                        use_us=True,
                        is_host=True,
                    )
                ),
                Constant.NA,
                f'{data[5]}',
                data[6],
                data[7],
            )
            for data in summary_data
        ]

    @staticmethod
    def format_tx_ex_device_summary_data(device_data: list) -> list:
        return [
            (
                data[0],
                format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(data[1])),
                format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(data[1] + data[4])),
            )
            for data in device_data
        ]

    @staticmethod
    def format_tx_ex_device_timeline_data(device_data: list) -> list:
        """
        to format msprof device data to chrome trace json
        :param msproftx_device_data: msprof device data
        :return: timeline_trace list
        """
        task_trace = []
        json_pid_data = InfoConfReader().get_json_pid_data()
        for data in device_data:
            task_trace.append(
                [
                    data[5],
                    json_pid_data,
                    data[2],
                    InfoConfReader().trans_syscnt_into_local_time(data[1]),
                    InfoConfReader().duration_from_syscnt(data[4]),
                    {"Physic Stream Id": data[2], "Task Id": data[3]},
                ]
            )
        return task_trace

    @staticmethod
    def get_msproftx_ex_flow_end_points(task_data: list) -> list:
        """
        add msproftx_ex end points for host to device connection
        :param traces: msproftx traces as json list
        :return: msproftx_ex_flow_end_points list
        """
        if not task_data:
            return []
        end_points = []
        json_pid_data = InfoConfReader().get_json_pid_data()
        for data in task_data:
            mark_id = data[0]
            end_point = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'MsTx_{mark_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 'f',
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(mark_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: InfoConfReader().trans_syscnt_into_local_time(data[1]),
                TraceViewHeaderConstant.TRACE_HEADER_CAT: StrConstant.MSTX,
                TraceViewHeaderConstant.TRACE_HEADER_PID: json_pid_data,
                TraceViewHeaderConstant.TRACE_HEADER_TID: data[2],
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
            end_points.append(end_point)
        return end_points

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        :return:summary data
        """
        with MsprofTxModel(
            self.params.get('project'), DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX]
        ) as tx_model:
            msproftx_data = tx_model.get_summary_data()
        with MsprofTxExModel(
            self.params.get('project'), DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX_EX]
        ) as msproftx_ex_model:
            msproftx_ex_data = msproftx_ex_model.get_summary_data()
        if not msproftx_data and not msproftx_ex_data:
            return MsvpConstant.MSVP_EMPTY_DATA
        msproftx_data = self.format_tx_summary_data(msproftx_data)
        msproftx_ex_data = self.format_tx_ex_summary_data(msproftx_ex_data)
        summary_data = msproftx_data + msproftx_ex_data
        summary_data.sort(key=lambda x: x[6])
        return self.configs.get('headers'), summary_data, len(summary_data)

    def get_timeline_data(self: any) -> any:
        """
        to get timeline data
        :return:timeline data
        """
        TraceViewHeaderConstant.update_layer_info_map(InfoConfReader().get_json_pid_name())
        with MsprofTxModel(
            self.params.get('project'), DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX]
        ) as tx_model:
            msproftx_data = tx_model.get_timeline_data()
        with MsprofTxExModel(
            self.params.get('project'), DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX_EX]
        ) as msproftx_ex_model:
            msproftx_ex_data = msproftx_ex_model.get_timeline_data()
        if not msproftx_data and not msproftx_ex_data:
            return []

        # add mark_id message to dict
        for data in msproftx_ex_data:
            MsprofTxViewer.mark_id_message_dict[data.mark_id] = data.message

        tx_trace_data = self.format_tx_timeline_data(msproftx_data)
        msproftx_ex_trace_data = self.format_tx_ex_timeline_data(msproftx_ex_data)
        _trace = TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, tx_trace_data + msproftx_ex_trace_data
        )
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(msproftx_data + msproftx_ex_data))
        result.extend(_trace)
        return result

    def get_device_summary_data(self: any) -> tuple:
        """
        to get device summary data
        :return:device summary data
        """
        with MsprofTxExModel(
            self.params.get('project'), DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE]
        ) as msproftx_ex_model:
            msproftx_device_data = msproftx_ex_model.get_device_data()

        msproftx_device_data = self.format_tx_ex_device_summary_data(msproftx_device_data)
        device_headers = ['index_id', 'start_time(us)', 'end_time(us)']

        return device_headers, msproftx_device_data, len(msproftx_device_data)

    def get_device_timeline_data(self: any) -> any:
        """
        to get device timeline data
        :return:device timeline data
        """
        with MsprofTxExModel(
            self.params.get('project'), DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE]
        ) as msproftx_ex_model:
            msproftx_device_data = msproftx_ex_model.get_device_data()

        msproftx_device_message_data = []

        # add message to device data
        for data in msproftx_device_data:
            if MsprofTxViewer.mark_id_message_dict.get(data[0]):
                data.append(MsprofTxViewer.mark_id_message_dict[data[0]])
            else:
                data.append(Constant.NA)
            msproftx_device_message_data.append(data)
        msproftx_device_trace_data = self.format_tx_ex_device_timeline_data(msproftx_device_message_data)
        _trace = TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, msproftx_device_trace_data
        )
        result = TraceViewManager.metadata_event(self.get_device_timeline_header(msproftx_device_trace_data))
        result.extend(_trace)
        msproftx_ex_flow_end_points = self.get_msproftx_ex_flow_end_points(msproftx_device_message_data)
        result.extend(msproftx_ex_flow_end_points)
        return result
