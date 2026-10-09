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
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.block_log_dto import BlockLogDto
from profiling_bean.db_dto.task_time_dto import TaskTimeDto


class BlockLogModel(ParserModel):
    """
    AIC/AIV block model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush AIC/AIV block data to db
        :param data_list:AIC/AIV block data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_BLOCK_LOG, data_list)


class BlockLogViewModel(ViewModel):
    def get_timeline_data(self: any) -> list:
        """
        get timeline data from table
        :return: timeline data list
        """
        return self.get_all_data(DBNameConstant.TABLE_BLOCK_LOG)

    def get_summary_data(self: any) -> list:
        """
        get op_summary data from table
        :return: op_summary data list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_BLOCK_LOG):
            return []
        sql = (
            "select task_type, stream_id, task_id, task_time/{NS_TO_US} as task_time, "
            "start_time, end_time from {}".format(DBNameConstant.TABLE_BLOCK_LOG, NS_TO_US=NumberConstant.NS_TO_US)
        )
        task_time_data = DBManager.fetch_all_data(self.cur, sql, dto_class=TaskTimeDto)
        return task_time_data

    def get_block_log_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_BLOCK_LOG):
            return []
        # acsq task subtask_id is always 0xffffffff
        sql = (
            "select {1}.stream_id, {1}.task_id, {1}.block_id, {0} as context_id, {1}.start_time,"
            "task_time as duration,  {1}.task_type as device_task_type, {1}.core_type, {1}.core_id from {1} ".format(
                NumberConstant.DEFAULT_GE_CONTEXT_ID, DBNameConstant.TABLE_BLOCK_LOG
            )
        )
        device_tasks = DBManager.fetch_all_data(self.cur, sql, dto_class=BlockLogDto)
        if not device_tasks:
            logging.error(
                "get device AIC/AIV block from %s.%s error", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_BLOCK_LOG
            )
        return device_tasks
