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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.ascend_task.ascend_task import TopDownTask
from msconfig.config_manager import ConfigManager
from msmodel.task_time.ascend_task_model import AscendTaskModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class OpSummaryOpSceneCalculator(MsMultiProcess):
    """
    op summary for single op
    """

    TASK_TIME_COL_NUM = 7

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self.conn = None
        self.curs = None

    @staticmethod
    def _get_ge_sql() -> str:
        device_id = InfoConfReader().get_device_id()
        ge_sql = (
            "SELECT model_id, task_id, stream_id, "
            "op_name, op_type, op_state, block_num, mix_block_num, task_type, "
            "tensor_num, input_formats, input_data_types, input_shapes, output_formats, output_data_types,"
            "output_shapes, index_id, timestamp, batch_id, context_id, op_flag from {0} where device_id = {1}".format(
                DBNameConstant.TABLE_GE_TASK, device_id
            )
        )
        return ge_sql

    @staticmethod
    def _create_core_table(curs: any, table_name: str) -> str:
        cols_infos = []
        cols_with_type = DBManager.get_table_info(curs, table_name)
        for col, col_type in cols_with_type.items():
            cols_infos.append("[{}] {}".format(col, col_type))
        return ",".join(cols_infos)

    def ms_run(self: any) -> None:
        """
        process
        :return: None
        """
        if ProfilingScene().is_all_export() or ProfilingScene().is_step_export():
            self.process()

    def process(self: any) -> None:
        """
        run for op summary
        """
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            logging.info("The db %s already exists, and won't create again.", DBNameConstant.DB_AICORE_OP_SUMMARY)
            return
        if not os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_ASCEND_TASK)):
            logging.warning(
                "No %s found, no need to create %s", DBNameConstant.DB_ASCEND_TASK, DBNameConstant.DB_AICORE_OP_SUMMARY
            )
            return
        self.create_summary_table()

    def create_ge_summary_table(self: any) -> bool:
        """
        create ge summary table
        """
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if not DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            return False

        ge_merge_data = self._get_ge_merge_data()
        if not ge_merge_data:
            return False

        create_ge_summary_sql = DBManager.sql_create_general_table(
            "SummaryGeMap", DBNameConstant.TABLE_SUMMARY_GE, ConfigManager.TABLES_OPERATOR
        )
        DBManager.execute_sql(self.conn, create_ge_summary_sql)

        insert_sql = "insert into {0} values({value})".format(
            DBNameConstant.TABLE_SUMMARY_GE, value="?," * (len(ge_merge_data[0]) - 1) + "?"
        )
        DBManager.executemany_sql(self.conn, insert_sql, ge_merge_data)
        return True

    def create_ai_core_metrics_table(self: any) -> bool:
        """
        create ai core metrics table
        """
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRIC_SUMMARY):
            core_merge_data = self._get_ai_core_metric(DBNameConstant.TABLE_METRIC_SUMMARY)
        elif DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_AIV_METRIC_SUMMARY):
            core_merge_data = self._get_ai_core_metric(DBNameConstant.TABLE_AIV_METRIC_SUMMARY)
        else:
            return False
        if core_merge_data:
            insert_sql = "insert into {0} values({value})".format(
                DBNameConstant.TABLE_SUMMARY_METRICS, value="?," * (len(core_merge_data[0]) - 1) + "?"
            )
            DBManager.executemany_sql(self.conn, insert_sql, core_merge_data)
            return True
        return False

    def get_task_time_data(self: any) -> list:
        """
        get task time data
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

    def create_task_time_table(self: any) -> bool:
        """
        create task time table
        """
        ascend_task_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_ASCEND_TASK)

        if not os.path.exists(ascend_task_db_path):
            logging.warning(
                "no task db %s found, task_time table will not be created", DBNameConstant.TABLE_ASCEND_TASK
            )
            return False
        create_table_sql = DBManager.sql_create_general_table(
            "ModifiedTaskTimeMap", DBNameConstant.TABLE_SUMMARY_TASK_TIME, ConfigManager.TABLES_OPERATOR
        )
        DBManager.execute_sql(self.conn, create_table_sql)

        data = self.get_task_time_data()
        if not data:
            return False
        insert_sql = 'insert or ignore into {0} values ({value})'.format(
            DBNameConstant.TABLE_SUMMARY_TASK_TIME, value="?," * (len(data[0]) - 1) + "?"
        )
        DBManager.executemany_sql(self.conn, insert_sql, data)
        return True

    def create_summary_table(self: any) -> None:
        """
        store ge graph and task data in ge_info.db
        """
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if not DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            logging.warning("Try to export op summary without ge data, maybe the data of framework is not collected.")

        self.conn, self.curs = DBManager.create_connect_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        )
        if self.conn and self.curs:
            self._create_summary_table_helper()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def _get_ge_data_from_summary(self: any) -> list:
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_SUMMARY_GE):
            return []
        ge_sql = "SELECT task_type, stream_id, task_id, batch_id, context_id from {0}".format(
            DBNameConstant.TABLE_SUMMARY_GE
        )
        return DBManager.fetch_all_data(self.curs, ge_sql, dto_class=GeTaskDto)

    def _get_ge_merge_data(self: any) -> list:
        ge_result = []
        ge_conn, ge_curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_GE_INFO)
        if not (ge_conn and ge_curs):
            DBManager.destroy_db_connect(ge_conn, ge_curs)
            return ge_result
        ge_merge_sql = self._get_ge_sql()
        ge_result = DBManager.fetch_all_data(ge_curs, ge_merge_sql)
        DBManager.destroy_db_connect(ge_conn, ge_curs)
        return ge_result

    def _get_ai_core_metric(self: any, table_name: str) -> list:
        core_data = []
        core_conn, core_curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_METRICS_SUMMARY)
        if not (core_conn and core_curs):
            DBManager.destroy_db_connect(core_conn, core_curs)
            return core_data
        sql = (
            "create table if not exists {0} (".format(DBNameConstant.TABLE_SUMMARY_METRICS)
            + self._create_core_table(core_curs, table_name)
            + ")"
        )
        DBManager.execute_sql(self.conn, sql)

        sql = "select * from {0}".format(table_name)
        core_data = DBManager.fetch_all_data(core_curs, sql)
        DBManager.destroy_db_connect(core_conn, core_curs)
        return core_data

    def _create_summary_table_helper(self: any) -> None:
        if not self.create_ge_summary_table():
            logging.warning("unable to create ge summary table")
        if not self.create_ai_core_metrics_table():
            logging.warning("unable to create ai core metrics table")
        if not self.create_task_time_table():
            logging.warning("unable to create task time table")
