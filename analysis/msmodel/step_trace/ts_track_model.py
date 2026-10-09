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

from abc import ABC
from functools import partial

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from msmodel.interface.base_model import BaseModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto
from profiling_bean.db_dto.step_trace_ge_dto import StepTraceGeDto
from profiling_bean.db_dto.tiling_block_num_dto import TilingBlockNumDto
from profiling_bean.db_dto.time_section_dto import TimeSectionDto
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """

    TS_AI_CPU_TYPE = 1

    @staticmethod
    def __aicpu_in_time_range(data, min_timestamp, max_timestamp):
        return min_timestamp <= data[2] <= max_timestamp

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)

    def create_table(self: any, table_name: str) -> None:
        """
        create table
        """
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)

    def get_ai_cpu_data(self: any, iter_time_range) -> list:
        """
        get ai cpu data
        :param iter_time_range: iteration time range
        :return: ai cpu with state
        """
        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE), DBNameConstant.TABLE_TASK_TYPE
        ):
            return []

        sql = (
            "select stream_id, task_id, timestamp, task_state from {0} where task_type={1} order by timestamp ".format(
                DBNameConstant.TABLE_TASK_TYPE, self.TS_AI_CPU_TYPE
            )
        )
        ai_cpu_with_state = DBManager.fetch_all_data(self.cur, sql)

        for index, datum in enumerate(ai_cpu_with_state):
            ai_cpu_with_state[index] = list(datum)
            # index 2 is timestamp
            ai_cpu_with_state[index][2] = int(datum[2])

        if not ProfilingScene().is_all_export() and iter_time_range:
            min_timestamp = min(iter_time_range)
            max_timestamp = max(iter_time_range)

            # data index 2 is timestamp
            ai_cpu_with_state = list(
                filter(
                    partial(self.__aicpu_in_time_range, min_timestamp=min_timestamp, max_timestamp=max_timestamp),
                    ai_cpu_with_state,
                )
            )

        return ai_cpu_with_state

    def get_step_trace_data(self: any, table_name: str) -> list:
        """
        get step trace data
        """
        if not DBManager.judge_table_exist(self.cur, table_name) or not DBManager.judge_row_exist(self.cur, table_name):
            return []
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0}".format(table_name)
        step_trace_data = DBManager.fetch_all_data(self.cur, sql, dto_class=StepTraceDto)
        return step_trace_data

    def get_index_range_with_model(self, model_id):
        """
        get the max iteration id of the model id.
        """
        table_name = ProfilingScene().get_step_table_name()
        if not DBManager.judge_table_exist(self.cur, table_name) or not DBManager.judge_row_exist(self.cur, table_name):
            return []
        sql = f'select min(index_id), max(index_id) from {table_name} where model_id=?'
        return DBManager.fetchone(self.cur, sql, (model_id,))

    def get_step_syscnt_range_by_iter_range(self, iteration: IterationRange):
        """
        get step time range by the iteration range.
        """
        table_name = ProfilingScene().get_step_table_name()
        if not DBManager.judge_table_exist(self.cur, table_name) or not DBManager.judge_row_exist(self.cur, table_name):
            return EmptyClass()
        iteration_range = iteration.get_iteration_range()
        if iteration_range[0] == 1:
            sql = (
                f"select 0 as step_start, max(step_end) as step_end "
                f"from {table_name} where model_id=? and index_id>=? and index_id<=?"
            )
        else:
            sql = (
                f"select min(step_start) as step_start, max(step_end) as step_end "
                f"from {table_name} where model_id=? and index_id>=? and index_id<=?"
            )
        return DBManager.fetchone(self.cur, sql, (iteration.model_id, *iteration_range), dto_class=StepTraceDto)

    def get_step_syscnt_range(self, iteration: IterationRange):
        """
        get step start sys_cnt and end sys_cnt
        """
        table_name = ProfilingScene().get_step_table_name()
        if not DBManager.judge_table_exist(self.cur, table_name) or not DBManager.judge_row_exist(self.cur, table_name):
            return EmptyClass()
        iteration_range = iteration.get_iteration_range()
        sql = (
            f"select min(step_start) as step_start, max(step_end) as step_end "
            f"from {table_name} where model_id = ? and index_id >= ? and index_id <= ?"
        )
        return DBManager.fetchone(self.cur, sql, (iteration.model_id, *iteration_range), dto_class=StepTraceDto)

    def get_step_end_list_with_iter_range(self, iteration: IterationRange):
        """
        get step trace within the range of iteration.
        """
        table_name = ProfilingScene().get_step_table_name()
        if not DBManager.judge_table_exist(self.cur, table_name) or not DBManager.judge_row_exist(self.cur, table_name):
            return []
        sql = (
            f"select index_id, step_end from {table_name} "
            f"where model_id=? and index_id>=? and index_id<=? order by step_end"
        )
        return DBManager.fetch_all_data(
            self.cur, sql, (iteration.model_id, *iteration.get_iteration_range()), dto_class=StepTraceDto
        )

    def get_task_flip_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_DEVICE_TASK_FLIP):
            return []
        sql = "select stream_id, timestamp, task_id, flip_num from {0}".format(DBNameConstant.TABLE_DEVICE_TASK_FLIP)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TaskFlip)

    def get_step_trace_with_tag(self: any, tags: list) -> list:
        if (
            not tags
            or not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE)
            or not DBManager.judge_row_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE)
        ):
            return []
        tags_condition = ",".join([str(tag) for tag in tags])
        select_sql = (
            f"select DISTINCT index_id, model_id, timestamp, tag_id, stream_id "
            f"from {DBNameConstant.TABLE_STEP_TRACE} "
            f"where tag_id in ({tags_condition}) order by timestamp"
        )
        return DBManager.fetch_all_data(self.cur, select_sql, dto_class=StepTraceOriginDto)


class TsTrackViewModel(ViewModel):
    def __init__(self: any, path: str, table_list: list = None) -> None:
        super().__init__(path, DBNameConstant.DB_STEP_TRACE, table_list if table_list else [])

    def get_hccl_operator_exe_data(self) -> list:
        if not self.attach_to_db(DBNameConstant.DB_GE_INFO):
            return []
        device_id = InfoConfReader().get_device_id()
        sql = (
            "SELECT t1.model_id model_id, t1.index_id index_id, t1.stream_id stream_id, t1.task_id task_id, "
            "t1.tag_id tag_id, t1.timestamp timestamp, t2.op_name op_name, t2.op_type op_type "
            "FROM ( SELECT model_id, index_id, tag_id, stream_id, task_id-1 AS task_id, timestamp "
            "FROM {0} WHERE tag_id>=10000 ) t1 LEFT JOIN ( "
            "SELECT model_id, index_id, stream_id, task_id, op_name, op_type FROM {1} WHERE task_type='{2}' ) t2 "
            "ON t1.model_id=t2.model_id AND (t1.index_id=t2.index_id OR t2.index_id=0 ) "
            "AND t1.stream_id = t2.stream_id AND t1.task_id = t2.task_id AND "
            "t2.device_id = {3} ORDER BY t1.timestamp".format(
                DBNameConstant.TABLE_STEP_TRACE,
                DBNameConstant.TABLE_GE_TASK,
                Constant.TASK_TYPE_COMMUNICATION,
                device_id,
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=StepTraceGeDto)

    def get_ai_cpu_data(self) -> list:
        sql = (
            "SELECT stream_id, task_id, timestamp, task_state FROM {} where "
            "task_type=1 and (task_state=1 or task_state=2) order by timestamp".format(DBNameConstant.TABLE_TASK_TYPE)
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_iter_time_data(self) -> list:
        sql = (
            "select model_id, index_id ,step_start as start_time, step_end as end_time "
            "from {} order by end_time".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)

    def get_tiling_block_num_data(self):
        sql = "select stream_id, task_id, timestamp, block_num from {}".format(DBNameConstant.TABLE_BLOCK_NUM)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TilingBlockNumDto)
