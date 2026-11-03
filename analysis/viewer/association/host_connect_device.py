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

from collections import defaultdict
from enum import Enum
from typing import Any, Dict, List, Set, Tuple

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import path_check, is_number
from common_func.path_manager import PathManager
from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_manager import TraceViewManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from msmodel.interface.view_model import ViewModel


class FlowCategory(Enum):
    """
    host侧api与device侧task连线的类别。连线名统一为 HostToDevice，非默认类别追加首字母大写括号后缀。
    """

    DEFAULT = "DEFAULT"  # 不带后缀，直接输出 HostToDevice
    KERNEL = "KERNEL"  # HostToDevice(Kernel)
    GRAPH = "GRAPH"  # HostToDevice(Graph)

    def get_flow_name(self) -> str:
        """返回该类别对应的连线名（cat 与 name 前缀），常量直取，不做运行时拼接"""
        return FLOW_CATEGORY_NAME.get(self.value, StrConstant.HOST_TO_DEVICE)


FLOW_CATEGORY_NAME = {
    FlowCategory.KERNEL.value: f'{StrConstant.HOST_TO_DEVICE}(Kernel)',
    FlowCategory.GRAPH.value: f'{StrConstant.HOST_TO_DEVICE}(Graph)',
}


class HostToDevice:
    """Connect CANN Node@launch api to corresponding device tasks/HCCL OP."""

    API_TYPE = 'api'
    MODULE_MSPROFTX = 'msprof_tx'
    MODULE_TASK_TIME = 'task_time'
    MODULE_HCCL = 'communication'
    NODE_LAUNCH = "Node@launch"

    def __init__(self, result_dir: str) -> None:
        self._result_dir = result_dir
        # 没有api数据时没必要增加连线
        self.exist_api = False
        self._acl_event_apis = {}
        self._memcpy_async_ids = defaultdict(list)
        # MODEL_EXECUTE 的 connection_id -> (thread_id, ts)，来自 AscendTask 表
        self._model_execute_apis = {}
        # 已生成过连线的 flow id，避免 host/device 两次导出重复添加
        self._model_execute_added = set()

    @staticmethod
    def is_node_launch(api_trace: Dict[str, Any]) -> bool:
        """
        check if some trace is the start of flow event, that is, it's Node@launch
        :param api_trace: api trace as json
        :return: bool
        """
        return api_trace.get("name") == HostToDevice.NODE_LAUNCH

    @staticmethod
    def is_hccl_trace(api_trace: Dict[str, Any], hccl_conn_ids: Set[int]) -> bool:
        connection_id = api_trace.get("args", {}).get("connection_id", Constant.DEFAULT_INVALID_VALUE)
        return connection_id in hccl_conn_ids

    @staticmethod
    def get_cann_pid():
        pid = InfoConfReader().get_json_pid_data()
        # Connection 字段实际未用到
        layer_info = TraceViewHeaderConstant.LayerInfo(
            "Connection", TraceViewHeaderConstant.GENERAL_LAYER_CPU, TraceViewHeaderConstant.LAYER_CANN_SORT
        )
        format_pid = TraceViewManager.get_format_pid(pid, layer_info)
        return format_pid

    @staticmethod
    def get_start_points(api_trace: Dict[str, Any], conn_to_ctxes: Dict[int, List[int]]) -> List[Dict[str, Any]]:
        """
        calculate start points of host to device connection for a single api trace
        :param api_trace: api trace as json
        :param conn_to_ctxes: connection id to ctx_ids map
        :return: start point
        """
        start_time = api_trace.get('ts', '0')
        connection_id = api_trace.get("args", {}).get("connection_id", Constant.DEFAULT_INVALID_VALUE)
        context_ids = conn_to_ctxes.get(connection_id, [Constant.DEFAULT_INVALID_VALUE])
        return [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'HostToDevice{(connection_id << 32) + ctx_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                TraceViewHeaderConstant.TRACE_HEADER_CAT: StrConstant.HOST_TO_DEVICE,
                TraceViewHeaderConstant.TRACE_HEADER_ID: str((connection_id << 32) + ctx_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID),
                TraceViewHeaderConstant.TRACE_HEADER_TID: api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID),
                TraceViewHeaderConstant.TRACE_HEADER_TS: start_time,
            }
            for ctx_id in context_ids
        ]

    @staticmethod
    def add_task_connection_data(
        traces: List[Dict[str, Any]],
        cann_pid: int,
        node_tasks: Dict[Tuple[int, int, int, int], Tuple[int, int]],
        device_id: int,
        acl_event_apis: Dict[str, Any],
    ) -> None:
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            if trace_args.get("Task Type") == StrConstant.MODEL_EXECUTE_TASK_TYPE:
                # MODEL_EXECUTE 由 add_model_execute_connection_data 单独处理，避免重复连线
                continue
            stream_id = trace_args.get("Physic Stream Id")
            task_id = trace_args.get("Task Id")
            batch_id = trace_args.get("Batch Id")
            context_id: int = trace_args.get("Subtask Id", Constant.DEFAULT_INVALID_VALUE)
            connection_id = trace_args.get("connection_id", Constant.DEFAULT_INVALID_VALUE)
            task_data = node_tasks.get((device_id, stream_id, task_id, batch_id), None)
            if task_data is not None:
                host_task_tid, host_task_ts = task_data
                host_task_ts = InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(host_task_ts, NumberConstant.MICRO_SECOND),
                    use_us=True,
                    is_host=True,
                )
            elif connection_id in acl_event_apis:
                api_trace = acl_event_apis[connection_id]
                host_task_tid = api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID)
                host_task_ts = api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS)
            else:
                continue
            pid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID)
            tid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID)

            # 由于同一个Node下面可能出现多个Task，使用device_id、 stream_id、task_id、batch_id、context_id来作为连线的唯一标识
            # |---6bit--|---16bit--|---16bit---|---16bit---|---32bit---|
            #  device_id  stream_id   task_id     batch_id   context_id
            connection_id = (device_id << 80) + (stream_id << 64) + (task_id << 48) + (batch_id << 32) + context_id
            connect_start = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'{FlowCategory.DEFAULT.get_flow_name()}{connection_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.DEFAULT.get_flow_name(),
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: cann_pid,
                TraceViewHeaderConstant.TRACE_HEADER_TID: host_task_tid,
                TraceViewHeaderConstant.TRACE_HEADER_TS: host_task_ts,
            }
            connect_end = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'{FlowCategory.DEFAULT.get_flow_name()}{connection_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 'f',
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
                TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.DEFAULT.get_flow_name(),
                TraceViewHeaderConstant.TRACE_HEADER_PID: pid,
                TraceViewHeaderConstant.TRACE_HEADER_TID: tid,
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
            tmp_list.append(connect_start)
            tmp_list.append(connect_end)
        traces.extend(tmp_list)

    @staticmethod
    def add_model_execute_connection_data(
        traces: List[Dict[str, Any]],
        cann_pid: int,
        model_execute_apis: Dict[int, Dict[str, Any]],
        added_ids: Set[int],
    ) -> None:
        """
        为 MODEL_EXECUTE task 添加连线：起点取 aclmdlRIExecuteAsync 的时间，终点取 task 自身时间。
        MODEL_EXECUTE 的 task 与 api 通过 connection_id 一一对应，id 统一使用 connection_id << 32。
        :param traces: task time traces as json list
        :param cann_pid: CANN 进程的 format pid
        :param model_execute_apis: connection_id -> api trace
        :param added_ids: 已生成过连线的 flow id 集合，用于幂等去重
        :return: None
        """
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            # 同一 connection_id 会被调用树下多个 device task 共享，必须先按 task 类型过滤，
            # 否则先到的 kernel trace 会抢占 flow id，把终点画到错误的 task 上
            if trace_args.get("Task Type") != StrConstant.MODEL_EXECUTE_TASK_TYPE:
                continue
            connection_id = trace_args.get("connection_id", Constant.DEFAULT_INVALID_VALUE)
            if connection_id == Constant.DEFAULT_INVALID_VALUE or connection_id not in model_execute_apis:
                continue
            flow_id = connection_id << 32
            # add_connect_line 在 host/device 两次 _view_data 中都会被调用，这里做幂等保护
            if flow_id in added_ids:
                continue
            added_ids.add(flow_id)
            api_trace = model_execute_apis[connection_id]
            connect_start = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'{FlowCategory.GRAPH.get_flow_name()}{flow_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.GRAPH.get_flow_name(),
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(flow_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: cann_pid,
                TraceViewHeaderConstant.TRACE_HEADER_TID: api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID),
                TraceViewHeaderConstant.TRACE_HEADER_TS: api_trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
            }
            connect_end = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'{FlowCategory.GRAPH.get_flow_name()}{flow_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 'f',
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(flow_id),
                TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.GRAPH.get_flow_name(),
                TraceViewHeaderConstant.TRACE_HEADER_TS: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
                TraceViewHeaderConstant.TRACE_HEADER_PID: trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID),
                TraceViewHeaderConstant.TRACE_HEADER_TID: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID),
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
            tmp_list.append(connect_start)
            tmp_list.append(connect_end)
        traces.extend(tmp_list)

    @staticmethod
    def add_hccl_end_points(traces: List[Dict[str, Any]]) -> None:
        """
        add end points for host to device connection
        :param traces: hccl traces as json list
        :return: None
        """
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            connection_id = trace_args.get('connection_id', Constant.DEFAULT_INVALID_VALUE)
            if connection_id == Constant.DEFAULT_INVALID_VALUE:
                continue
            context_id: int = trace_args.get("Subtask Id", Constant.DEFAULT_INVALID_VALUE)
            pid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID)
            tid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID)
            connect_dict = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'HostToDevice{(connection_id << 32) + context_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 'f',
                TraceViewHeaderConstant.TRACE_HEADER_ID: str((connection_id << 32) + context_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
                TraceViewHeaderConstant.TRACE_HEADER_CAT: StrConstant.HOST_TO_DEVICE,
                TraceViewHeaderConstant.TRACE_HEADER_PID: pid,
                TraceViewHeaderConstant.TRACE_HEADER_TID: tid,
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
            tmp_list.append(connect_dict)
        traces.extend(tmp_list)

    @staticmethod
    def add_msproftx_ex_start_points(traces: List[Dict[str, Any]]) -> None:
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            mark_id = trace_args.get('mark_id', NumberConstant.UINT64_MAX)
            if mark_id == NumberConstant.UINT64_MAX:
                continue
            del trace_args['mark_id']
            pid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID)
            tid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID)
            connect_dict = {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'MsTx_{mark_id}',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(mark_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
                TraceViewHeaderConstant.TRACE_HEADER_CAT: StrConstant.MSTX,
                TraceViewHeaderConstant.TRACE_HEADER_PID: pid,
                TraceViewHeaderConstant.TRACE_HEADER_TID: tid,
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
            tmp_list.append(connect_dict)
        traces.extend(tmp_list)

    @staticmethod
    def add_memcpy_async_start_points(api_traces: List[Dict[str, Any]], memcpy_async_ids: Dict[int, List[int]]) -> None:
        if not isinstance(api_traces, list):
            return
        tmp_list = []
        for api_trace in api_traces:
            if api_trace.get("args", {}).get("connection_id") in memcpy_async_ids:
                start_time = api_trace.get('ts', '0')
                connection_id = api_trace.get("args", {}).get("connection_id", Constant.DEFAULT_INVALID_VALUE)
                context_ids = memcpy_async_ids.get(connection_id, [Constant.DEFAULT_INVALID_VALUE])
                tmp_list.extend(
                    [
                        {
                            TraceViewHeaderConstant.TRACE_HEADER_NAME: (
                                f'{FlowCategory.DEFAULT.get_flow_name()}{(connection_id << 32) + ctx_id}'
                            ),
                            TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                            TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.DEFAULT.get_flow_name(),
                            TraceViewHeaderConstant.TRACE_HEADER_ID: str((connection_id << 32) + ctx_id),
                            TraceViewHeaderConstant.TRACE_HEADER_PID: api_trace.get(
                                TraceViewHeaderConstant.TRACE_HEADER_PID
                            ),
                            TraceViewHeaderConstant.TRACE_HEADER_TID: api_trace.get(
                                TraceViewHeaderConstant.TRACE_HEADER_TID
                            ),
                            TraceViewHeaderConstant.TRACE_HEADER_TS: start_time,
                        }
                        for ctx_id in context_ids
                    ]
                )
        api_traces.extend(tmp_list)

    @staticmethod
    def add_memcpy_async_end_points(traces: List[Dict[str, Any]], memcpy_async_ids: Dict[int, List[int]]) -> None:
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            if trace.get("name") == 'MEMCPY_ASYNC' and trace.get("args", {}).get("connection_id") in memcpy_async_ids:
                trace_args = trace.get('args', {})
                connection_id = trace_args.get('connection_id', Constant.DEFAULT_INVALID_VALUE)
                if connection_id == Constant.DEFAULT_INVALID_VALUE:
                    continue
                context_id: int = trace_args.get("Subtask Id", Constant.DEFAULT_INVALID_VALUE)
                pid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_PID)
                tid = trace.get(TraceViewHeaderConstant.TRACE_HEADER_TID)
                connection_id = (connection_id << 32) + context_id
                connect_dict = {
                    TraceViewHeaderConstant.TRACE_HEADER_NAME: f'{FlowCategory.DEFAULT.get_flow_name()}{connection_id}',
                    TraceViewHeaderConstant.TRACE_HEADER_PH: 'f',
                    TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                    TraceViewHeaderConstant.TRACE_HEADER_TS: trace.get(TraceViewHeaderConstant.TRACE_HEADER_TS),
                    TraceViewHeaderConstant.TRACE_HEADER_CAT: FlowCategory.DEFAULT.get_flow_name(),
                    TraceViewHeaderConstant.TRACE_HEADER_PID: pid,
                    TraceViewHeaderConstant.TRACE_HEADER_TID: tid,
                    TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
                }
                tmp_list.append(connect_dict)
        traces.extend(tmp_list)

    def get_acl_event_trace(self, api_traces: List[Dict[str, Any]]) -> None:
        for api_trace in api_traces:
            if (
                api_trace.get("name") == StrConstant.ACL_RECORD_EVENT
                or api_trace.get("name") == StrConstant.ACL_WAIT_EVENT
            ):
                connection_id = api_trace.get("args", {}).get("connection_id")
                self._acl_event_apis[connection_id] = api_trace
            elif api_trace.get("name") == StrConstant.ACL_MODEL_EXECUTE:
                connection_id = api_trace.get("args", {}).get("connection_id")
                # connection_id 缺失（None）或非法时不入表，避免写入无效键
                if connection_id is not None and connection_id != Constant.DEFAULT_INVALID_VALUE:
                    self._model_execute_apis[connection_id] = api_trace

    def add_hccl_start_points(
        self, api_traces: List[Dict[str, Any]], conn_to_ctxes: Dict[int, List[int]], hccl_conn_ids: Set[int]
    ) -> None:
        """
        add start points to api traces for host to device connection
        to do this, we need task info from host side
        this is bad design BTW
        :param api_traces: api traces as json list
        :param conn_to_ctxes: connection id to ctx_ids map
        :param hccl_conn_ids: hccl ops connection id set
        :return: None
        """
        if not isinstance(api_traces, list):
            return
        tmp_list = []
        for api_trace in api_traces:
            # only add start point for hccl op
            if HostToDevice.is_node_launch(api_trace) and HostToDevice.is_hccl_trace(api_trace, hccl_conn_ids):
                start_point = self.get_start_points(api_trace, conn_to_ctxes)
                tmp_list.extend(start_point)
        api_traces.extend(tmp_list)

    def add_connect_line(self, traces: List[Dict[str, Any]], data_type: str) -> None:
        """
        为Host task和HCCL OP添加连线：
        1.对于Host Task数据（data_type == MODULE_TASK_TIME）时添加连线的起点和中终点，起点为实际Host task的开始时间
        2.对于HCCL OP在data_type为API_TYPE时添加连线的起点，data_type为API_TYPE时添加连线的终点
        :param traces: json traces
        :param data_type: export type
        """
        if data_type == self.MODULE_MSPROFTX:
            self.add_msproftx_ex_start_points(traces)
            return
        device_id = InfoConfReader().get_device_id()
        if not is_number(device_id):
            return
        device_id = int(device_id)
        node_tasks = self.get_node_tasks()
        if data_type == self.MODULE_TASK_TIME:
            if not self.exist_api:
                return
            cann_pid = self.get_cann_pid()
            self.add_task_connection_data(traces, cann_pid, node_tasks, device_id, self._acl_event_apis)
            self.add_model_execute_connection_data(
                traces, cann_pid, self._model_execute_apis, self._model_execute_added
            )
            self.add_memcpy_async_end_points(traces, self._memcpy_async_ids)
        elif data_type == self.API_TYPE:
            self.exist_api = True
            hccl_conn_ids = self.get_hccl_op_connection_ids()
            conn_to_ctxes = self.get_connection_id_to_context_ids_mapping(node_tasks, device_id)
            self.get_acl_event_trace(traces)
            self.add_hccl_start_points(traces, conn_to_ctxes, hccl_conn_ids)
            self.add_memcpy_async_start_points(traces, self._memcpy_async_ids)
        elif data_type == self.MODULE_HCCL:
            self.add_hccl_end_points(traces)

    def get_node_tasks(self) -> Dict[Tuple[int, int, int, int], Tuple[int, int]]:
        """
        get node tasks set
        :return: node tasks set
        """
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_GE_INFO)):
            return {}
        with ViewModel(self._result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK]) as task_info_model:
            sql = (
                f'select device_id, stream_id, task_id, batch_id, thread_id, timestamp '
                f'from {DBNameConstant.TABLE_GE_TASK}'
            )
            tasks = task_info_model.get_sql_data(sql)
        return {task[:4]: task[-2:] for task in tasks}

    def get_connection_id_to_context_ids_mapping(
        self, node_tasks: Dict[Tuple[int, int, int, int], Tuple[int, int]], device_id: int
    ):
        """
        get device tasks
        :return: device tasks
        """
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_ASCEND_TASK)):
            return {}
        ascend_task_model = ViewModel(
            self._result_dir, DBNameConstant.DB_ASCEND_TASK, [DBNameConstant.TABLE_ASCEND_TASK]
        )
        ascend_task_model.init()
        sql = 'select stream_id, task_id, batch_id, context_id, connection_id, host_task_type from AscendTask'
        ascend_tasks = ascend_task_model.get_sql_data(sql)
        result = defaultdict(list)
        for stream_id, task_id, batch_id, context_id, connection_id, host_task_type in ascend_tasks:
            if host_task_type == 'MEMCPY_ASYNC':
                self._memcpy_async_ids[connection_id].append(context_id)
            if host_task_type == StrConstant.MODEL_EXECUTE_TASK_TYPE:
                # MODEL_EXECUTE 单独走 ModelExecute 连线，不再归入 HostToDevice
                continue
            if (device_id, stream_id, task_id, batch_id) not in node_tasks:
                continue
            result[connection_id].append(context_id)
        return result

    def get_hccl_op_connection_ids(self):
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            return set()
        with ViewModel(
            self._result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]
        ) as hccl_model:
            if not hccl_model.check_table():
                return set()
            sql = f"select distinct op_id from {DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE}"
            connection_ids = hccl_model.get_sql_data(sql)

        return set(conn_id[0] for conn_id in connection_ids)
