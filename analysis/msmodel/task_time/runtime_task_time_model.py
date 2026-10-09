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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.base_model import BaseModel
from msmodel.interface.sql_helper import SqlWhereCondition
from mscalculate.ascend_task.ascend_task import DeviceTask


class RuntimeTaskTimeModel(BaseModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_RUNTIME, [DBNameConstant.TABLE_RUNTIME_TASK_TIME])

    def get_runtime_task_data_within_time_range(self: any, start_time: float, end_time: float) -> list:
        # in this chip subtask_id is always 0xffffffff
        # in this chip task time is uploaded by ts, which is stored in ts_track.data
        sql = (
            "select {1}.stream_id, {1}.task_id, {0} as context_id, {1}.running as timestamp, "
            "{1}.complete - {1}.running as duration, {1}.tasktype as task_type from {1} "
            "{2} order by timestamp".format(
                NumberConstant.DEFAULT_GE_CONTEXT_ID,
                DBNameConstant.TABLE_RUNTIME_TASK_TIME,
                SqlWhereCondition.get_interval_intersection_condition(
                    start_time, end_time, DBNameConstant.TABLE_RUNTIME_TASK_TIME, "running", "complete"
                ),
            )
        )
        device_tasks = DBManager.fetch_all_data(self.cur, sql, dto_class=DeviceTask)
        if not device_tasks:
            logging.error(
                "get device task from %s.%s error", DBNameConstant.DB_HWTS, DBNameConstant.TABLE_HWTS_TASK_TIME
            )
        return device_tasks
