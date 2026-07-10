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

import os
from abc import ABC
from collections import defaultdict

from viewer.interface.base_viewer import BaseViewer
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.msprof_common import MsProfCommonConstant
from msmodel.hardware.ccu_mission_model import CCUViewerMissionModel
from msmodel.add_info.ccu_add_info_model import CCUViewerTaskInfoModel
from msmodel.add_info.ccu_add_info_model import CCUViewerWaitSignalInfoModel
from msmodel.add_info.ccu_add_info_model import CCUViewerGroupInfoModel
from msmodel.hardware.ccu_channel_model import CCUViewerChannelModel
from profiling_bean.prof_enum.data_tag import DataTag


class CCUMissionViewer(BaseViewer, ABC):
    """
    class for get ccu mission data
    """

    RESERVED = "RESERVED"

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        ccu_device_path = self.params.get(StrConstant.PARAM_RESULT_DIR)
        ccu_add_info_path = os.path.join(ccu_device_path, os.pardir, StrConstant.HOST_PATH)
        self.model_list = {
            DataTag.CCU_MISSION: (CCUViewerMissionModel, ccu_device_path),
            DataTag.CCU_CHANNEL: (CCUViewerChannelModel, ccu_device_path),
        }
        if os.path.exists(ccu_add_info_path):
            self.model_list.update(
                {
                    DataTag.CCU_TASK: (CCUViewerTaskInfoModel, ccu_add_info_path),
                    DataTag.CCU_WAIT_SIGNAL: (CCUViewerWaitSignalInfoModel, ccu_add_info_path),
                    DataTag.CCU_GROUP: (CCUViewerGroupInfoModel, ccu_add_info_path),
                }
            )
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    @staticmethod
    def format_mission_summary_data(summary_data: list) -> list:
        return [
            (
                data.stream_id,
                data.task_id,
                data.lp_instr_id,
                format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(data.lp_start_time)),
                format_high_precision_for_csv(
                    str(InfoConfReader().duration_from_syscnt(data.lp_end_time - data.lp_start_time))
                ),
                data.setckebit_instr_id,
                data.rel_id,
                format_high_precision_for_csv(
                    str(InfoConfReader().duration_from_syscnt(data.rel_end_time - data.setckebit_start_time))
                ),
            )
            for data in summary_data
        ]

    @staticmethod
    def get_max_delay_channel_and_channel_delay(host_data: list, mission_data: any, channel_data: dict) -> any:
        if not channel_data:
            return None, None
        host_channel_ids = {channel.channel_id for channel in host_data}

        within_channel = []
        for host_channel_id in host_channel_ids:
            if host_channel_id in channel_data:
                within_channel.extend(channel_data[host_channel_id])
        if not within_channel:
            return None, None

        seq_channel = [channel for channel in within_channel if channel.timestamp < mission_data.end_time]
        if seq_channel:
            max_delay_channel = max(seq_channel, key=lambda x: x.avg_bw)
            return max_delay_channel.channel_id, max_delay_channel.avg_bw
        return None, None

    def get_timeline_header(self) -> list:
        header = [
            ["process_name", self.pid, self.tid, TraceViewHeaderConstant.PROCESS_CCU],
            ["thread_name", self.pid, self.tid, TraceViewHeaderConstant.PROCESS_COMMUNICATION],
        ]
        return TraceViewManager.metadata_event(header)

    def get_model_instance(self: any) -> any:
        """
        get model instance from list
        return model_list [(model_name, model_instance)]
        """
        model_dict = defaultdict(list)
        for model_name, model_info in self.model_list.items():
            model_dict[model_name] = model_info[0](model_info[1])
        return model_dict

    def get_data_from_db(self: any) -> dict:
        """
        get data from msmodel
        :return: []
        """
        ccu_data_dict = defaultdict(list)
        model_dict = self.get_model_instance()
        if self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            for tag, model in model_dict.items():
                if not model or not model.check_table():
                    continue
                data = model.get_timeline_data()
                ccu_data_dict[tag] = data
                model.finalize()
        else:
            mission_model = model_dict.get(DataTag.CCU_MISSION)
            if not mission_model or not mission_model.check_table():
                return ccu_data_dict
            mission_summary_data = mission_model.get_summary_data()
            ccu_data_dict = {DataTag.CCU_MISSION: mission_summary_data}
        return ccu_data_dict

    def get_timeline_data(self: any) -> str:
        """
        get model list timeline data
        @return:timeline trace data
        """
        ccu_data_dict = self.get_data_from_db()
        result = self.get_trace_timeline(ccu_data_dict)
        return result

    def get_trace_timeline(self: any, ccu_data_dict: dict) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not ccu_data_dict:
            return []
        result = []
        ccu_mission_data = ccu_data_dict.get(DataTag.CCU_MISSION, [])
        loop_data = [data for data in ccu_mission_data if data.time_type == 'LoopGroup']
        wait_data = [data for data in ccu_mission_data if data.time_type == 'Wait']
        result.extend(self.get_formatted_loop_data(loop_data, ccu_data_dict.get(DataTag.CCU_GROUP, [])))
        result.extend(
            self.get_formatted_wait_data(
                wait_data, ccu_data_dict.get(DataTag.CCU_WAIT_SIGNAL, []), ccu_data_dict.get(DataTag.CCU_CHANNEL, [])
            )
        )
        if not result:
            return []
        return self.get_timeline_header() + TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result
        )

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db().get(DataTag.CCU_MISSION, [])
        formatted_data = self.format_mission_summary_data(summary_data)
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_data, len(formatted_data)

    def get_formatted_loop_data(self, device_loop_data, group_data):
        result = []
        if not device_loop_data:
            return result
        grouped_loop_data = defaultdict(list)
        for item in device_loop_data:
            key = (item.task_id, item.lp_instr_id)
            grouped_loop_data[key] = item

        grouped_group_data = defaultdict(list)
        for item in group_data:
            key = (item.task_id, item.instr_id)
            grouped_group_data[key].append(item)

        for key, data in grouped_loop_data.items():
            start_time = InfoConfReader().trans_syscnt_into_local_time(data.start_time)
            duration = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            host_data = grouped_group_data.get(key, [])
            args = {"Physic Stream Id": data.stream_id, "Task Id": data.task_id, "Instruction ID": data.lp_instr_id}
            if host_data:
                args.update({"Die Id": host_data[0].die_id, "Data Size": host_data[0].data_size})
                if duration != 0:
                    args.update({"Bandwidth (MB/s)": host_data[0].data_size / duration * Constant.BYTE_US_TO_MB_S})
                if host_data[0].reduce_op_type != CCUMissionViewer.RESERVED:
                    args.update(
                        {
                            "Reduce Op Type": host_data[0].reduce_op_type,
                            "Input Data Type": host_data[0].input_data_type,
                            "Output Data Type": host_data[0].output_data_type,
                        }
                    )
            result.append([data.time_type, self.pid, self.tid, start_time, duration if duration > 0 else 0, args])
        return result

    def get_formatted_wait_data(self, wait_data, wait_signal_data, channel_data):
        result = []
        if not wait_data:
            return result
        grouped_loop_data = defaultdict(list)
        for item in wait_data:
            key = (item.task_id, item.setckebit_instr_id)
            grouped_loop_data[key].append(item)

        grouped_wait_signal_data = defaultdict(list)
        for item in wait_signal_data:
            key = (item.task_id, item.instr_id)
            grouped_wait_signal_data[key].append(item)

        # channel data is classified by channel_id for easy access when calculating max delay channel and channel delay
        channel_data_classified = defaultdict(list)
        for item in channel_data:
            key = item.channel_id
            channel_data_classified[key].append(item)

        for key, data_list in grouped_loop_data.items():
            latest_data = max(data_list, key=lambda x: x.end_time)
            start_time = InfoConfReader().trans_syscnt_into_local_time(latest_data.start_time)
            duration = InfoConfReader().duration_from_syscnt(latest_data.end_time - latest_data.start_time)
            host_data = grouped_wait_signal_data.get(key, [])
            args = {
                "Physic Stream Id": latest_data.stream_id,
                "Task Id": latest_data.task_id,
                "Notify Instruction ID": latest_data.setckebit_instr_id,
                "Notify Rank ID": latest_data.rel_id,
            }
            if host_data:
                args.update({"Die Id": host_data[0].die_id, "Mask": host_data[0].mask})
                max_delay_channel, max_channel_delay = self.get_max_delay_channel_and_channel_delay(
                    host_data, latest_data, channel_data_classified
                )
                if max_delay_channel and max_channel_delay:
                    args.update(
                        {"Maximum Delay Channel": max_delay_channel, "Maximum Channel Delay": max_channel_delay}
                    )

            result.append(
                [latest_data.time_type, self.pid, self.tid, start_time, duration if duration > 0 else 0, args]
            )
        return result
