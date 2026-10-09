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
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.sql_helper import SqlWhereCondition
from msmodel.sqe_type_map import SqeType
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from mscalculate.ascend_task.ascend_task import DeviceTask


class AcsqTaskModel(ParserModel):
    """
    acsq task model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_ACSQ_TASK, data_list)

    def get_summary_data(self: any) -> list:
        """
        get op_summary data from table
        :return: op_summary data list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACSQ_TASK):
            return []
        sql = (
            "select 'N/A' as op_name, task_type, stream_id, task_id, task_time/{NS_TO_US} as task_time, "
            "start_time, end_time from {}".format(DBNameConstant.TABLE_ACSQ_TASK, NS_TO_US=NumberConstant.NS_TO_US)
        )
        task_time_data = DBManager.fetch_all_data(self.cur, sql, dto_class=TaskTimeDto)
        return task_time_data

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from table
        :return: timeline data list
        """
        return self.get_all_data(DBNameConstant.TABLE_ACSQ_TASK)

    def get_ffts_type_data(self: any) -> list:
        """
        get all ffts type data
        :param step_start:
        :param step_end:
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACSQ_TASK):
            return []
        sql = "select 0, task_id, stream_id, start_time, task_time from {0} where task_type={task_type}".format(
            DBNameConstant.TABLE_ACSQ_TASK, task_type=SqeType().instance.AI_CORE.name
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_acsq_data_within_time_range(self: any, start_time: float, end_time: float) -> list:
        # acsq task subtask_id is always 0xffffffff
        sql = (
            "select {1}.stream_id, {1}.task_id, {0} as context_id, {1}.start_time as timestamp, "
            "task_time as duration,  {1}.task_type from {1} "
            "{2}".format(
                NumberConstant.DEFAULT_GE_CONTEXT_ID,
                DBNameConstant.TABLE_ACSQ_TASK,
                SqlWhereCondition.get_interval_intersection_condition(
                    start_time, end_time, DBNameConstant.TABLE_ACSQ_TASK, "start_time", "end_time"
                ),
            )
        )
        device_tasks = DBManager.fetch_all_data(self.cur, sql, dto_class=DeviceTask)
        if not device_tasks:
            logging.error(
                "get device acsq task from %s.%s error", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_ACSQ_TASK
            )
        return device_tasks
