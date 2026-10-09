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
from collections import deque

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from msmodel.stars.ffts_log_model import FftsLogModel
from msmodel.stars.sub_task_model import SubTaskTimeModel
from profiling_bean.prof_enum.data_tag import DataTag


class SubTaskCalculator(MsMultiProcess):
    """
    calculate subtask data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.file_list = file_list
        self.result_dir = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.conn = None
        self.cur = None
        self.subtask_time_data = []

    @staticmethod
    def _get_thread_task_time_sql() -> str:
        """
        table ffts_log have both thread start logs and end logs
        Timestamps of logs with the same subtask_ID and task_id and stream_id
        are subtracted to obtain the dur_time.
        """
        thread_base_sql = (
            "Select end_log.subtask_id as subtask_id, end_log.task_id as task_id,"
            "end_log.stream_id as stream_id,end_log.subtask_type as subtask_type, "
            "end_log.ffts_type as ffts_type,end_log.thread_id as thread_id,"
            "start_log.task_time as task_time,"
            "(end_log.task_time-start_log.task_time) as dur_time "
            "from {0} end_log "
            "join {0} start_log on end_log.thread_id=start_log.thread_id "
            "and end_log.subtask_id=start_log.subtask_id "
            "and end_log.stream_id=start_log.stream_id "
            "and end_log.task_id=start_log.task_id "
            "where end_log.task_type='{1}' and start_log.task_type='{2}' "
            "group by end_log.subtask_id,end_log.thread_id, end_log.task_id "
            "order by end_log.subtask_id".format(
                DBNameConstant.TABLE_FFTS_LOG, StarsConstant.FFTS_LOG_END_TAG, StarsConstant.FFTS_LOG_START_TAG
            )
        )

        return thread_base_sql

    def get_subtask_time(self) -> list:
        with FftsLogModel(self.result_dir, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_FFTS_LOG]) as ffts_model:
            ffts_log_data = ffts_model.get_ffts_log_data()
        task_map = {}
        ffts_log_data.sort(key=lambda x: x.task_time)
        for data in ffts_log_data:
            task_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.subtask_id, data.thread_id)
            task_map.setdefault(task_key, {}).setdefault(data.task_type, deque([])).append(data)
        matched_result = []
        mismatch_start_count = 0
        mismatch_end_count = 0
        notify_mismatch = 0
        for task_key, data in task_map.items():
            start_que = data.get(StarsConstant.FFTS_LOG_START_TAG, [])
            end_que = data.get(StarsConstant.FFTS_LOG_END_TAG, [])
            while start_que and end_que:
                start_task = start_que[0]
                end_task = end_que[0]
                if start_task.task_time > end_task.task_time:
                    mismatch_end_count += 1
                    _ = end_que.popleft()
                    continue
                start_task = start_que.popleft()
                end_task = end_que.popleft()
                matched_result.append(
                    [
                        start_task.subtask_id,
                        start_task.task_id,
                        start_task.stream_id,
                        start_task.subtask_type,
                        start_task.ffts_type,
                        start_task.task_time,
                        end_task.task_time,
                        end_task.task_time - start_task.task_time,
                        int(start_task.thread_id),
                    ]
                )
            for start_task in start_que:
                if start_task.subtask_type == "Notify Wait":
                    notify_mismatch += 1
                    matched_result.append(
                        [
                            start_task.subtask_id,
                            start_task.task_id,
                            start_task.stream_id,
                            start_task.subtask_type,
                            start_task.ffts_type,
                            start_task.task_time,
                            start_task.task_time,
                            0,
                            int(start_task.thread_id),
                        ]
                    )
            if start_que or end_que:
                logging.debug(
                    "subtask_time task mismatch happen in %s, start_que size: %d, end_que size: %d",
                    task_key,
                    len(start_que),
                    len(end_que),
                )
                mismatch_start_count += len(start_que)
                mismatch_end_count += len(end_que)
        if mismatch_end_count > 0:
            logging.warning("There are %d subtask_time end logs mismatching.", mismatch_end_count)
        if notify_mismatch > 0:
            logging.error("There are %d notify wait mismatching.", notify_mismatch)
        if mismatch_start_count > 0:
            logging.error("There are %d subtask_time start logs mismatching.", mismatch_start_count)
        return sorted(matched_result, key=lambda data: data[5])  # data[5] represents subtask start time

    def ms_run(self: any) -> None:
        """
        calculate for subtask
        :return: None
        """
        if not self.file_list.get(DataTag.STARS_LOG) or not ChipManager().is_ffts_plus_type():
            return
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if ProfilingScene().is_all_export() and DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_THREAD_TASK):
            logging.info(
                "The Table %s already exists in the %s, and won't be calculate again.",
                DBNameConstant.TABLE_THREAD_TASK,
                DBNameConstant.DB_SOC_LOG,
            )
            return
        self.calculate()

    def calculate(self: any) -> None:
        try:
            self.init()
        except ValueError:
            logging.warning("calculate subtask failed, maybe the data is not in fftsplus mode")
            return
        self.subtask_time_data = self.get_subtask_time()
        try:
            self.save()
        except ValueError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        finally:
            DBManager().destroy_db_connect(self.conn, self.cur)

    def init(self: any) -> None:
        self.conn, self.cur = DBManager().check_connect_db(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if (
            not self.conn
            or not self.cur
            or not DBManager().check_tables_in_db(
                PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG), DBNameConstant.TABLE_FFTS_LOG
            )
        ):
            raise ValueError

    def save(self: any) -> None:
        self.__create_log_table(DBNameConstant.TABLE_THREAD_TASK, self._get_thread_task_time_sql())
        with SubTaskTimeModel(self.result_dir) as subtask_model:
            subtask_model.flush(self.subtask_time_data)

    def __create_log_table(self: any, table_name: str, sql: str) -> None:
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        create_sql = "create table {0} as {1}".format(table_name, sql)
        self.cur.execute(create_sql)
