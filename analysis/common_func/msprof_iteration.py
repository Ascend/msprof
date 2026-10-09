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
from itertools import chain

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.utils import Utils
from msmodel.step_trace.ts_track_model import TsTrackModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class MsprofIteration:
    """
    mainly process iteration
    """

    HOST_START_TAG = 0
    HOST_END_TAG = 1

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir
        self._track_model = TsTrackModel(
            self._result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]
        )

    @staticmethod
    def get_iter_id_within_iter_range(step_trace_data: list, timestamp: int, iter_range: IterationRange):
        while step_trace_data:
            step_trace = step_trace_data[0]
            if InfoConfReader().time_from_syscnt(step_trace.step_end) < timestamp:
                step_trace_data.pop(0)
                continue
            return step_trace.index_id
        return iter_range.iteration_end

    @staticmethod
    def _generate_trace_result(trace_datas: list, time_fmt: int = NumberConstant.MICRO_SECOND) -> list:
        trace_datas = list(chain.from_iterable(trace_datas))
        if not trace_datas:
            return []
        trace_datas = [InfoConfReader().time_from_syscnt(timestamp, time_fmt) for timestamp in trace_datas]
        result = [(0, max(trace_datas))] if len(trace_datas) == 1 else [(min(trace_datas), max(trace_datas))]
        return result

    @staticmethod
    def _generate_trace_iter_end_result(trace_datas: list) -> dict:
        iter_end_dict = OrderedDict()
        for trace_data in trace_datas:
            iter_end_dict.setdefault(trace_data[0], trace_data[1])
        return iter_end_dict

    @staticmethod
    def _get_host_iter_range_cnts(all_iter_end_cnts, iter_range: IterationRange):
        """
        return cnt range for iter_range
        :param all_iter_end_cnts: [(iter_id, end_cnt), (iter_id, end_cnt)]
        :param iter_range:
        :return: [lower_bound, end_bound]
        """
        start_iter = iter_range.iteration_start
        end_iter = iter_range.iteration_end
        min_iter = all_iter_end_cnts[0][0]
        max_iter = all_iter_end_cnts[-1][0]
        lower_bound = 0.0
        upper_bound = float("inf")
        for i, (iter_id, cnt) in enumerate(all_iter_end_cnts):
            if iter_id > min_iter and iter_id == start_iter:
                lower_bound = all_iter_end_cnts[i - 1][1]
            if iter_id < max_iter and iter_id == end_iter:
                upper_bound = cnt
        return [lower_bound, upper_bound]

    def get_step_syscnt_range_by_iter_range(self, iter_range: IterationRange):
        """
        the time range within the iteration range.
        """
        with TsTrackModel(
            self._result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]
        ) as _trace:
            time_range = _trace.get_step_syscnt_range_by_iter_range(iter_range)
        return [time_range.step_start, time_range.step_end] if time_range else []

    def get_step_end_range_by_iter_range(self, iter_range: IterationRange):
        """
        return step trace dto with the mapping of index_id and step_end
        """
        with TsTrackModel(
            self._result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]
        ) as _trace:
            return _trace.get_step_end_list_with_iter_range(iter_range)

    def get_iter_interval(self: any, iter_range: IterationRange, time_fmt: int = NumberConstant.NANO_SECOND) -> list:
        step_trace_range = self.get_step_syscnt_range_by_iter_range(iter_range)
        if step_trace_range:
            return [InfoConfReader().time_from_syscnt(_range, time_fmt) for _range in step_trace_range]
        return []

    def get_iteration_time(self: any, iter_range: IterationRange, time_fmt: int = NumberConstant.MICRO_SECOND) -> list:
        """
        get iteration start and end timestamp
        :param iter_range: iter range
        :param time_fmt: timestamp format
        :return: iteration list
        """
        if Utils.is_step_scene(self._result_dir):
            return self._generate_trace_result(self.get_step_iteration_time(iter_range), time_fmt)
        return []

    def get_step_iteration_time(self: any, iter_range: IterationRange) -> list:
        """
        get step iteration time
        """
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        table_name = ProfilingScene().get_step_table_name()
        if not trace_conn or not trace_curs or not DBManager.judge_table_exist(trace_curs, table_name):
            return []
        try:
            trace_data = self._get_iteration_time(trace_curs, iter_range)
            return trace_data
        except sqlite3.Error as trace_err:
            logging.error("Get step trace data failed, %s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(trace_conn, trace_curs)

    def get_parallel_iter_range(self: any, iter_range: IterationRange) -> list:
        """
        get step iteration time
        :param iter_range: model id
        :return: [iter_id - 1, iter_id] or [min_iter_id - 1, max_iter_id] in pytorch graph
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        table_name = ProfilingScene().get_step_table_name()

        parallel_iter_range = []
        for _iter in iter_range.get_iteration_range():
            trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
            if not trace_conn or not trace_curs or not DBManager.check_tables_in_db(db_path, table_name):
                return []
            current_iter = self.get_iteration_info_by_index_id(_iter, iter_range.model_id)
            if not current_iter:
                return []

            # find first parallel iter
            sql = (
                "select model_id, index_id, iter_id, step_start, step_end from {0} "
                "where step_end>? and step_end<=? order by step_end".format(table_name)
            )
            first_parallel_iter = DBManager.fetchone(
                trace_curs,
                sql,
                (
                    current_iter.step_start,
                    current_iter.step_end,
                ),
                dto_class=StepTraceDto,
            )
            DBManager.destroy_db_connect(trace_conn, trace_curs)
            parallel_iter_range.extend([first_parallel_iter.iter_id, current_iter.iter_id])
        return [min(parallel_iter_range), max(parallel_iter_range)]

    def get_index_id_list_with_index_and_model(self: any, iter_range: IterationRange) -> set:
        """
        get step iter dict with index and model
        :param iter_range: index id
        :return: {iter_id: [index_id, model_id]} in mix single op and graph
        """
        iter_set = {
            (iter_range.iteration_id + _count, iter_range.model_id) for _count in range(iter_range.iteration_count)
        }
        iter_set.add((NumberConstant.STATIC_SHAPE_ITER_ID, iter_range.model_id))
        if not (ProfilingScene().is_mix_operator_and_graph() and iter_range.model_id == Constant.GE_OP_MODEL_ID):
            return iter_set

        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        table_name = ProfilingScene().get_step_table_name()
        if not trace_conn or not trace_curs or not DBManager.check_tables_in_db(db_path, table_name):
            return set()
        iter_id_range = self.get_parallel_iter_range(iter_range)
        if not iter_id_range:
            return iter_set
        sql = (
            "select model_id, index_id, iter_id, step_start, step_end from {0} where iter_id>=? and iter_id<=?".format(
                table_name
            )
        )
        parallel_iter_info_list = DBManager.fetch_all_data(trace_curs, sql, iter_id_range, dto_class=StepTraceDto)

        for parallel_iter_info in parallel_iter_info_list:
            iter_set.add((NumberConstant.STATIC_SHAPE_ITER_ID, parallel_iter_info.model_id))
            iter_set.add((parallel_iter_info.index_id, parallel_iter_info.model_id))
        return iter_set

    def get_iteration_end_dict(self: any) -> dict:
        """
        get iteration end timestamp
        """
        if Utils.is_step_scene(self._result_dir):
            return self.__get_trace_iteration_end()
        return {}

    def get_iteration_info_by_index_id(self: any, index_id: int, model_id: int) -> any:
        """
        get iteration info by index_id
        """
        table_name = ProfilingScene().get_step_table_name()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs or not DBManager.check_tables_in_db(db_path, table_name):
            return EmptyClass()
        sql = (
            "select model_id, index_id, iter_id, step_start, step_end from {0} "
            "where model_id={1} and index_id={2}".format(table_name, model_id, index_id)
        )
        iter_info = DBManager.fetchone(trace_curs, sql, dto_class=StepTraceDto)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return iter_info

    def get_condition_within_iteration(self: any, iter_range: IterationRange, time_start_key: str, time_end_key: str):
        """
        get the condition for sql that data should be within iteration_id.
        """
        if ProfilingScene().is_all_export():
            return ""
        iter_time_range = self.get_iteration_time(iter_range, time_fmt=NumberConstant.NANO_SECOND)
        if not iter_time_range:
            return ''
        iter_start = InfoConfReader().get_host_syscnt_from_dev_time(iter_time_range[0][0])
        iter_end = InfoConfReader().get_host_syscnt_from_dev_time(iter_time_range[0][1])
        return (
            f'where ({time_start_key}>={iter_start} and {time_start_key}<={iter_end}) '
            f'or ({time_start_key}<={iter_start} and {iter_start}<={time_end_key})'
        )

    def get_step_trace_op(self: any) -> set:
        """
        get step trace task set
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if (
            not trace_conn
            or not trace_curs
            or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE)
        ):
            return set()
        sql = "select stream_id, task_id from {0} where tag_id=1 or tag_id=4 order by timestamp".format(
            DBNameConstant.TABLE_STEP_TRACE
        )
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return {"{0}-{1}".format(task_id, stream_id) for task_id, stream_id in trace_datas}

    def _get_iteration_time(self: any, trace_curs: any, iter_range: IterationRange) -> list:
        step_syscnt_end = []
        for _iter in iter_range.get_iteration_range():
            current_iter = self.get_iteration_info_by_index_id(_iter, iter_range.model_id)
            if not current_iter:
                return []

            # find last and not parallel iter
            sql = "select step_end from {0} where step_end<? order by step_end desc ".format(
                ProfilingScene().get_step_table_name()
            )
            last_not_parallel_iter = DBManager.fetchone(
                trace_curs, sql, (current_iter.step_start,), dto_class=StepTraceDto
            )
            if not last_not_parallel_iter:
                step_syscnt_end.extend([NumberConstant.ZERO_ITER_ID, current_iter.step_end])
                continue
            step_syscnt_end.extend([last_not_parallel_iter.step_end, current_iter.step_end])
        return [(min(step_syscnt_end),), (max(step_syscnt_end),)]

    def __get_trace_iteration_end(self: any) -> dict:
        iter_end_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if (
            not trace_conn
            or not trace_curs
            or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA)
        ):
            return iter_end_dict
        sql = "select iter_id, step_end from {0} order by step_end".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_end_dict
        return MsprofIteration._generate_trace_iter_end_result(trace_datas)
