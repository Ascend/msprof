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
from common_func.profiling_scene import ProfilingScene
from mscalculate.ascend_task.ascend_task import TopDownTask
from msconfig.config_manager import ConfigManager
from msmodel.task_time.ascend_task_model import AscendTaskModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class ParseAiCoreOpSummaryCalculator(MsMultiProcess):
    """
    handler ai core op data and get a summary data sheet
    """

    TASK_TIME_COL_NUM = 8
    TRAIN_TASK_TIME_COL_NUM = 7
    TABLE_PATH = ConfigManager.TABLES
    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.conn = None
        self.curs = None

    def ms_run(self: any) -> None:
        """
        entry
        :return:
        """
        if ProfilingScene().is_graph_export():
            self.process()

    def process(self: any) -> None:
        """
        entry for analysis op summary data
        :return: None
        """
        if not os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_ASCEND_TASK)):
            logging.warning(
                "No %s found, no need to create %s", DBNameConstant.DB_ASCEND_TASK, DBNameConstant.DB_AICORE_OP_SUMMARY
            )
            return
        try:
            self.init_params()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.create_summary_table()
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def init_params(self: any) -> None:
        """
        initial and check params
        :return: None
        """
        if self.project_path is None or not os.path.exists(self.project_path):
            logging.error("Failed to get project path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        sql_dir = PathManager.get_sql_dir(self.project_path)
        if not os.path.exists(sql_dir):
            logging.error("Failed to get sqlite path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            os.remove(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY))

    def create_summary_table(self: any) -> None:
        """
        store ge graph and task data in ge_info.db
        :return: None
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            logging.warning("maybe the data of framework is not collected.try to export data with no framework data.")
            if not DBManager.check_tables_in_db(
                self.get_db_path(DBNameConstant.DB_METRICS_SUMMARY), DBNameConstant.TABLE_METRIC_SUMMARY
            ):
                logging.warning("No need to create db for op summary, maybe the data of aicore is not collected.")
                return
        self.create_conn()
        if not (self.conn and self.curs):
            return
        self.create_ge_summary_table()
        self.create_ai_core_metrics_table()
        self.create_task_time_table()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def create_conn(self: any) -> None:
        """
        create connection
        :return: connect and cursor
        """
        conn_path = self.get_db_path(DBNameConstant.DB_AICORE_OP_SUMMARY)
        self.conn, self.curs = DBManager.create_connect_db(conn_path)
        os.chmod(conn_path, NumberConstant.FILE_AUTHORITY)

    def create_ge_summary_table(self: any) -> None:
        """
        create ge summary table
        :return: None
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            logging.warning(
                "unable to create ge summary table, because table %s is not found.", DBNameConstant.TABLE_GE_TASK
            )
            return
        if not DBManager.attach_to_db(self.conn, self.project_path, DBNameConstant.DB_GE_INFO, "ge_info"):
            logging.warning("unable to create ge summary table, because attach db of ge failed.")
            return
        ge_create_sql = DBManager.sql_create_general_table(
            "GeSummaryMap", DBNameConstant.TABLE_SUMMARY_GE, self.TABLES_PATH
        )
        DBManager.execute_sql(self.conn, ge_create_sql)
        ge_data = self._get_ge_data()
        DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_SUMMARY_GE, ge_data)

    def create_ai_core_metrics_table(self: any) -> None:
        """
        create ai core metrics table
        :return: None
        """
        db_name = os.path.splitext(DBNameConstant.DB_METRICS_SUMMARY)[0]
        if not DBManager.attach_to_db(self.conn, self.project_path, DBNameConstant.DB_METRICS_SUMMARY, db_name):
            logging.warning("unable to create ai core metrics table, because attach db of runtime failed.")
            return
        if DBManager.check_tables_in_db(
            self.get_db_path(DBNameConstant.DB_METRICS_SUMMARY), DBNameConstant.TABLE_METRIC_SUMMARY
        ):
            sql = "create table if not exists ai_core_metrics as select * from {0}.{1}".format(
                db_name, DBNameConstant.TABLE_METRIC_SUMMARY
            )
        elif DBManager.check_tables_in_db(
            self.get_db_path(DBNameConstant.DB_METRICS_SUMMARY), DBNameConstant.TABLE_AIV_METRIC_SUMMARY
        ):
            sql = "create table if not exists ai_core_metrics as select * from {0}.{1}".format(
                db_name, DBNameConstant.TABLE_AIV_METRIC_SUMMARY
            )
        else:
            logging.warning("unable to create ai core metrics table, because table is not found.")
            return
        DBManager.execute_sql(self.conn, sql)

    def create_task_time_table(self: any) -> None:
        """
        create task time table
        :return: true or false
        """
        ascend_task_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_ASCEND_TASK)

        if not os.path.exists(ascend_task_db_path):
            logging.warning(
                "no task db %s found, task_time table will not be created", DBNameConstant.TABLE_ASCEND_TASK
            )
            return
        create_table_sql = DBManager.sql_create_general_table("ModifiedTaskTimeMap", "task_time", self.TABLE_PATH)
        if not create_table_sql:
            logging.error("unable to create task time table, generate sql statement failed!")
            return
        DBManager.execute_sql(self.conn, create_table_sql)
        data = self.get_task_time_data()
        if not data:
            logging.warning("unable to create task time table, because no task data found.")
            return
        insert_sql = 'insert or ignore into {0} values ({value})'.format(
            DBNameConstant.TABLE_SUMMARY_TASK_TIME, value="?," * (len(data[0]) - 1) + "?"
        )
        DBManager.executemany_sql(self.conn, insert_sql, data)

    def get_task_time_data(self: any) -> list:
        """
        get task time data
        :return: task data list
        """
        conn, curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_ASCEND_TASK)
        if not (conn and curs):
            DBManager.destroy_db_connect(conn, curs)
            return []
        with AscendTaskModel(self.project_path, DBNameConstant.TABLE_ASCEND_TASK) as model:
            tasks = model.get_all_data(DBNameConstant.TABLE_ASCEND_TASK)
            if not tasks:
                logging.error("Get tasks from %s error", DBNameConstant.TABLE_ASCEND_TASK)
                return []
            ascend_tasks = [TopDownTask(*task) for task in tasks]
            return [
                [
                    task.task_id,
                    task.stream_id,
                    task.start_time,
                    task.duration,
                    0,
                    task.device_task_type,
                    task.index_id,
                    task.model_id,
                    task.batch_id,
                    task.context_id,
                ]
                for task in ascend_tasks
            ]

    def get_db_path(self: any, db_name: str) -> str:
        """
        get database path
        :param db_name: db name
        :return: path of db
        """
        return PathManager.get_db_path(self.project_path, db_name)

    def _get_ge_data_from_summary(self: any) -> list:
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_SUMMARY_GE):
            return []
        ge_sql = "SELECT task_type, stream_id, task_id, batch_id, context_id from {0}".format(
            DBNameConstant.TABLE_SUMMARY_GE
        )
        return DBManager.fetch_all_data(self.curs, ge_sql, dto_class=GeTaskDto)

    def _get_ge_data(self: any) -> list:
        ge_data = []
        iter_list = MsprofIteration(self.project_path).get_index_id_list_with_index_and_model(self.iter_range)
        device_id = InfoConfReader().get_device_id()
        ge_sql = (
            f"SELECT model_id, batch_id, task_id, stream_id, "
            f"op_name, op_type, op_state, block_num, mix_block_num, task_type, tensor_num, input_formats,"
            f" input_data_types, input_shapes, output_formats, output_data_types,"
            f" output_shapes, timestamp, index_id, context_id, op_flag "
            f"from {DBNameConstant.TABLE_GE_TASK} where index_id=? and model_id=? and device_id=?"
        )
        for index_and_model in iter_list:
            index_and_model_list = list(index_and_model)
            index_and_model_list.append(device_id)
            index_and_model = tuple(index_and_model_list)
            ge_data.extend(DBManager.fetch_all_data(self.curs, ge_sql, index_and_model))
        return ge_data
