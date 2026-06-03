# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from common_func.ms_constant.number_constant import NumberConstant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from profiling_bean.db_dto.fusion_task_dto import FusionTaskDto


class FusionTaskModel(ParserModel):
    """
    fusion task model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush fusion task data to db
        :param data_list: fusion task data list
        """
        self.insert_data_to_db(DBNameConstant.TABLE_FUSION_TASK, data_list)

    def get_summary_data(self: any) -> list:
        """
        get summary data from table
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_FUSION_TASK):
            return []
        sql = (
            "select fusion_task_type as op_name, task_type, stream_id, task_id, "
            "task_time/{NS_TO_US} as task_time, "
            "start_time, end_time, mission_id, ccu_die_id from {table}".format(  # nosec B608
                table=DBNameConstant.TABLE_FUSION_TASK, NS_TO_US=NumberConstant.NS_TO_US
            )
        )  # nosec B608
        return DBManager.fetch_all_data(self.cur, sql, dto_class=FusionTaskDto)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from table
        """
        return self.get_all_data(DBNameConstant.TABLE_FUSION_TASK)
