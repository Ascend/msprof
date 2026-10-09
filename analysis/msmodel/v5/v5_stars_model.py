# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.v5_pmu_dto import V5PmuDto
from mscalculate.ascend_task.ascend_task import DeviceTask


class V5StarsModel(ParserModel):
    """
    v5 device data model class
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_V5_TASK])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_V5_TASK) -> None:
        """
        insert data to table
        :param data_list: v5 device's info
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)


class V5StarsViewModel(ViewModel):
    """
    v5 device data view model class
    """

    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_SOC_LOG, [])

    def get_v5_data_within_time_range(self: any, start_time: float, end_time: float) -> list:
        # v5 task subtask_id is always 0xffffffff
        sql = (
            "select {1}.stream_id, {1}.task_id, {0} as context_id, {1}.start_time as timestamp, "
            "duration as duration,  {1}.task_type from {1} "
            "{2}".format(
                NumberConstant.DEFAULT_GE_CONTEXT_ID,
                DBNameConstant.TABLE_V5_TASK,
                SqlWhereCondition.get_interval_intersection_condition(
                    start_time, end_time, DBNameConstant.TABLE_V5_TASK, "start_time", "end_time"
                ),
            )
        )
        device_tasks = DBManager.fetch_all_data(self.cur, sql, dto_class=DeviceTask)
        if not device_tasks:
            logging.error(
                "get device v5 task from %s.%s error", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_V5_TASK
            )
        return device_tasks

    def get_v5_pmu_details(self) -> list:
        """
        get data for pmu calculate
        """
        sql = (
            "select stream_id, task_id, total_cycle, block_num, pmu0, pmu1, pmu2, pmu3,"
            "pmu4, pmu5, pmu6, pmu7, pmu8, pmu9 "
            "from {}".format(DBNameConstant.TABLE_V5_TASK)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=V5PmuDto)
