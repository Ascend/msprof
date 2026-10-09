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

import logging
import os
import sqlite3
from collections import defaultdict

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.ascend_task.ascend_task import TopDownTask
from msconfig.config_manager import ConfigManager
from msmodel.task_time.ascend_task_model import AscendTaskModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from viewer.ge_info_report import get_ge_model_name_dict


class MergeOpCounterCalculator(MsMultiProcess):
    """
    class to merge GE DB and runtime tasktime data
    """

    TABLE_PATH = ConfigManager.TABLES
    MODEL_NAME_INDEX = 8
    MODEL_ID_INDEX = 7

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self.iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.device_id = self.sample_config.get(StrConstant.PARAM_DEVICE_ID)
        self.sql_path = None
        self.conn = None
        self.curs = None

    @staticmethod
    def _get_op_report_sql() -> str:
        start_ts, _ = InfoConfReader().get_collect_time()
        start_time_raw_timestamp = InfoConfReader().trans_from_local_time_into_dev_raw_time(start_ts)
        # ge or subtask need modify the context_id or subtask_id so that it should be same.
        sql = (
            "select op_type, {0}.task_type, count(op_type), sum(duration) as total_time, "
            "min(duration) as min, sum(duration)/count(op_type) as avg, "
            "max(duration) as max, {0}.model_id from {0}, {1} "
            "where {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id "
            "and {0}.batch_id={1}.batch_id "
            "and {0}.context_id={1}.subtask_id "
            "and {1}.start_time > {2} "
            "group by op_type,{0}.task_type".format(
                DBNameConstant.TABLE_OP_COUNTER_GE_MERGE,
                DBNameConstant.TABLE_OP_COUNTER_RTS_TASK,
                start_time_raw_timestamp,
            )
        )
        return sql

    @staticmethod
    def _cal_total(type_time: dict) -> dict:
        """
        calculate total time for each device
        :param type_time: {"model":{"op":{'task_type': 0,'count': 0,'duration': 0,
        'min': 0,'avg': 0,max': 0}}}
        :return: total time
        """
        total_time = defaultdict(int)
        for model in type_time:
            for ops in type_time[model].values():
                total_time[model] += ops.get("duration", 0)
        return total_time

    def ms_run(self: any) -> None:
        """
        entry
        :return: None
        """
        if ProfilingScene().is_graph_export():
            self.process()

    def process(self: any) -> None:
        """
        merge process
        :return: None
        """
        try:
            self._init_params()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.create_and_insert_db()

    def create_and_insert_db(self: any) -> None:
        if not self._is_db_need_to_create():
            logging.warning(
                "No need to create db for op counter, maybe the data of framework or task is not collected."
            )
            return
        self.create_db(self.TABLE_PATH)
        self._create_ge_merge()
        self._create_task()
        self._create_report()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def create_db(self: any, map_path: str) -> None:
        """
        analysis and create db for op statics
        :param map_path: table config file path
        :return: connection of op statics
        """
        self.conn, self.curs = DBManager.create_connect_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_OP_COUNTER)
        )
        if not self.conn or not self.curs:
            logging.error("unable to create op counter db connection")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        ge_create_sql = DBManager.sql_create_general_table(
            "GeMergeMap", DBNameConstant.TABLE_OP_COUNTER_GE_MERGE, map_path
        )
        DBManager.execute_sql(self.conn, ge_create_sql)

        rts_task_create_sql = DBManager.sql_create_general_table(
            "RtsTaskMap", DBNameConstant.TABLE_OP_COUNTER_RTS_TASK, map_path
        )
        DBManager.execute_sql(self.conn, rts_task_create_sql)

        op_report_create_sql = DBManager.sql_create_general_table(
            "OpReportMap", DBNameConstant.TABLE_OP_COUNTER_OP_REPORT, map_path
        )
        DBManager.execute_sql(self.conn, op_report_create_sql)

    def _init_params(self: any) -> None:
        """
        initial and check params
        :return: None
        """
        self.sql_path = self.sample_config.get('sql_path', self._get_sql_dir())
        if not os.path.exists(self.sql_path):
            logging.error("Failed to get sqlite path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_OP_COUNTER)):
            os.remove(PathManager.get_db_path(self.project_path, DBNameConstant.DB_OP_COUNTER))

    def _is_db_need_to_create(self: any) -> bool:
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            return True
        if ChipManager().is_chip_v1():
            return DBManager.check_tables_in_db(
                PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                DBNameConstant.TABLE_RUNTIME_TASK_TIME,
            )
        return DBManager.check_tables_in_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_HWTS), DBNameConstant.TABLE_HWTS_TASK_TIME
        )

    def _get_ge_data(self: any, ge_curs: any) -> list:
        device_id = InfoConfReader().get_device_id()
        hccl_task_type = Constant.TASK_TYPE_COMMUNICATION
        hccl_ai_cpu = Constant.TASK_TYPE_HCCL_AI_CPU
        ge_data = []
        iter_list = MsprofIteration(self.project_path).get_index_id_list_with_index_and_model(self.iter_range)
        ge_sql = (
            f"select model_id, op_name, op_type, task_type, task_id, stream_id, batch_id, context_id "
            f"from {DBNameConstant.TABLE_GE_TASK} where index_id=? and model_id=? and device_id={device_id} "
            f"and task_type != '{hccl_task_type}' and task_type != '{hccl_ai_cpu}'"
        )
        for index_and_model in iter_list:
            ge_data.extend(DBManager.fetch_all_data(ge_curs, ge_sql, index_and_model))

        return ge_data

    def _get_ge_data_from_merge_task(self) -> list:
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_OP_COUNTER_GE_MERGE):
            return []
        ge_sql = "SELECT task_type, stream_id, task_id, batch_id, context_id from {0}".format(
            DBNameConstant.TABLE_OP_COUNTER_GE_MERGE
        )
        return DBManager.fetch_all_data(self.curs, ge_sql, dto_class=GeTaskDto)

    def _create_ge_merge(self: any) -> None:
        """
        merge GE ge_task_data、ge_graph_data into merged db
        :return: None
        """
        ge_conn, ge_curs = DBManager.check_connect_db_path(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        )
        if ge_conn and ge_curs and DBManager.judge_table_exist(ge_curs, DBNameConstant.TABLE_GE_TASK):
            ge_data = self._get_ge_data(ge_curs)
            if ge_data:
                insert_sql = "insert into {} values({value})".format(
                    DBNameConstant.TABLE_OP_COUNTER_GE_MERGE, value='?,' * (len(ge_data[0]) - 1) + '?'
                )
                DBManager.executemany_sql(self.conn, insert_sql, ge_data)
        DBManager.destroy_db_connect(ge_conn, ge_curs)

    def _create_task(self: any) -> None:
        """
        insert data to task time table
        :return:
        """
        project_path = self.sample_config.get("result_dir")
        ascend_task_db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_ASCEND_TASK)

        if not DBManager.check_tables_in_db(ascend_task_db_path, DBNameConstant.TABLE_ASCEND_TASK):
            logging.warning("No table %s found in %s", DBNameConstant.TABLE_ASCEND_TASK, DBNameConstant.DB_ASCEND_TASK)
            return
        with AscendTaskModel(project_path, DBNameConstant.TABLE_ASCEND_TASK) as model:
            tasks = model.get_all_data(DBNameConstant.TABLE_ASCEND_TASK)
            if not tasks:
                logging.error("Get tasks from %s error", DBNameConstant.TABLE_ASCEND_TASK)
                return
            ascend_tasks = [TopDownTask(*task) for task in tasks]
            rts_data = [
                [
                    task.task_id,
                    task.stream_id,
                    task.start_time,
                    task.duration,
                    task.device_task_type,
                    task.index_id,
                    task.model_id,
                    task.batch_id,
                    task.context_id,
                ]
                for task in ascend_tasks
            ]
            try:
                DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_OP_COUNTER_RTS_TASK, rts_data)
            except sqlite3.Error as err:
                logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _get_sql_dir(self: any) -> str:
        return PathManager.get_sql_dir(self.project_path)

    def _update_model_name(self: any, task_data: list) -> list:
        model_name_dict = get_ge_model_name_dict(self.project_path)
        result_data = [0] * len(task_data)
        for index, _task_data in enumerate(task_data):
            _task_data = list(_task_data)
            model_name = model_name_dict.get(_task_data[self.MODEL_ID_INDEX], Constant.NA)
            _task_data.insert(self.MODEL_NAME_INDEX, model_name)
            result_data[index] = _task_data
        return result_data

    def _create_report(self: any) -> None:
        """
        create report table
        :return: None
        """
        sql = self._get_op_report_sql()
        task_data = DBManager.fetch_all_data(self.curs, sql)
        if not task_data:
            return
        task_data = self._update_model_name(task_data)
        type_time = defaultdict(dict)
        for task in task_data:
            type_time[task[self.MODEL_NAME_INDEX]]["{}_{}".format(task[0], task[1])] = {
                'op_type': task[0],
                'task_type': task[1],
                'count': task[2],
                'duration': task[3],
                'min': task[4],
                'avg': task[5],
                'max': task[6],
            }
        total_time = self._cal_total(type_time)
        total_data = []
        for model in type_time:
            for op_type in type_time[model]:
                task_data = type_time[model][op_type]
                try:
                    task_duration = round(
                        float(task_data["duration"] / total_time[model] * 100), NumberConstant.DECIMAL_ACCURACY
                    )
                except ZeroDivisionError:
                    task_duration = 0
                total_data.append(
                    (
                        model,
                        task_data['op_type'],
                        str(task_data['task_type']),
                        task_data["count"],
                        round(float(task_data["duration"]), NumberConstant.DECIMAL_ACCURACY),
                        round(float(task_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                        round(float(task_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                        round(float(task_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                        task_duration,
                    )
                )
        if total_data:
            sorted_total_data = sorted(total_data, key=lambda x: x[7], reverse=True)
            sql = 'insert into {} values({})'.format(
                DBNameConstant.TABLE_OP_COUNTER_OP_REPORT, '?,' * (len(sorted_total_data[0]) - 1) + '?'
            )
            DBManager.executemany_sql(self.conn, sql, sorted_total_data)
