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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from msconfig.config_manager import ConfigManager
from viewer.calculate_rts_data import calculate_task_schedule_data
from viewer.calculate_rts_data import multi_calculate_task_cost_time


class OpTaskSchedulerCalculator(MsMultiProcess):
    """
    analysis task data for scene of operator
    """

    STREAM_TASK_FMT = "{0}-{1}"
    COMPLETE_TIME_INDEX = 9

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self.device_id = sample_config.get(StrConstant.PARAM_DEVICE_ID, 0)
        self.iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self.task_data = []
        self.cnt_miss = -1

    @staticmethod
    def _create_task_time_table(runtime_conn: any, runtime_curs: any) -> None:
        if DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        sql = DBManager.sql_create_general_table(
            'TaskTimeMap', DBNameConstant.TABLE_RUNTIME_TASK_TIME, ConfigManager.TABLES_OPERATOR
        )
        DBManager.execute_sql(runtime_conn, sql)

    @staticmethod
    def _insert_report_task_data(runtime_conn: any, runtime_curs: any, device_id: int) -> None:
        report_data = calculate_task_schedule_data(runtime_curs, device_id)
        if not report_data:
            logging.info('Unable to get report task data')
            return
        sql = 'insert into {0} values({value})'.format(
            DBNameConstant.TABLE_RUNTIME_REPORT_TASK, value='?,' * (len(report_data[0]) - 1) + '?'
        )
        DBManager.executemany_sql(runtime_conn, sql, report_data)

    def ms_run(self: any) -> None:
        """
        entry
        :return: None
        """
        if ProfilingScene().is_operator():
            self.process()

    def process(self: any) -> None:
        """
        parsing task process
        :return: None
        """
        try:
            self.op_generate_report_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))

    def op_create_task_time(self: any, runtime_conn: any, device: int) -> None:
        """
        create task time table
        :param runtime_conn: connect for runtime
        :param device: device id
        :return: NA
        """
        logging.info('start create task time table')
        runtime_curs = runtime_conn.cursor()
        if not DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TIMELINE):
            logging.info("TimeLine table doesn't exist")
            return
        self._create_task_time_table(runtime_conn, runtime_curs)
        try:
            cal_task_data = self._get_timeline_data(device, runtime_curs)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        task_time = self._add_info(cal_task_data)
        self._collect_aicpu(task_time)
        self._insert_task_time_data(task_time, runtime_conn, runtime_curs)
        logging.info('create task time table end')

    def op_pre_mini_task_data(self: any, project_path: str, device_id: int) -> None:
        """
        deal with the ts track data for operator
        :param project_path: project path
        :param device_id: device id
        :return: NA
        """
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(
            PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME)
        )
        if not runtime_conn or not runtime_curs:
            return
        try:
            self.op_create_task_time(runtime_conn, device_id)
        except Exception as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def op_insert_report_data(self: any, project_path: str, device_id: int) -> None:
        """
        insert data into report
        :param project_path: project path
        :param device_id: device id
        :return: None
        """
        logging.info('start insert op data into report table')
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME)
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(db_path)
        if (
            not runtime_conn
            or not runtime_curs
            or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        ):
            return
        self._create_report_task_table(runtime_conn)
        self._insert_report_task_data(runtime_conn, runtime_curs, device_id)
        logging.info('Insert op data into report table finished.')
        DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def op_generate_report_data(self: any) -> None:
        """
        insert report task table
        :return: None
        """
        if DBManager.check_tables_in_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
            DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
        ):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
                DBNameConstant.DB_RUNTIME,
            )
            return
        self.op_pre_mini_task_data(self.project_path, self.device_id)
        self.op_insert_report_data(self.project_path, self.device_id)

    def _add_info(self: any, cal_task_data: list) -> list:
        # 0 is default batch id
        task_time = [
            task_data + (self.iter_range.iteration_id, NumberConstant.INVALID_MODEL_ID, NumberConstant.DEFAULT_BATCH_ID)
            for task_data in cal_task_data
        ]
        return task_time

    def _collect_aicpu(self: any, task_time: list) -> None:
        aicpu_collector = AICpuFromTsCollector(self.project_path)

        for data in task_time:
            task_id = data[5]
            stream_id = data[6]
            start = data[9]
            end = data[10]
            task_type = data[4]

            aicpu_feature = (stream_id, task_id, start, end, task_type)
            aicpu_collector.filter_aicpu(aicpu_feature)
        aicpu_collector.save_aicpu()

    def _insert_task_time_data(self: any, task_time: list, runtime_conn: any, runtime_curs: any) -> None:
        # sort by complete time
        task_time = sorted(task_time, key=lambda data: data[self.COMPLETE_TIME_INDEX])
        insert_sql = "insert into TaskTime values ({value})".format(value="?," * (len(task_time[0]) - 1) + "?")
        DBManager.executemany_sql(runtime_conn, insert_sql, task_time)

    def _get_timeline_data(self: any, device: int, runtime_curs: any) -> list:
        timeline_sql = (
            "select replayId,device_id,'','',taskType,"
            "task_id,stream_id,timeStamp,taskState "
            "from TimeLine WHERE device_id=?"
            "order by task_id, stream_id,timeStamp,taskState,device_id;"
        )
        timeline_data = DBManager.fetch_all_data(runtime_curs, timeline_sql, (device,))
        cal_task_data = multi_calculate_task_cost_time(timeline_data, self.project_path)
        return cal_task_data

    def _create_report_task_table(self: any, runtime_conn: any) -> None:
        if DBManager.check_tables_in_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
            DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
        ):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_REPORT_TASK)
        sql = DBManager.sql_create_general_table(
            'ReportTaskMap', DBNameConstant.TABLE_RUNTIME_REPORT_TASK, ConfigManager.TABLES_OPERATOR
        )
        DBManager.execute_sql(runtime_conn, sql)
