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

from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.ge_logic_stream_singleton import GeLogicStreamSingleton
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.memory_copy.memcpy_model import MemcpyModel


class MemoryCopyViewer:
    """
    memory copy viewer
    """

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._model = MemcpyModel(
            self._project_path, DBNameConstant.DB_MEMORY_COPY, DBNameConstant.TABLE_TS_MEMCPY_CALCULATION
        )
        self._pid = InfoConfReader().get_json_pid_data()

    @staticmethod
    def add_runtime_memcpy_args(sql_data: list, args: dict) -> dict:
        """
        if runtime belongs to memcpy, add direction, data size, stream id, task id to args
        sql index: 0 api name, 1 start time, 2 duration, 3 pid, 4 data size, 5 direction,
        6 stream id, 7 task id
        params: sql_data: runtime data
        params: args: timeline args
        return: updated timeline args
        """
        direction = MemoryCopyConstant.get_direction(sql_data[5])
        # if data size is not zero and direction is not 'other', the runtime belongs to memcpy
        if sql_data[4] and direction != MemoryCopyConstant.DEFAULT_NAME:
            args.setdefault("data size", "{} byte".format(sql_data[4]))
            args.setdefault("memcpy direction", "{}".format(direction))

            task_id = sql_data[7]
            # if task id is not '', it is async
            if task_id:
                args.setdefault("stream id", sql_data[6])
                args.setdefault("task id", sql_data[7])

        return args

    def get_memory_copy_chip0_summary(self: any) -> list:
        """
        get memory copy summary data when chip type is 0.
        datum index: 0 sum duration of start to end; 1 task type; 2 task id;
        3 stream id; 4 duration of receive to start; 5 avg duration of start to end;
        6 min duration of start to end; 7 max duration of start to end; 8 count
        return: None
        """
        summary_data = []
        if self._model.check_db():
            export_data = self._model.return_chip0_summary(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)

            for datum in export_data:
                default_time_ratio = 0
                sum_val = datum[0]
                running = datum[0]
                waiting = datum[4]
                avg_val = datum[5]
                min_val = datum[6]
                max_val = datum[7]
                task_count = datum[8]
                pending = MemoryCopyConstant.DEFAULT_VIEWER_VALUE
                op_name = MemoryCopyConstant.DEFAULT_VIEWER_VALUE

                summary_data.append(
                    (
                        default_time_ratio,
                        sum_val,
                        task_count,
                        avg_val,
                        min_val,
                        max_val,
                        waiting,
                        running,
                        pending,
                        datum[1],
                        StrConstant.AYNC_MEMCPY,
                        datum[2],
                        op_name,
                        datum[3],
                    )
                )
        return summary_data

    def get_memory_copy_non_chip0_summary(self: any) -> list:
        """
        get memory copy summary data when chip type is not 0.
        datum index: 0 name; 1 task type; 2 stream_id;
        3 task_id; 4 duration; 5 start; 6 end
        task time, task start, task stop should be str,
        because they are too long to show in excel
        return: None
        """
        summary_data = []
        if self._model.check_db():
            export_data = self._model.return_not_chip0_summary(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)

            for datum in export_data:
                export_datum = []
                task_start = format_high_precision_for_csv(InfoConfReader().trans_into_local_time(datum[5], True))
                task_stop = format_high_precision_for_csv(InfoConfReader().trans_into_local_time(datum[6], True))
                export_datum.append(datum[0])
                export_datum.append(datum[1])
                export_datum.append(datum[2])
                export_datum.append(datum[3])
                export_datum.append(round(datum[4], NumberConstant.ROUND_THREE_DECIMAL))
                export_datum.append(task_start)
                export_datum.append(task_stop)
                summary_data.append(tuple(export_datum))

        return summary_data

    def get_memory_copy_timeline(self: any) -> list:
        """
        get memory copy timeline
        datum index: 0 name; 1 type; 2 receive time;
        3 start time; 4 end time; 5 duration; 6 stream id; 7 task id
        """
        timeline_data = []
        if self._model.check_db():
            export_data = self._model.return_task_scheduler_timeline(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)
            for datum in export_data:
                start_time = InfoConfReader().trans_into_local_time(raw_timestamp=datum[3], use_us=True)
                end_time = InfoConfReader().trans_into_local_time(raw_timestamp=datum[4], use_us=True)
                receive_time = InfoConfReader().trans_into_local_time(raw_timestamp=datum[2], use_us=True)
                args = OrderedDict(
                    [
                        ("Task Type", datum[1]),
                        ("Stream Id", datum[6]),
                        ("Task Id", datum[7]),
                        ("Receive Time", receive_time),
                        ("Start Time", start_time),
                        ("End Time", end_time),
                    ]
                )

                logic_stream_id = GeLogicStreamSingleton().get_logic_stream_id(datum[6])
                timeline_datum = [datum[0], self._pid, logic_stream_id, start_time, datum[5], args]
                timeline_data.append(timeline_datum)

        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, timeline_data)
