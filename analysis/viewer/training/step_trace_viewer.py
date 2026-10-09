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
import sqlite3
from collections import OrderedDict
from functools import wraps

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number, format_high_precision_for_csv, float_calculate
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from profiling_bean.db_dto.step_trace_dto import IterationRange


def catch_exception(fun: any) -> any:
    """
    catch exception
    :param fun: function
    :return: function
    """

    @wraps(fun)
    def wrapper(trace_data: list) -> any:
        """
        wrapper function
        :param trace_data: trace data
        :return: any
        """
        try:
            res = fun(trace_data)
            return res
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            res = None
            return res
        finally:
            res = None

    return wrapper


class TimeLineJsonMaker:
    """
    make training trace timeline json
    """

    @staticmethod
    def create_trace_parm(trace_parm: dict, data: list) -> None:
        """
        create trace parm
        :param trace_parm: trace parm
        :param data: data
        :return: void
        """
        trace_parm[StepTraceConstant.ITER_ID] = data[0]
        trace_parm[StepTraceConstant.FORWARD_PROPAGATION] = data[1]
        trace_parm[StepTraceConstant.BACK_PROPAGATION] = data[2]
        trace_parm[StepTraceConstant.STEP_END] = data[3]
        trace_parm[StepTraceConstant.ITER_TIME] = data[4]
        trace_parm[StepTraceConstant.FORWARD_TO_BACK] = data[5]
        trace_parm[StepTraceConstant.ITERATION_REFRESH] = data[6]
        trace_parm[StepTraceConstant.DATA_AUGMENTATION] = data[7] if is_number(data[7]) else 0

    @staticmethod
    def make_iter_time(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make iter time
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return [
            "Iteration {}".format(trace_parm[StepTraceConstant.ITER_ID]),
            pid,
            tid,
            trace_parm.get(StepTraceConstant.STEP_END) - trace_parm.get(StepTraceConstant.ITER_TIME),
            trace_parm.get(StepTraceConstant.ITER_TIME),
            OrderedDict(
                [
                    ("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                    ("FP Start", trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
                    ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                    ("Iteration End", trace_parm[StepTraceConstant.STEP_END]),
                    ("Iteration Time(us)", trace_parm.get(StepTraceConstant.ITER_TIME)),
                ]
            ),
            "Iteration Time",
        ]

    @staticmethod
    def make_fp_bp_data(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make fp bp data
        :param trace_parm: data parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return [
            "FP_BP Time {}".format(trace_parm[StepTraceConstant.ITER_ID]),
            pid,
            tid,
            trace_parm[StepTraceConstant.FORWARD_PROPAGATION],
            (trace_parm[StepTraceConstant.BACK_PROPAGATION] - trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
            OrderedDict(
                [
                    ("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                    ("FP Start", trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
                    ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                    ("FP_BP Time(us)", trace_parm.get(StepTraceConstant.FORWARD_TO_BACK)),
                ]
            ),
            "FP_BP Time",
        ]

    @staticmethod
    def make_grad_refresh_data(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make grad refresh data
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return [
            "Iteration Refresh {}".format(trace_parm[StepTraceConstant.ITER_ID]),
            pid,
            tid,
            trace_parm[StepTraceConstant.BACK_PROPAGATION],
            (trace_parm[StepTraceConstant.STEP_END] - trace_parm[StepTraceConstant.BACK_PROPAGATION]),
            OrderedDict(
                [
                    ("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                    ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                    ("Iteration End", trace_parm[StepTraceConstant.STEP_END]),
                    ("Iteration Refresh(us)", trace_parm.get(StepTraceConstant.ITERATION_REFRESH)),
                ]
            ),
            "Iteration Refresh",
        ]

    @staticmethod
    def make_data_aug_dict0(trace_parm: dict, pid: int, tid: int) -> dict:
        """
        make data aug dict0
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: dict
        """
        return OrderedDict(
            [
                ("name", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID] - 1)),
                ("cat", "Data_aug Bound"),
                ("ph", "t"),
                (
                    "ts",
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=InfoConfReader().time_from_syscnt(
                            trace_parm[StepTraceConstant.FORWARD_PROPAGATION], NumberConstant.MICRO_SECOND
                        ),
                        use_us=True,
                    ),
                ),
                ("pid", pid),
                ("tid", tid),
                ("id", "Data_aug Bound {}_{}".format(pid, tid)),
                ("args", OrderedDict([("Iteration ID", trace_parm[StepTraceConstant.ITER_ID])])),
            ]
        )

    @staticmethod
    def make_data_aug_dict1(trace_parm: dict, pid: int, tid: int) -> dict:
        """
        make data aug dict1
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: dict
        """
        data_aug_bound = trace_parm[StepTraceConstant.DATA_AUGMENTATION]
        refresh_dur = trace_parm[StepTraceConstant.STEP_END] - trace_parm[StepTraceConstant.BACK_PROPAGATION]
        ts = trace_parm[StepTraceConstant.BACK_PROPAGATION] + refresh_dur * 0.5

        data_aug_bound = data_aug_bound * NumberConstant.MICRO_SECOND / InfoConfReader().get_freq(StrConstant.HWTS)
        ts = InfoConfReader().trans_into_local_time(
            raw_timestamp=InfoConfReader().time_from_syscnt(ts, NumberConstant.MICRO_SECOND), use_us=True
        )

        return OrderedDict(
            [
                ("name", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID])),
                ("cat", "Data_aug Bound"),
                ("ph", "s"),
                ("ts", ts),
                ("pid", pid),
                ("tid", tid),
                ("id", "Data_aug Bound {}_{}".format(pid, tid)),
                ("args", OrderedDict([("Data_aug Bound(us)", data_aug_bound)])),
            ]
        )


class StepTraceViewer:
    """
    viewer of training trace data
    """

    model_to_pid = {}
    SORT_INDEX_OFFSET = 70000

    @staticmethod
    def get_step_trace_data(curs: any, message: dict) -> list:
        """
        get training trace data
        :param curs: sqlite cur
        :param message: message
        :return: data
        """
        sql = (
            "select iteration_id, "
            "(case when FP_start={1} then 'N/A' else FP_start end), "
            "(case when BP_end={1} then 'N/A' else BP_end end), "
            "iteration_end, "
            "(case when iteration_time={1} then 'N/A' else iteration_time end), "
            "(case when fp_bp_time={1} then 'N/A' else fp_bp_time end), "
            "(case when grad_refresh_bound={1} then 'N/A' else grad_refresh_bound end), "
            "(case when data_aug_bound={1} then 'N/A' else data_aug_bound end), "
            "(case when model_id={2} then 'N/A' else model_id end) "
            " from {0} where device_id=?".format(
                DBNameConstant.TABLE_TRAINING_TRACE,
                NumberConstant.NULL_NUMBER,
                NumberConstant.DEFAULT_MODEL_ID,
            )
        )
        data = DBManager.fetch_all_data(curs, sql, (message["device_id"],))
        return data

    @staticmethod
    def get_model_pid(data: list) -> int:
        """
        get model pid
        :param data: data
        :return: pid
        """
        return StepTraceViewer.model_to_pid.get(data[8])

    @staticmethod
    def make_model_meta(result_data: list, trace_data: list) -> None:
        """
        make model meta
        :param result_data: result data
        :param trace_data: trace data
        :return: void
        """
        if not trace_data:
            return
        model_id_index = 8  # 8 is model_id index in training_trace list
        model_ids_set = set()
        for trace_datum in trace_data:
            if len(trace_datum.get('training_trace', [])) > model_id_index:
                model_ids_set.add(trace_datum.get('training_trace', [])[model_id_index])
        if Constant.NA in model_ids_set:
            # if model_id is -1,it has been set "N/A"
            model_ids_set.remove(Constant.NA)
        model_ids = sorted(model_ids_set)
        result_data.extend(
            TraceViewManager.metadata_event(
                [
                    [
                        "process_name",
                        InfoConfReader().get_json_pid_data(),
                        InfoConfReader().get_json_tid_data(),
                        "Step Trace",
                    ]
                ]
            )
        )
        for i, model_id in enumerate(model_ids):
            StepTraceViewer.model_to_pid[model_id] = (
                InfoConfReader().get_json_pid_data() + i + StepTraceViewer.SORT_INDEX_OFFSET
            )
            result_data.extend(
                TraceViewManager.metadata_event(
                    [
                        [
                            "thread_name",
                            InfoConfReader().get_json_pid_data(),
                            StepTraceViewer.model_to_pid.get(model_id),
                            "Model ID:{}".format(model_id),
                        ],
                        [
                            "thread_sort_index",
                            InfoConfReader().get_json_pid_data(),
                            StepTraceViewer.model_to_pid.get(model_id),
                            int(model_id) + StepTraceViewer.SORT_INDEX_OFFSET,
                        ],
                    ]
                )
            )

    @staticmethod
    def add_reduce_headers(conn: any, headers: list, message: dict) -> None:
        """
        add reduece headers
        :param conn: sqlite connection
        :param headers: headers
        :param message: message
        :return:
        """
        if DBManager.judge_table_exist(conn.cursor(), DBNameConstant.TABLE_ALL_REDUCE):
            reduce_data = (
                conn.cursor()
                .execute(
                    "select max(count) from "
                    "(select count(*) as count from  {0} where device_id=?"
                    " group by iteration_end, model_id);".format(DBNameConstant.TABLE_ALL_REDUCE),
                    (message["device_id"],),
                )
                .fetchone()
            )
            if reduce_data[0]:
                headers += ["Reduce Start(us)", "Reduce Duration(us)"] * int(reduce_data[0])

    @staticmethod
    def format_reduce_json(data: list, trace_parm: dict, pid: int, tid: int, result_data: list) -> None:
        """
        format reduce json
        :param data: data
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: index
        :param result_data: result data
        :return: void
        """
        i = 0
        for reduce_data in data:
            if len(reduce_data) != 2:  # length of reduce_data is 2 containing start time and end time
                continue
            reduce_trace_data = []
            trace_parm[StepTraceConstant.REDUCE_START] = reduce_data[0]
            trace_parm[StepTraceConstant.REDUCE_END] = reduce_data[1]
            grad_refresh_data = [
                "Reduce_{}_{}".format(trace_parm[StepTraceConstant.ITER_ID], i),
                pid,
                tid,
                trace_parm[StepTraceConstant.REDUCE_START],
                (trace_parm[StepTraceConstant.REDUCE_END] - trace_parm[StepTraceConstant.REDUCE_START]),
                OrderedDict(
                    [
                        ("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                        ("Reduce Start {}".format(i), trace_parm[StepTraceConstant.REDUCE_START]),
                        ("Reduce End {}".format(i), trace_parm[StepTraceConstant.REDUCE_END]),
                    ]
                ),
                "Reduce",
            ]
            StepTraceViewer.update_reduce(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, grad_refresh_data, i)
            reduce_trace_data.append(grad_refresh_data)
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, reduce_trace_data)
            )
            i = i + 1

    @staticmethod
    def get_step_trace_summary(message: dict) -> tuple:
        """
        @param message
        Rewrite gRPC get_training_trace method.
        Return a GetTraceResponse protobuf message for client's request
        defined by GetTraceRequest.
        """
        headers = []

        conn_path = PathManager.get_db_path(message.get("project_path"), DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return MsvpConstant.MSVP_EMPTY_DATA
        conn, curs = DBManager.check_connect_db_path(conn_path)
        if not (conn and curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        data = StepTraceViewer.get_step_trace_data(curs, message)
        headers.append("Model ID")
        try:
            StepTraceViewer.add_reduce_headers(conn, headers, message)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return MsvpConstant.MSVP_EMPTY_DATA
        merge_data = StepTraceViewer._reformat_step_trace_data(data, conn)
        DBManager.destroy_db_connect(conn, curs)
        start_ts, _ = InfoConfReader().get_collect_time()
        logging.info(
            "There are %d records before step_trace data filtering, timestamp is %s.", len(merge_data), start_ts
        )

        def data_time_fetcher(item):
            if is_number(item[3]) and is_number(item[4]):
                step_start = float_calculate([item[3], item[4]], "-")
                return step_start, item[3]
            return 0.0, 0.0

        filtered_data = Utils.filter_data_by_start_time_condition(merge_data, start_ts, data_time_fetcher)
        logging.info("There are %d records after step_trace data filtering.", len(filtered_data))
        return headers, filtered_data, len(filtered_data)

    @staticmethod
    def get_step_trace_timeline(message: dict) -> list:
        """
        @param message
        Rewrite gRPC get_training_trace method.
        Return a GetTraceResponse protobuf message for client's request
        defined by GetTraceRequest.
        """
        conn_path = PathManager.get_db_path(message.get("project_path"), DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return MsvpConstant.EMPTY_LIST

        conn, curs = DBManager.check_connect_db_path(conn_path)
        data = StepTraceViewer.get_step_trace_data(curs, message)
        curs.close()

        return StepTraceViewer.get_trace_timeline_data(conn, data)

    @staticmethod
    def get_one_iter_timeline_data(result_dir: str, iter_range: IterationRange) -> str:
        """
        get one iteration timeline data
        :param result_dir: data dir
        :param iter_range: iter range
        :return: timeline json data
        """
        conn_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return MsvpConstant.EMPTY_LIST
        conn, curs = DBManager.check_connect_db_path(conn_path)
        values = StepTraceViewer.__select_trace_one_iter(curs, iter_range)

        return StepTraceViewer.get_trace_timeline_data(conn, values)

    @staticmethod
    def transfer_trace_unit(trace: list) -> None:
        """
        transfer trace unit
        :param trace: trace
        :param use_decimal: high-precision calculation switch
        :return: void
        """
        for i in range(1, 4):
            if trace[i] != "N/A":
                trace[i] = InfoConfReader().trans_syscnt_into_local_time(trace[i])

    @staticmethod
    def get_trace_timeline_data(cnn: any, values: list) -> any:
        """
        get trace timeline data
        :param cnn: db conn
        :param values: training data
        :return:json format data
        """
        step = 0
        data = list(range(len(values)))
        for line in values:
            trace = {
                'training_trace': list(line),
            }
            getnext = StepTraceViewer.__select_getnext(cnn, trace.get('training_trace', []))
            getnext = Utils.generator_to_list(list(data) for data in getnext)
            trace['get_next'] = getnext
            all_reduce = StepTraceViewer.__select_reduce(cnn, trace.get('training_trace', []))
            all_reduce = Utils.generator_to_list(list(data) for data in all_reduce)
            trace['all_reduce'] = all_reduce
            data[step] = trace
            # Cursor step moved 1 step
            step = step + 1
        if cnn:
            DBManager.destroy_db_connect(cnn, cnn.cursor())
        return StepTraceViewer.__format_trace_json(data)

    @staticmethod
    def format_get_next_json(data: list, pid: int, tid: int, result_data: list) -> None:
        get_next_trace_data = []
        for get_next_data in data:
            # length of get_next_data is 2 containing start time and end time
            if len(get_next_data) != 2 or get_next_data[0] == "N/A" or get_next_data[1] == "N/A":
                continue
            get_next_start = get_next_data[0]
            get_next_end = get_next_data[1]
            refresh_data = [
                "GetNext",
                pid,
                tid,
                get_next_start,
                get_next_end - get_next_start,
                OrderedDict(
                    [
                        ("GetNext Start", get_next_start),
                        ("GetNext End", get_next_end),
                        ("GetNext Time(us)", get_next_end - get_next_start),
                    ]
                ),
                "GetNext Time",
            ]
            StepTraceViewer.update_get_next_data(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, refresh_data)
            get_next_trace_data.append(refresh_data)
        result_data.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, get_next_trace_data)
        )

    @staticmethod
    def update_iteration(headers: list, data: list) -> None:
        StepTraceViewer.update_ts_and_dur(headers, data)
        data[headers.index('args')]['FP Start'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['FP Start']
        )
        data[headers.index('args')]['BP End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['BP End']
        )
        data[headers.index('args')]['Iteration End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['Iteration End']
        )
        data[headers.index('args')]['Iteration Time(us)'] = StepTraceViewer.calculate_duration(
            data[headers.index('args')]['Iteration Time(us)']
        )

    @staticmethod
    def update_fp_bp_time(headers: list, data: list) -> None:
        StepTraceViewer.update_ts_and_dur(headers, data)
        data[headers.index('args')]['FP_BP Time(us)'] = StepTraceViewer.calculate_duration(
            data[headers.index('args')]['FP_BP Time(us)']
        )
        data[headers.index('args')]['FP Start'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['FP Start']
        )
        data[headers.index('args')]['BP End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['BP End']
        )

    @staticmethod
    def update_reduce(headers: list, data: list, index: int) -> None:
        StepTraceViewer.update_ts_and_dur(headers, data)
        data[headers.index('args')]['Reduce Start ' + str(index)] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['Reduce Start ' + str(index)]
        )
        data[headers.index('args')]['Reduce End ' + str(index)] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['Reduce End ' + str(index)]
        )

    @staticmethod
    def update_get_next_data(headers: list, data: list) -> None:
        StepTraceViewer.update_ts_and_dur(headers, data)
        data[headers.index('args')]['GetNext Start'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['GetNext Start']
        )
        data[headers.index('args')]['GetNext End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['GetNext End']
        )
        data[headers.index('args')]['GetNext Time(us)'] = StepTraceViewer.calculate_duration(
            data[headers.index('args')]['GetNext Time(us)']
        )

    @staticmethod
    def update_grad_refresh_data(headers: list, data: list) -> list:
        StepTraceViewer.update_ts_and_dur(headers, data)
        data[headers.index('args')]['BP End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['BP End']
        )
        data[headers.index('args')]['Iteration End'] = StepTraceViewer.calculate_localtime(
            data[headers.index('args')]['Iteration End']
        )
        data[headers.index('args')]['Iteration Refresh(us)'] = StepTraceViewer.calculate_duration(
            data[headers.index('args')]['Iteration Refresh(us)']
        )
        return data

    @staticmethod
    def update_ts_and_dur(headers: list, data: list) -> None:
        data[headers.index('ts')] = StepTraceViewer.calculate_localtime(data[headers.index('ts')])
        data[headers.index('dur')] = StepTraceViewer.calculate_duration(data[headers.index('dur')])

    @staticmethod
    def calculate_localtime(data: any) -> str:
        if not is_number(data):
            return data
        return InfoConfReader().trans_syscnt_into_local_time(data)

    @staticmethod
    def calculate_duration(data: any) -> str:
        if not is_number(data):
            return data
        return data * NumberConstant.MICRO_SECOND / InfoConfReader().get_freq(StrConstant.HWTS)

    @staticmethod
    def _reformat_step_trace_data(data: list, conn: any) -> list:
        merge_data = []
        for line in data:
            trace = list(line)
            if len(trace) >= 4:  # trace[3] refers to iteration end
                reduce_data = StepTraceViewer.__select_reduce(conn, trace)
                for item in reduce_data:
                    trace += [
                        format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(item[0])),
                        round(
                            (item[1] - item[0]) * StepTraceConstant.syscnt_to_micro(),
                            NumberConstant.ROUND_THREE_DECIMAL,
                        ),
                    ]
            trace[1:4] = map(format_high_precision_summary_data, trace[1:4])
            trace[4:8] = map(StepTraceViewer.calculate_duration, trace[4:8])
            merge_data.append(trace)
        return merge_data

    @staticmethod
    def __select_reduce(conn: any, trace: list) -> list:
        """
        Select date from all_reduce table with specific ids.
        :param conn: connect to database
        :param trace: trace data
        :return: result
        """
        curs = conn.cursor()
        iteration_end = trace[3]
        model_id = NumberConstant.DEFAULT_MODEL_ID if trace[-1] == "N/A" else trace[-1]

        sql = "select start, end from {0} where iteration_end=? and model_id=?".format(DBNameConstant.TABLE_ALL_REDUCE)
        result = DBManager.fetch_all_data(curs, sql, (iteration_end, model_id))
        curs.close()
        return result

    @staticmethod
    def __select_getnext(conn: any, trace: list) -> list:
        """
        Select date from getnext table with specific ids.
        :param conn: connect to database
        :param trace: trace data
        :return: result
        """
        curs = conn.cursor()
        iteration_id = trace[0]
        model_id = NumberConstant.DEFAULT_MODEL_ID if trace[-1] == "N/A" else trace[-1]

        sql = (
            "select "
            "(case when start_time={1} then 'N/A' else start_time end), "
            "(case when end_time={1} then 'N/A' else end_time end) "
            "from {0} where index_id=? and model_id=?".format(
                DBNameConstant.TABLE_GET_NEXT,
                NumberConstant.NULL_NUMBER,
            )
        )
        result = DBManager.fetch_all_data(curs, sql, (iteration_id, model_id))
        curs.close()
        return result

    @staticmethod
    def __select_trace_one_iter(curs: any, iter_range: IterationRange) -> list:
        """
        Select date from traing_trace limited by count and sort
        """

        sql = (
            "select iteration_id, "
            "(case when FP_start={1} then 'N/A' else FP_start end), "
            "(case when BP_end={1} then 'N/A' else BP_end end), "
            "iteration_end, "
            "(case when iteration_time={1} then 'N/A' else iteration_time end), "
            "(case when fp_bp_time={1} then 'N/A' else fp_bp_time end), "
            "(case when grad_refresh_bound={1} then 'N/A' else grad_refresh_bound end), "
            "(case when data_aug_bound={1} then 'N/A' else data_aug_bound end), "
            "(case when model_id={2} then 'N/A' else model_id end) "
            "from {0} "
            "where model_id=? and iteration_id>=? and iteration_id<=?".format(
                DBNameConstant.TABLE_TRAINING_TRACE, NumberConstant.NULL_NUMBER, NumberConstant.DEFAULT_MODEL_ID
            )
        )
        return DBManager.fetch_all_data(curs, sql, (iter_range.model_id, *iter_range.get_iteration_range()))

    @staticmethod
    @catch_exception
    def __format_trace_json(trace_data: list) -> any:
        """
        Parse hwts protobuf message to json format
        trace_parm//100: Convert 10ns-level timestamp to us-level timestamp
        tid=0:Iteration Time
        tid=1:FP_BP Time;Grad_refresh Bound
        tid=2:Reduce time
        """
        result_data = []
        trace_parm = {}
        result_dict = {}
        StepTraceViewer.make_model_meta(result_data, trace_data)
        pid = InfoConfReader().get_json_pid_data()
        for _, trace_item in enumerate(trace_data):
            data = trace_item.get('training_trace', [])
            trace_view_data = []
            TimeLineJsonMaker.create_trace_parm(trace_parm, data)

            tid = StepTraceViewer.get_model_pid(data)

            iter_time_data = TimeLineJsonMaker.make_iter_time(trace_parm, pid, tid)
            StepTraceViewer.update_iteration(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, iter_time_data)

            if data[1] == "N/A" or data[2] == "N/A":
                trace_view_data.append(iter_time_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, trace_view_data)
                )
            else:
                fp_bp_data = TimeLineJsonMaker.make_fp_bp_data(trace_parm, pid, tid)
                StepTraceViewer.update_fp_bp_time(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, fp_bp_data)
                grad_refresh_data = TimeLineJsonMaker.make_grad_refresh_data(trace_parm, pid, tid)
                grad_refresh_data = StepTraceViewer.update_grad_refresh_data(
                    TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, grad_refresh_data
                )
                result_dict["data_aug_dict0"] = TimeLineJsonMaker.make_data_aug_dict0(trace_parm, pid, tid)
                result_dict["data_aug_dict1"] = TimeLineJsonMaker.make_data_aug_dict1(trace_parm, pid, tid)

                trace_view_data.append(iter_time_data)
                trace_view_data.append(fp_bp_data)
                trace_view_data.append(grad_refresh_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, trace_view_data)
                )
                result_data.extend([result_dict.get("data_aug_dict0", {}), result_dict.get("data_aug_dict1", {})])

            StepTraceViewer.format_reduce_json(trace_item.get("all_reduce", []), trace_parm, pid, tid, result_data)
            StepTraceViewer.format_get_next_json(trace_item.get("get_next", []), pid, tid, result_data)

        return result_data


def format_high_precision_summary_data(data: int) -> str:
    if not data or data == 'N/A':
        return data
    local_time = InfoConfReader().trans_syscnt_into_local_time(data)
    return format_high_precision_for_csv(local_time)
