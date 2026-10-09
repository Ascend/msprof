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

import logging
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.event.event_data_viewer_model import EventDataViewModel


class EventViewer:
    """
    Viewer for api data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = EventDataViewModel(params)

    @staticmethod
    def _get_event_result_data(timeline_data: list, pid: str, tid: str) -> list:
        result_data = []
        tid_values = set()
        for timeline_data_dto in timeline_data:
            tid_values.add(timeline_data_dto.thread_id)

        meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_EVENT]]
        meta_data.extend(["thread_name", pid, tid_value, f"Thread {tid_value}"] for tid_value in tid_values)
        meta_data.extend(["thread_sort_index", pid, tid_value, tid_value] for tid_value in tid_values)
        result_data.extend(TraceViewManager.metadata_event(meta_data))
        return result_data

    @staticmethod
    def _get_event_data(timeline_data: list, pid: str) -> list:
        trace_data = []
        for timeline_data_dto in timeline_data:
            struct_type = str(timeline_data_dto.struct_type) if str(timeline_data_dto.struct_type) else Constant.NA
            args = OrderedDict([('Thread Id', timeline_data_dto.thread_id)])
            args.setdefault("level", timeline_data_dto.level)
            args.setdefault("id", timeline_data_dto.id)
            args.setdefault("item_id", timeline_data_dto.item_id)
            args.setdefault("request_id", timeline_data_dto.request_id)
            args.setdefault("connection_id", timeline_data_dto.connection_id)
            trace_data.append(
                (
                    struct_type,
                    pid,
                    timeline_data_dto.thread_id,
                    InfoConfReader().trans_into_local_time(
                        InfoConfReader().time_from_host_syscnt(timeline_data_dto.start, NumberConstant.MICRO_SECOND),
                        use_us=True,
                        is_host=True,
                    ),
                    InfoConfReader().get_host_duration(
                        (timeline_data_dto.end - timeline_data_dto.start), NumberConstant.MICRO_SECOND
                    ),
                    args,
                )
            )
        return trace_data

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from event data
        :return:
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Failed to connect %s", DBNameConstant.DB_API_EVENT)
                return []
            timeline_data = _model.get_timeline_data()
            if not timeline_data:
                logging.warning("Unable to get event data.")
                return []
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            result_data = self._get_event_result_data(timeline_data, pid, tid)
            trace_data = self._get_event_data(timeline_data, pid)
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data)
            )
            return result_data
