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
import logging
from typing import Dict
from typing import List

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ge_logic_stream_singleton import GeLogicStreamSingleton
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from msmodel.sqe_type_map import SqeType
from msmodel.stars.op_summary_model import OpSummaryModel
from msmodel.stars.sub_task_model import SubTaskTimeModel
from msmodel.task_time.ascend_task_model import AscendTaskModel
from msmodel.add_info.kfc_info_model import KfcInfoViewModel
from msmodel.add_info.kfc_info_model import KfcTurnData
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.interface.base_viewer import BaseViewer
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


class TaskTimeViewer(BaseViewer):
    """
    class for get task time data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.project_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)
        self.trace_pid_map = {
            TraceViewHeaderConstant.PROCESS_TASK: InfoConfReader().get_json_pid_data(),
            TraceViewHeaderConstant.PROCESS_SUBTASK: 1,
            TraceViewHeaderConstant.PROCESS_THREAD_TASK: 2,
        }

    @staticmethod
    def get_device_task_type(device_task_type: str) -> str:
        if not device_task_type.isdigit():
            return device_task_type
        elif ChipManager().chip_id != ChipModel.CHIP_V2_1_0 and int(device_task_type) in [
            enum.value for enum in SqeType().instance
        ]:
            return SqeType().instance(int(device_task_type)).name
        else:
            return Constant.TASK_TYPE_OTHER

    @staticmethod
    def get_task_type(host_task_type: str, device_task_type: str) -> str:
        if host_task_type == Constant.TASK_TYPE_FFTS_PLUS:
            return device_task_type
        elif host_task_type == Constant.TASK_TYPE_UNKNOWN:
            return TaskTimeViewer.get_device_task_type(device_task_type)
        return host_task_type

    @staticmethod
    def _update_op_name_and_type(data, node_info_dict, node_key) -> None:
        default_task_type = TaskTimeViewer.get_task_type(data.host_task_type, data.device_task_type)
        node_info = node_info_dict.get(node_key, {})
        node_info_task_type = node_info.get("task_type")
        task_type = node_info_task_type if node_info_task_type not in (None, Constant.NA) else default_task_type
        setattr(data, 'op_name', node_info.get('op_name', task_type))
        setattr(data, 'task_type', task_type)

    def get_time_timeline_header(self: any, data: list, pid_header=TraceViewHeaderConstant.PROCESS_TASK) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        header = [
            ["process_name", self.trace_pid_map.get(pid_header, 0), InfoConfReader().get_json_tid_data(), pid_header]
        ]
        subtask = []
        tid_set = set((item[1], item[2]) for item in data)
        for item in tid_set:
            if pid_header == TraceViewHeaderConstant.PROCESS_THREAD_TASK:
                thread_id = int(item[1] / (max(StarsConstant.SUBTASK_TYPE) + 1))
                device_task_type = StarsConstant.SUBTASK_TYPE.get(item[1] % (max(StarsConstant.SUBTASK_TYPE) + 1))
                subtask.append(["thread_name", item[0], item[1], f'Thread {thread_id}({device_task_type})'])
            elif pid_header == TraceViewHeaderConstant.PROCESS_SUBTASK:
                subtask.append(["thread_name", item[0], item[1], f'Stream {StarsConstant.SUBTASK_TYPE.get(item[1])}'])
            else:
                subtask.append(["thread_name", item[0], item[1], f'Stream {item[1]}'])
            subtask.append(["thread_sort_index", item[0], item[1], item[1]])
        header.extend(subtask)
        return header

    def get_ascend_task_data(self: any) -> Dict[str, List[TopDownTask]]:
        task_data = {
            'task_data_list': [],
            'subtask_data_list': [],
        }
        task_data_list = []
        conn, curs = DBManager.check_connect_db(self.project_dir, DBNameConstant.DB_ASCEND_TASK)
        if conn and curs:
            DBManager.destroy_db_connect(conn, curs)
            with AscendTaskModel(self.project_dir, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
                task_data_list = model.get_ascend_task_data_without_unknown()
        for data in task_data_list:
            if data.context_id == NumberConstant.DEFAULT_GE_CONTEXT_ID:
                task_data['task_data_list'].append(TopDownTask(*data))
            else:
                task_data['subtask_data_list'].append(TopDownTask(*data))
        return task_data

    def get_kfc_turn_data(self: any) -> dict:
        if not os.path.exists(PathManager.get_db_path(self.project_dir, DBNameConstant.DB_KFC_INFO)):
            return {}
        kfc_turn_data = {}
        with KfcInfoViewModel(
            self.project_dir, [DBNameConstant.TABLE_KFC_COMM_TURN, DBNameConstant.TABLE_KFC_COMPUTE_TURN]
        ) as model:
            kfc_comm_turn_time = model.get_kfc_comm_turn_data()
            kfc_compute_turn_time = model.get_kfc_compute_turn_data()
        kfc_comm_turn_data = []
        kfc_compute_turn_data = []
        for data in kfc_comm_turn_time:
            kfc_comm_turn_data += [
                KfcTurnData(
                    "StartServer {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.server_start_time),
                    InfoConfReader().duration_from_syscnt(data.wait_msg_start_time - data.server_start_time),
                ),
                KfcTurnData(
                    "TaskWaitRequest {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.wait_msg_start_time),
                    InfoConfReader().duration_from_syscnt(data.kfc_alg_exe_start_time - data.wait_msg_start_time),
                ),
                KfcTurnData(
                    "TaskOrchestration {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.kfc_alg_exe_start_time),
                    InfoConfReader().duration_from_syscnt(data.send_task_start_time - data.kfc_alg_exe_start_time),
                ),
                KfcTurnData(
                    "TaskLaunch {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.send_task_start_time),
                    InfoConfReader().duration_from_syscnt(data.send_sqe_finish_time - data.send_task_start_time),
                ),
                KfcTurnData(
                    "TaskExecute {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.send_sqe_finish_time),
                    InfoConfReader().duration_from_syscnt(data.rtsq_exe_end_time - data.send_sqe_finish_time),
                ),
                KfcTurnData(
                    "Finalize {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.rtsq_exe_end_time),
                    InfoConfReader().duration_from_syscnt(data.server_end_time - data.rtsq_exe_end_time),
                ),
            ]
        for data in kfc_compute_turn_time:
            kfc_compute_turn_data += [
                KfcTurnData(
                    "WaitCompute {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.wait_compute_start_time),
                    InfoConfReader().duration_from_syscnt(data.compute_start_time - data.wait_compute_start_time),
                ),
                KfcTurnData(
                    "Compute {}".format(data.current_turn),
                    data.stream_id,
                    data.task_id,
                    InfoConfReader().trans_syscnt_into_local_time(data.compute_start_time),
                    InfoConfReader().duration_from_syscnt(data.compute_exe_end_time - data.compute_start_time),
                ),
            ]
        kfc_turn_data["comm"] = kfc_comm_turn_data
        kfc_turn_data["compute"] = kfc_compute_turn_data
        return kfc_turn_data

    def get_timeline_data(self: any) -> list:
        """
        get model list timeline data
        @return:timeline trace data
        """
        result = []
        result_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)
        # add memory copy data
        memory_copy_viewer = MemoryCopyViewer(result_dir)
        trace_data_memcpy = memory_copy_viewer.get_memory_copy_timeline()
        result.extend(trace_data_memcpy)

        timeline_data = self.get_ascend_task_data()
        trace_tasks = self.get_trace_timeline(timeline_data)
        result.extend(trace_tasks)
        kfc_turn_data = self.get_kfc_turn_data()
        kfc_trace_tasks = self.format_kfc_turn(kfc_turn_data)
        result.extend(kfc_trace_tasks)
        if not result:
            logging.warning(
                "Can not export task time data, the current chip does not support "
                "exporting this data or the data may be not collected."
            )
            return []
        return result

    def get_trace_timeline(self: any, data_list: dict) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        if not data_list or not any(data_list.values()):
            return []
        self.add_node_name_and_type(data_list)
        if self.params.get("data_type") == ExportDataType.FFTS_SUB_TASK_TIME.name.lower():
            self.add_thread_id(data_list)
            return self.format_ffts_sub_task_data(data_list)
        return self.format_task_scheduler(data_list)

    def format_task_type_data(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [
                    data.op_name,
                    self.trace_pid_map.get("Subtask Time", 1),
                    StarsConstant().find_key_by_value(data.device_task_type),
                    InfoConfReader().trans_into_local_time(data.start_time),
                    data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                    {
                        "Physic Stream Id": data.stream_id,
                        "Task Id": data.task_id,
                        'Batch Id': data.batch_id,
                        "Subtask Id": data.context_id,
                        "connection_id": data.connection_id,
                    },
                ]
            )
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result_list)
        result = TraceViewManager.metadata_event(
            self.get_time_timeline_header(result_list, pid_header=TraceViewHeaderConstant.PROCESS_SUBTASK)
        )
        result.extend(_trace)
        return result

    def format_thread_task_data(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [
                    data.op_name,
                    self.trace_pid_map.get("Thread Task Time", 2),
                    data.thread_id * (max(StarsConstant.SUBTASK_TYPE) + 1)
                    + StarsConstant().find_key_by_value(data.device_task_type),
                    InfoConfReader().trans_into_local_time(data.start_time),
                    data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                    {
                        "Physic Stream Id": data.stream_id,
                        "Task Id": data.task_id,
                        'Batch Id': data.batch_id,
                        "Subtask Id": data.context_id,
                        "Subtask Type": data.task_type,
                        "connection_id": data.connection_id,
                    },
                ]
            )
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result_list)
        result = TraceViewManager.metadata_event(
            self.get_time_timeline_header(result_list, pid_header=TraceViewHeaderConstant.PROCESS_THREAD_TASK)
        )
        result.extend(_trace)
        return result

    def format_task_scheduler(self, data_list):
        result_list = []
        keys = ["subtask_data_list", "task_data_list"]
        for key in keys:
            for data in data_list.get(key, []):
                result_list.append(
                    [
                        data.op_name,
                        self.trace_pid_map.get("Task Scheduler", 0),
                        GeLogicStreamSingleton().get_logic_stream_id(data.stream_id),
                        InfoConfReader().trans_into_local_time(data.start_time),
                        data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                        {
                            "Model Id": data.model_id,
                            "Task Type": data.task_type,
                            "Physic Stream Id": data.stream_id,
                            "Task Id": data.task_id,
                            'Batch Id': data.batch_id,
                            "Subtask Id": data.context_id,
                            "connection_id": data.connection_id,
                        },
                    ]
                )
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result_list)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result_list))
        result.extend(_trace)
        return result

    def format_kfc_turn(self, kfc_turn_data: dict) -> list:
        result_list = []
        for kfc_turn_value in kfc_turn_data.values():
            for data in kfc_turn_value:
                result_list.append(
                    [
                        data.op_name,
                        self.trace_pid_map.get(TraceViewHeaderConstant.PROCESS_TASK, 0),
                        GeLogicStreamSingleton().get_logic_stream_id(data.stream_id),
                        data.start_time,
                        data.duration if data.duration > 0 else 0,
                        {
                            "Physic Stream Id": data.stream_id,
                            "Task Id": data.task_id,
                        },
                    ]
                )
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result_list)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result_list))
        result.extend(_trace)
        return result

    def format_ffts_sub_task_data(self, data_list):
        return self.format_thread_task_data(data_list) + self.format_task_type_data(data_list)

    def add_thread_id(self: any, data_dict: dict) -> None:
        with SubTaskTimeModel(self.project_dir) as model:
            subtask_data = model.get_all_data(DBNameConstant.TABLE_SUBTASK_TIME, TaskTimeDto)
        thread_id_dict = {}
        for data in subtask_data:
            key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.subtask_id, data.start_time)
            thread_id_dict[key] = data.thread_id
        for data in data_dict.get("subtask_data_list", []):
            thread_id_key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.context_id, data.start_time)
            setattr(data, 'thread_id', thread_id_dict.get(thread_id_key, 0))  # 给个默认值 避免后面使用thread_id计算出错

    def add_node_name_and_type(self: any, data_dict: dict) -> None:
        with OpSummaryModel({"result_dir": self.params.get("project")}) as _model:
            node_info_dict = _model.get_op_name_from_ge_by_id()
        ffts_plus_set = set()
        for data in data_dict.get("subtask_data_list", []):
            ffts_plus_set.add(
                "{0}-{1}-{2}-{3}".format(
                    data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID, data.batch_id
                )
            )
            node_key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.context_id, data.batch_id)
            self._update_op_name_and_type(data, node_info_dict, node_key)
        tradition_list = []
        for data in data_dict.get("task_data_list", []):
            node_key = "{0}-{1}-{2}-{3}".format(
                data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID, data.batch_id
            )
            if node_key not in ffts_plus_set:
                self._update_op_name_and_type(data, node_info_dict, node_key)
                tradition_list.append(data)
        data_dict["task_data_list"] = tradition_list
