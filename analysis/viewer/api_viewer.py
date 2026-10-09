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
from typing import List

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.api.api_data_viewer_model import ApiDataViewModel
from msmodel.event.event_data_viewer_model import EventDataViewModel
from profiling_bean.db_dto.api_data_dto import ApiDataDtoTuple


class ApiViewer:
    """
    Viewer for api data
    """

    ACL_LEVEL = 'acl'
    HCCL_LEVEL = 'communication'
    MODEL_LOAD = 'ModelLoad'

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._api_model = ApiDataViewModel(params)
        self._event_model = EventDataViewModel(params)

    @staticmethod
    def _get_api_result_data(timeline_data: list, pid: str, tid: str) -> list:
        result_data = []
        tid_values = set()
        for sql_data in timeline_data:
            tid_values.add(sql_data[3])

        meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_API]]
        meta_data.extend(["thread_name", pid, tid_value, f"Thread {tid_value}"] for tid_value in tid_values)
        meta_data.extend(["thread_sort_index", pid, tid_value, tid_value] for tid_value in tid_values)
        result_data.extend(TraceViewManager.metadata_event(meta_data))
        return result_data

    @staticmethod
    def _api_reformat(api_data: list) -> list:
        return [
            (
                data[0],
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data[1], NumberConstant.MICRO_SECOND),
                    use_us=True,
                    is_host=True,
                ),
                InfoConfReader().get_host_duration(data[2], NumberConstant.MICRO_SECOND),
                *data[3:],
            )
            for data in api_data
        ]

    @staticmethod
    def _event_reformat(event_data: List[ApiDataDtoTuple]) -> list:
        return [
            (
                data.struct_type,
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data.start, NumberConstant.MICRO_SECOND),
                    use_us=True,
                    is_host=True,
                ),
                InfoConfReader().get_host_duration((data.end - data.start), NumberConstant.MICRO_SECOND),
                data.thread_id,
                data.level,
                data.id,
                data.item_id,
                data.connection_id,
            )
            for data in event_data
        ]

    @staticmethod
    def _check_range_time(record: list, model_load_data: dict, start_time):
        float_start = float(record[1])
        if record[3] not in model_load_data.keys():
            return float_start >= start_time
        for model_data in model_load_data[record[3]]:
            if float_start >= start_time or model_data[0] <= float_start <= model_data[1]:
                return True
        return False

    @staticmethod
    def _get_data_api_name(api_data: list) -> str:
        # api_data[0]: struct_type    api_data[5]: id    api_data[6]: item_id
        level = api_data[4]
        if level == ApiViewer.ACL_LEVEL:
            api_name = str(api_data[5]) if str(api_data[5]) else Constant.NA
        elif level == ApiViewer.HCCL_LEVEL:
            api_name = str(api_data[6]) if str(api_data[6]) else Constant.NA
        else:
            api_name = str(api_data[0]) if str(api_data[0]) else Constant.NA
        return api_name

    @staticmethod
    def _get_api_data(timeline_data: list, pid: str) -> list:
        trace_data = []
        for sql_data in timeline_data:
            args = OrderedDict([('Thread Id', sql_data[3])])
            args.setdefault("Mode", sql_data[0])
            args.setdefault("level", sql_data[4])
            args.setdefault("id", sql_data[5])
            args.setdefault("item_id", sql_data[6])
            args.setdefault("connection_id", sql_data[7])
            trace_data.append(
                (ApiViewer._get_data_api_name(sql_data), pid, sql_data[3], sql_data[1], sql_data[2], args)
            )
        return trace_data

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from api data
        :return:
        """
        timeline_data = []
        with self._api_model as _model:
            if _model.check_db() and _model.check_table():
                timeline_data.extend(self._api_reformat(_model.get_timeline_data()))

        with self._event_model as _model:
            if _model.check_db() and _model.check_table():
                timeline_data.extend(self._event_reformat(_model.get_timeline_data()))

        timeline_data = self._filter_api_data(timeline_data)

        if not timeline_data:
            logging.warning("Unable to get api or event data.")
            return []

        pid = InfoConfReader().get_json_pid_data()
        tid = InfoConfReader().get_json_tid_data()
        result_data = self._get_api_result_data(timeline_data, pid, tid)
        trace_data = self._get_api_data(timeline_data, pid)
        result_data.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data)
        )
        return result_data

    def _filter_api_data(self, timeline_data: list):
        start_ts_float = float(InfoConfReader().get_collect_time()[0])
        timeline_data.sort(key=lambda x: float(x[1]))  # 1 api.start
        filter_data = [[]] * len(timeline_data)
        index = 0
        model_load_data = dict()
        for record in timeline_data:
            if record[0] == self.MODEL_LOAD:
                model_data = model_load_data.get(record[3], [])
                model_data.append([float(record[1]), float(record[1]) + float(record[2])])
                model_load_data[record[3]] = model_data
            if not self._check_range_time(record, model_load_data, start_ts_float):
                continue
            filter_data[index] = record
            index += 1
        return filter_data[:index]
