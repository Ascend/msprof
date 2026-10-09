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
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.sql_helper import SqlWhereCondition
from mscalculate.ascend_task.ascend_task import DeviceTask


class AiCpuModel(ParserModel):
    """
    ai_cpu model class
    """

    def __init__(self: any, result_dir: str, table_list: any = ()) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AI_CPU, table_list)

    def flush(self: any, data_list: list, ai_cpu_table_name: str = DBNameConstant.TABLE_AI_CPU) -> None:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        self.insert_data_to_db(ai_cpu_table_name, data_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                self.drop_table(table_name)
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def get_ai_cpu_data_from_ts(self: any) -> list:
        """
        get all data from db
        :param table_name: table name
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_AI_CPU_FROM_TS):
            return []
        all_data_sql = "select stream_id, task_id, sys_start, sys_end, batch_id from {}".format(
            DBNameConstant.TABLE_AI_CPU_FROM_TS
        )
        return DBManager.fetch_all_data(self.cur, all_data_sql)

    def get_ai_cpu_data_within_time_range(self: any, start_time: float, end_time: float) -> list:
        ai_cpu_sql = (
            "select {1}.stream_id, {1}.task_id, {0} as context_id, {1}.sys_start * {3} as timestamp, "
            "({1}.sys_end - {1}.sys_start) * {3} as duration, '{2}' as task_type from {1} "
            "{4}".format(
                NumberConstant.DEFAULT_GE_CONTEXT_ID,
                DBNameConstant.TABLE_AI_CPU_FROM_TS,
                Constant.TASK_TYPE_AI_CPU,
                NumberConstant.MS_TO_NS,
                SqlWhereCondition.get_interval_intersection_condition(
                    start_time / NumberConstant.MS_TO_NS,
                    end_time / NumberConstant.MS_TO_NS,
                    DBNameConstant.TABLE_AI_CPU_FROM_TS,
                    "sys_start",
                    "sys_end",
                ),
            )
        )
        ai_cpu_device_tasks = DBManager.fetch_all_data(self.cur, ai_cpu_sql, dto_class=DeviceTask)
        if not ai_cpu_device_tasks:
            logging.error(
                "no aicpu device task get from %s.%s", DBNameConstant.DB_AI_CPU, DBNameConstant.TABLE_AI_CPU_FROM_TS
            )
        return ai_cpu_device_tasks
