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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.utils import Utils
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.sql_helper import SqlWhereCondition
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from mscalculate.ascend_task.ascend_task import DeviceTask


class FftsLogModel(ParserModel):
    """
    ffts thread log and subtask log model
    """

    def insert_log_data(self: any, data_list: list) -> None:
        """
        insert soc log data to db
        :param data_list: data list
        :return: None
        """
        result = Utils.generator_to_list(
            (
                data.stars_common.stream_id,
                data.stars_common.task_id,
                data.subtask_id,
                data.thread_id,
                StarsConstant.SUBTASK_TYPE.get(data.subtask_type, data.subtask_type),
                StarsConstant.FFTS_TYPE.get(data.ffts_type, data.ffts_type),
                data.func_type,
                data.stars_common.timestamp,
            )
            for data in data_list
        )

        self.insert_data_to_db(DBNameConstant.TABLE_FFTS_LOG, result)

    def flush(self: any, data_list: list) -> None:
        """
        flush data list to db
        :param data_list:
        :return: None
        """
        self.insert_log_data(data_list)

    def get_summary_data(self: any) -> list:
        """
        to get timeline data from database
        :return: result list
        """
        return self.get_all_data(DBNameConstant.TABLE_FFTS_LOG)

    def get_ffts_log_data(self: any) -> list:
        """
        get ffts log data from database
        :return: result list
        """
        return self.get_all_data(DBNameConstant.TABLE_FFTS_LOG, dto_class=TaskTimeDto)

    def get_ffts_task_data(self):
        """
        get ffts task data
        :return:
        """
        sql = "select subtask_id, task_id, stream_id, start_time, dur_time from {0} order by start_time ".format(
            DBNameConstant.TABLE_SUBTASK_TIME
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_ffts_plus_sub_task_data_within_time_range(self: any, start_time: float, end_time: float) -> list:
        # ffts+ task subtask_id is stored in db
        sql = (
            "select {0}.stream_id, {0}.task_id, {0}.subtask_id as context_id, {0}.start_time as timestamp, "
            "({0}.end_time - {0}.start_time) as duration, {0}.subtask_type as task_type from {0} "
            "{1} order by timestamp".format(
                DBNameConstant.TABLE_SUBTASK_TIME,
                SqlWhereCondition.get_interval_intersection_condition(
                    start_time, end_time, DBNameConstant.TABLE_SUBTASK_TIME, "start_time", "end_time"
                ),
            )
        )
        device_tasks = DBManager.fetch_all_data(self.cur, sql, dto_class=DeviceTask)
        if not device_tasks:
            logging.error(
                "get device ffts plus sub task from %s.%s error",
                DBNameConstant.DB_SOC_LOG,
                DBNameConstant.TABLE_SUBTASK_TIME,
            )
        return device_tasks

    def _get_thread_time_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_THREAD_TASK, dto_class=TaskTimeDto)

    def _get_subtask_time_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_SUBTASK_TIME, dto_class=TaskTimeDto)
