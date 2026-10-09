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
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from msconfig.config_manager import ConfigManager
from msmodel.step_trace.ts_track_model import TsTrackModel
from viewer.calculate_rts_data import calculate_task_schedule_data
from viewer.calculate_rts_data import multi_calculate_task_cost_time


class TaskSchedulerCalculator(MsMultiProcess):
    """
    calculate origin data
    """

    TABLE_PATH = ConfigManager.TABLES
    COMPLETE_TIME_INDEX = 10

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)

    @staticmethod
    def _insert_report_task_data(runtime_conn: any, runtime_curs: any, device_id: str) -> None:
        report_data = calculate_task_schedule_data(runtime_curs, device_id)
        if not report_data:
            logging.info('Unable to get report task data')
            return
        sql = 'insert into ReportTask values({value})'.format(value='?,' * (len(report_data[0]) - 1) + '?')
        DBManager.executemany_sql(runtime_conn, sql, report_data)

    def ms_run(self: any) -> None:
        """
        entry
        :return:
        """
        if not ProfilingScene().is_operator():
            self.process()

    def process(self: any) -> None:
        """
        calculate for task scheduler
        :return:
        """
        try:
            self.generate_report_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))

    def create_task_time(self: any, runtime_conn: any, device: int, iter_time_range: list) -> None:
        """
        create task time table
        :param runtime_conn: connect for runtime
        :param device: device id
        :param iter_time_range: iteration range
        :return: None
        """
        runtime_curs = runtime_conn.cursor()
        logging.info('start create task time table')
        if not DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TIMELINE):
            logging.warning(
                "TimeLine data not found, and no need to create task time data, please check the ts track data."
            )
            return
        self._create_task_time_table(runtime_conn, runtime_curs)
        try:
            cal_task_data = self._get_timeline_data(device, iter_time_range, runtime_curs)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        task_time = self._add_info(sorted(cal_task_data, key=lambda x: x[-1]))
        self._collect_aicpu(task_time)
        self._insert_task_time_data(task_time, runtime_conn)
        logging.info('create task time table end')

    def insert_report_data(self: any, project_path: str, device_id: str) -> None:
        """
        insert data into report
        :param device_id:
        :param project_path: project path
        :return:
        """
        logging.info('start insert data into report table')
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME)
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(db_path)
        if (
            not runtime_conn
            or not runtime_curs
            or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        ):
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)
            return
        self._create_report_task_table(runtime_conn)
        self._insert_report_task_data(runtime_conn, runtime_curs, device_id)

        logging.info('Insert data into report table finished.')
        DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def generate_report_data(self: any) -> None:
        """
        insert report task table
        :return: None
        """
        devices = InfoConfReader().get_device_list()
        if not devices:
            logging.error("No device list found in info.json")
            return
        iter_time_range = MsprofIteration(self.project_path).get_step_syscnt_range_by_iter_range(self.iter_range)
        if not iter_time_range:
            return
        self.__pre_mini_task_data(self.project_path, devices[0], iter_time_range)
        self.insert_report_data(self.project_path, devices[0])

    def _insert_task_time_data(self: any, task_time: list, runtime_conn: any) -> None:
        # sort by complete time
        task_time = sorted(task_time, key=lambda data: data[self.COMPLETE_TIME_INDEX])
        insert_sql = "insert into TaskTime values ({value})".format(value="?," * (len(task_time[0]) - 1) + "?")
        DBManager.executemany_sql(runtime_conn, insert_sql, task_time)

    def _create_task_time_table(self: any, runtime_conn: any, runtime_curs: any) -> None:
        if DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        sql = DBManager.sql_create_general_table('TaskTimeMap', DBNameConstant.TABLE_RUNTIME_TASK_TIME, self.TABLE_PATH)
        DBManager.execute_sql(runtime_conn, sql)

    def _get_timeline_data(self: any, device: int, iter_time_range: list, runtime_curs: any) -> list:
        timeline_sql = (
            "select replayId,device_id,'','',taskType,"
            "task_id,stream_id,timeStamp,taskState "
            "from TimeLine WHERE device_id=? and timestamp>? and timestamp<?"
            "order by task_id, stream_id,timeStamp,taskState,device_id;"
        )
        timeline_data = DBManager.fetch_all_data(
            runtime_curs, timeline_sql, (device, iter_time_range[0], iter_time_range[1])
        )
        cal_task_data = multi_calculate_task_cost_time(timeline_data, self.project_path)
        return cal_task_data

    def __pre_mini_task_data(self: any, project_path: str, device_id: int, iter_time_range: list) -> None:
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(
            PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME)
        )
        if not runtime_conn or not runtime_curs:
            return
        try:
            self.create_task_time(runtime_conn, device_id, iter_time_range)
        except Exception as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def _create_report_task_table(self: any, runtime_conn: any) -> None:
        if DBManager.check_tables_in_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
            DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
        ):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_REPORT_TASK)
        sql = DBManager.sql_create_general_table('ReportTaskMap', 'ReportTask', self.TABLE_PATH)
        DBManager.execute_sql(runtime_conn, sql)

    def _add_info(self: any, cal_task_data: list) -> list:
        # 0 is default batch id
        with TsTrackModel(
            self.project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]
        ) as _trace:
            step_trace_data = _trace.get_step_end_list_with_iter_range(self.iter_range)
        task_time = [
            task_data
            + (
                MsprofIteration(self.project_path).get_iter_id_within_iter_range(
                    step_trace_data, task_data[-1], self.iter_range
                ),
                self.iter_range.model_id,
                NumberConstant.DEFAULT_BATCH_ID,
            )
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

            aicpu_data = (stream_id, task_id, start, end, task_type)
            aicpu_collector.filter_aicpu(aicpu_data)
        aicpu_collector.save_aicpu()
