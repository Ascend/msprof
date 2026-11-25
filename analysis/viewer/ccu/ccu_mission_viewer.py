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

import os
from abc import ABC
from bisect import bisect_left
from collections import defaultdict

from viewer.interface.base_viewer import BaseViewer
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.msprof_common import MsProfCommonConstant
from common_func.platform.chip_manager import ChipManager
from msmodel.hardware.ccu_mission_model import CCUViewerMissionModel
from msmodel.hardware.ccu_mission_model import CCUViewerMissionV62Model
from msmodel.add_info.ccu_add_info_model import CCUViewerTaskInfoModel
from msmodel.add_info.ccu_add_info_model import CCUViewerWaitSignalInfoModel
from msmodel.add_info.ccu_add_info_model import CCUViewerGroupInfoModel
from msmodel.hardware.ccu_channel_model import CCUViewerChannelModel
from msmodel.hardware.ccu_channel_model import CCUViewerChannelV62Model
from profiling_bean.hardware.ccu_profile import CCU_V6_2_PROFILE
from profiling_bean.prof_enum.data_tag import DataTag


class CCUMissionViewer(BaseViewer, ABC):
    """
    class for get ccu mission data
    """

    RESERVED = "RESERVED"

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self._is_v62 = ChipManager().chip_id in CCU_V6_2_PROFILE.supported_chip_models
        ccu_device_path = self.params.get(StrConstant.PARAM_RESULT_DIR, "")
        ccu_add_info_path = os.path.join(ccu_device_path, os.pardir, StrConstant.HOST_PATH)
        self.model_list = {}
        if not self._is_v62:
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
    def build_channel_delay_index(channel_data: list) -> dict:
        return CCUMissionViewer._build_delay_index(
            (channel.channel_id, channel.timestamp, channel.avg_bw) for channel in channel_data
        )

    @staticmethod
    def _build_delay_index(samples) -> dict:
        grouped_channel_data = defaultdict(list)
        for channel_id, timestamp, avg_delay in samples:
            grouped_channel_data[channel_id].append((timestamp, avg_delay))

        channel_delay_index = {}
        for channel_id, records in grouped_channel_data.items():
            records.sort(key=lambda record: record[0])
            timestamps = []
            prefix_max_delays = []
            max_delay = 0
            for timestamp, avg_delay in records:
                max_delay = max(max_delay, avg_delay)
                timestamps.append(timestamp)
                prefix_max_delays.append(max_delay)
            channel_delay_index[channel_id] = (timestamps, prefix_max_delays)
        return channel_delay_index

    @staticmethod
    def get_max_delay_channel_and_channel_delay(host_data: list, mission_data: any, channel_delay_index: dict) -> any:
        return CCUMissionViewer._get_max_delay(host_data, mission_data.end_time, channel_delay_index)

    @staticmethod
    def _get_max_delay(host_data: list, end_syscnt: int, channel_delay_index: dict) -> tuple:
        if not channel_delay_index:
            return None, None
        max_delay_channel = None
        max_channel_delay = 0
        host_channel_ids = {channel.channel_id for channel in host_data}
        for channel_id in host_channel_ids:
            channel_records = channel_delay_index.get(channel_id)
            if not channel_records:
                continue
            timestamps, prefix_max_delays = channel_records
            record_index = bisect_left(timestamps, end_syscnt) - 1
            if record_index < 0:
                continue
            channel_delay = prefix_max_delays[record_index]
            if channel_delay > max_channel_delay or (
                channel_delay == max_channel_delay
                and channel_delay > 0
                and (max_delay_channel is None or channel_id < max_delay_channel)
            ):
                max_delay_channel = channel_id
                max_channel_delay = channel_delay
        if max_delay_channel is None:
            return None, None
        return max_delay_channel, max_channel_delay

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

    def get_data_from_db(self: any, table_name: str = None) -> dict:
        """
        get data from msmodel
        :return: []
        """
        if self._is_v62:
            return self._get_v62_mission_data(table_name)
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

    def _get_v62_mission_data(self, table_name: str) -> dict:
        result_dir = self.params.get(StrConstant.PARAM_RESULT_DIR, "")
        model = CCUViewerMissionV62Model(result_dir)
        readers = {
            DBNameConstant.TABLE_CCU_LOOPGROUP_V62: model.get_loopgroup_summary_data,
            DBNameConstant.TABLE_CCU_SETCKB_V62: model.get_setckb_summary_data,
        }
        tables = (table_name,) if table_name else readers
        try:
            if not model.check_db():
                return {}
            return {table: readers[table]() for table in tables}
        finally:
            model.finalize()

    def get_timeline_data(self: any) -> str:
        """
        get model list timeline data
        @return:timeline trace data
        """
        if self._is_v62:
            return self._get_v62_timeline_data()
        ccu_data_dict = self.get_data_from_db()
        result = self.get_trace_timeline(ccu_data_dict)
        return result

    def _get_v62_timeline_data(self: any) -> list:
        result_dir = self.params.get(StrConstant.PARAM_RESULT_DIR, "")
        mission_data = self.get_data_from_db()
        loopgroup_data = mission_data.get(DBNameConstant.TABLE_CCU_LOOPGROUP_V62, [])
        setckb_data = mission_data.get(DBNameConstant.TABLE_CCU_SETCKB_V62, [])
        trace_data = []
        if loopgroup_data:
            group_info = self._load_v62_host_info(result_dir, CCUViewerGroupInfoModel)
            trace_data.extend(self._format_loopgroup_v62_timeline(loopgroup_data, group_info))
        if setckb_data:
            wait_info = self._load_v62_host_info(result_dir, CCUViewerWaitSignalInfoModel)
            delay_indices = {}
            if wait_info:
                with CCUViewerChannelV62Model(result_dir) as model:
                    delay_indices = self._build_v62_delay_indices(model.get_summary_data())
            trace_data.extend(self._format_setckb_v62_timeline(setckb_data, wait_info, delay_indices))
        if not trace_data:
            return []
        return self.get_timeline_header() + TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data
        )

    @staticmethod
    def _load_v62_host_info(result_dir: str, model_type) -> dict:
        host_path = os.path.join(result_dir, os.pardir, StrConstant.HOST_PATH)
        model = model_type(host_path)
        by_instruction = defaultdict(list)
        try:
            if model.check_table():
                for row in model.get_timeline_data():
                    by_instruction[(row.task_id, row.instr_id, row.die_id)].append(row)
        finally:
            model.finalize()
        return by_instruction

    @staticmethod
    def _build_v62_delay_indices(channel_data: list) -> dict:
        samples_by_die = defaultdict(list)
        for row in channel_data:
            samples_by_die[row.die_id].append((row.channel_id, int(row.syscnt), row.avg_delay))
        return {die: CCUMissionViewer._build_delay_index(samples) for die, samples in samples_by_die.items()}

    @staticmethod
    def _group_metadata(row) -> tuple:
        return row.die_id, row.data_size, row.reduce_op_type, row.input_data_type, row.output_data_type

    @staticmethod
    def _loopgroup_args(host_row, duration: float) -> dict:
        args = {"Die Id": host_row.die_id, "Data Size": host_row.data_size}
        if duration > 0:
            args["Bandwidth (MB/s)"] = host_row.data_size / duration * Constant.BYTE_US_TO_MB_S
        if host_row.reduce_op_type != CCUMissionViewer.RESERVED:
            args.update(
                {
                    "Reduce Op Type": host_row.reduce_op_type,
                    "Input Data Type": host_row.input_data_type,
                    "Output Data Type": host_row.output_data_type,
                }
            )
        return args

    def _format_loopgroup_v62_timeline(self: any, loopgroup_data: list, group_info: dict = None) -> list:
        reader = InfoConfReader()
        trace_data = []
        group_info = group_info or {}
        for data in loopgroup_data:
            start_time = reader.trans_syscnt_into_local_time(data.loopgroup_start_syscnt)
            delta_syscnt = data.loopgroup_stop_syscnt - data.loopgroup_start_syscnt
            duration = reader.duration_from_syscnt(delta_syscnt) if delta_syscnt > 0 else 0
            args = {"Task Id": data.task_id, "Instruction ID": data.loopgroup_instr_id}
            if data.die_id is not None:
                args["Die Id"] = data.die_id
            rows = group_info.get((data.task_id, data.loopgroup_instr_id, data.die_id), [])
            if rows:
                metadata = self._group_metadata(rows[0])
                if all(self._group_metadata(row) == metadata for row in rows):
                    args.update(self._loopgroup_args(rows[0], duration))
            trace_data.append(["LoopGroup", self.pid, self.tid, start_time, duration if duration > 0 else 0, args])
        return trace_data

    @staticmethod
    def _resolve_setckb_time(records: list) -> tuple:
        """
        Resolve start/end syscnt for one instruction on one die, and one iteration for loop records.
        """
        for r in records:
            if r.syscnt_type == 1:
                return (r.setckb_syscnt, r.setckb_syscnt, r.setckb_ckbinfo, r.udf_flag, r.udf_profiling_data)
        start_r = None
        end_r = None
        for r in records:
            if r.syscnt_type == 0:
                start_r = r
            elif r.syscnt_type == 2:
                end_r = r
        if start_r and end_r:
            if end_r.setckb_syscnt < start_r.setckb_syscnt:
                return None
            return (
                start_r.setckb_syscnt,
                end_r.setckb_syscnt,
                end_r.setckb_ckbinfo,
                end_r.udf_flag,
                end_r.udf_profiling_data,
            )
        return None

    @staticmethod
    def _setckb_wait_args(key: tuple, wait_info: dict, delay_indices: dict, end_syscnt: int = None) -> dict:
        args = {"Notify Instruction ID": key[1]}
        die_id = key[2]
        if die_id is not None:
            args["Die Id"] = die_id
        rows = (wait_info or {}).get(key, [])
        if not rows:
            return args
        mask = rows[0].mask
        if any(row.mask != mask for row in rows):
            return args
        args["Mask"] = mask
        if end_syscnt is not None and delay_indices:
            index = delay_indices.get(die_id, {})
            channel, delay = CCUMissionViewer._get_max_delay(rows, end_syscnt, index)
            if channel is not None:
                args.update({"Maximum Delay Channel": channel, "Maximum Channel Delay": delay})
        return args

    def _format_setckb_v62_timeline(self: any, setckb_data: list, wait_info=None, delay_indices=None) -> list:
        reader = InfoConfReader()
        mission_data = [d for d in setckb_data if d.lp_flg == 0]
        loop_data = [d for d in setckb_data if d.lp_flg == 1]
        trace_data = []
        if mission_data:
            trace_data.extend(self._format_mission_setckb(mission_data, reader, wait_info, delay_indices))
        if loop_data:
            trace_data.extend(self._format_loop_setckb(loop_data, reader, wait_info, delay_indices))
        return trace_data

    def _format_mission_setckb(
        self, mission_data: list, reader: InfoConfReader, wait_info=None, delay_indices=None
    ) -> list:
        groups = defaultdict(list)
        for d in mission_data:
            groups[(d.task_id, d.setckb_instr_id, d.die_id)].append(d)

        trace_data = []
        for key, records in groups.items():
            task_id, instr_id, _ = key
            resolved = self._resolve_setckb_time(records)
            if not resolved:
                continue
            start_syscnt, end_syscnt, ckbinfo, udf_flag, udf_data = resolved
            start_time = reader.trans_syscnt_into_local_time(start_syscnt)
            duration = reader.duration_from_syscnt(end_syscnt - start_syscnt)

            args = {
                "Task Id": task_id,
                "Instruction ID": instr_id,
                "CKB Info": format(int(ckbinfo), '016b'),
            }
            if udf_flag == 1:
                args["UDF Profiling Data"] = udf_data
            wait_end = None if any(row.syscnt_type == 1 for row in records) else end_syscnt
            args.update(self._setckb_wait_args(key, wait_info, delay_indices, wait_end))

            trace_data.append(["SetCKB Mission", self.pid, self.tid, start_time, duration if duration > 0 else 0, args])
        return trace_data

    def _format_loop_setckb(self, loop_data: list, reader: InfoConfReader, wait_info=None, delay_indices=None) -> list:
        loop_groups = defaultdict(list)
        for d in loop_data:
            loop_groups[((d.task_id, d.setckb_instr_id, d.die_id), d.loop_cnt, d.loop_id)].append(d)

        instr_groups = defaultdict(list)
        for (key, loop_cnt, loop_id), records in loop_groups.items():
            resolved = self._resolve_setckb_time(records)
            if not resolved:
                continue
            start_syscnt, end_syscnt, _, _, _ = resolved
            has_wait_end = not any(row.syscnt_type == 1 for row in records)
            instr_groups[key].append((start_syscnt, end_syscnt, loop_cnt, loop_id, has_wait_end))

        trace_data = []
        for key, loops in instr_groups.items():
            task_id, instr_id, _ = key
            earliest = min(loops, key=lambda loop: (loop[0], loop[2], loop[3]))
            latest = min(loops, key=lambda loop: (-loop[1], loop[2], loop[3]))
            longest = min(loops, key=lambda loop: (loop[0] - loop[1], loop[2], loop[3]))

            start_time = reader.trans_syscnt_into_local_time(earliest[0])
            total_duration = reader.duration_from_syscnt(latest[1] - earliest[0])
            longest_duration = reader.duration_from_syscnt(longest[1] - longest[0])

            args = {
                "Task Id": task_id,
                "Instruction ID": instr_id,
                "Earliest Loop Id": earliest[3],
                "Latest Loop Id": latest[3],
                "Longest Duration Loop Id": longest[3],
                "Longest Duration(us)": longest_duration,
            }
            wait_end = latest[1] if any(loop[1] == latest[1] and loop[4] for loop in loops) else None
            args.update(self._setckb_wait_args(key, wait_info, delay_indices, wait_end))
            trace_data.append(
                ["SetCKB Loop", self.pid, self.tid, start_time, total_duration if total_duration > 0 else 0, args]
            )
        return trace_data

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

    def get_loopgroup_v62_summary_data(self: any) -> tuple:
        table = DBNameConstant.TABLE_CCU_LOOPGROUP_V62
        formatted_loopgroup = self.format_loopgroup_v62_summary_data(self.get_data_from_db(table).get(table, []))
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_loopgroup, len(formatted_loopgroup)

    def get_setckb_v62_summary_data(self: any) -> tuple:
        table = DBNameConstant.TABLE_CCU_SETCKB_V62
        formatted_setckb = self.format_setckb_v62_summary_data(self.get_data_from_db(table).get(table, []))
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_setckb, len(formatted_setckb)

    @staticmethod
    def format_loopgroup_v62_summary_data(loopgroup_data: list) -> list:
        reader = InfoConfReader()
        formatted_data = []
        for data in loopgroup_data:
            start_time = format_high_precision_for_csv(reader.trans_syscnt_into_local_time(data.loopgroup_start_syscnt))
            duration = format_high_precision_for_csv(
                str(reader.duration_from_syscnt(max(data.loopgroup_stop_syscnt - data.loopgroup_start_syscnt, 0)))
            )
            formatted_data.append((data.task_id, data.loopgroup_instr_id, start_time, duration))
        return formatted_data

    @staticmethod
    def format_setckb_v62_summary_data(setckb_data: list) -> list:
        reader = InfoConfReader()
        formatted_data = []
        for data in setckb_data:
            time_stamp = format_high_precision_for_csv(reader.trans_syscnt_into_local_time(data.setckb_syscnt))
            formatted_data.append(
                (
                    data.task_id,
                    data.setckb_instr_id,
                    data.loop_id,
                    data.loop_cnt,
                    format(int(data.setckb_ckbinfo), '016b'),
                    time_stamp,
                    data.udf_profiling_data,
                    data.udf_flag,
                    data.syscnt_type,
                    data.lp_flg,
                )
            )
        return formatted_data

    def get_formatted_loop_data(self, device_loop_data, group_data):
        result = []
        if not device_loop_data:
            return result
        grouped_loop_data = defaultdict(list)
        for item in device_loop_data:
            key = (item.stream_id, item.task_id, item.lp_instr_id)
            grouped_loop_data[key] = item

        grouped_group_data = defaultdict(list)
        for item in group_data:
            key = (item.stream_id, item.task_id, item.instr_id)
            grouped_group_data[key].append(item)

        for key, data in grouped_loop_data.items():
            start_time = InfoConfReader().trans_syscnt_into_local_time(data.start_time)
            duration = InfoConfReader().duration_from_syscnt(data.end_time - data.start_time)
            host_data = grouped_group_data.get(key, [])
            args = {"Physic Stream Id": data.stream_id, "Task Id": data.task_id, "Instruction ID": data.lp_instr_id}
            if host_data:
                args.update(self._loopgroup_args(host_data[0], duration))
            result.append([data.time_type, self.pid, self.tid, start_time, duration if duration > 0 else 0, args])
        return result

    def get_formatted_wait_data(self, wait_data, wait_signal_data, channel_data):
        result = []
        if not wait_data:
            return result
        grouped_wait_data = defaultdict(list)
        for item in wait_data:
            key = (item.stream_id, item.task_id, item.setckebit_instr_id)
            grouped_wait_data[key].append(item)

        grouped_wait_signal_data = defaultdict(list)
        for item in wait_signal_data:
            key = (item.stream_id, item.task_id, item.instr_id)
            grouped_wait_signal_data[key].append(item)

        channel_delay_index = self.build_channel_delay_index(channel_data)

        for key, data_list in grouped_wait_data.items():
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
                    host_data, latest_data, channel_delay_index
                )
                if max_delay_channel is not None and max_channel_delay is not None:
                    args.update(
                        {"Maximum Delay Channel": max_delay_channel, "Maximum Channel Delay": max_channel_delay}
                    )

            result.append(
                [latest_data.time_type, self.pid, self.tid, start_time, duration if duration > 0 else 0, args]
            )
        return result
