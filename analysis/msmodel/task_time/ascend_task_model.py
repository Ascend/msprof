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

from typing import List, Tuple
from collections import namedtuple

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from mscalculate.ascend_task.ascend_task import TopDownTask
from msmodel.interface.base_model import BaseModel
from msmodel.interface.view_model import ViewModel


class AscendTaskModel(BaseModel):
    """
    class used to operate top-down task
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_ASCEND_TASK, table_list)

    def get_ascend_task_data_without_unknown(self: any) -> List[TopDownTask]:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ASCEND_TASK):
            return []
        param = (Constant.TASK_TYPE_UNKNOWN,)
        data_sql = "select * from {} where device_task_type != ?".format(DBNameConstant.TABLE_ASCEND_TASK)
        return DBManager.fetch_all_data(self.cur, data_sql, param=param, dto_class=TopDownTask)


class AscendTaskViewModel(ViewModel):
    """
    class used to operate top-down task
    """

    ASCEND_TASK_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple(
            "AscendTask",
            [
                "model_id",
                "index_id",
                "stream_id",
                "task_id",
                "context_id",
                "batch_id",
                "timestamp",
                "duration",
                "host_task_type",
                "device_task_type",
                "connection_id",
                "op_name",
                "ts_virtual_batch_id",
            ],
        ),
        {},
    )

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_ASCEND_TASK, table_list)

    def get_ascend_task_data_with_op_name_pattern_and_stream_id(
        self: any, device_id: str, op_name_pattern: str, stream_ids: tuple
    ) -> List[ASCEND_TASK_TYPE]:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ASCEND_TASK) or not self.attach_to_db(
            DBNameConstant.DB_GE_INFO
        ):
            return []
        stream_id_condition = "b.stream_id in ({})".format(",".join(map(str, stream_ids)))
        sql = (
            "SELECT b.model_id, b.index_id, b.stream_id, b.task_id, b.context_id, b.batch_id, b.start_time, "
            "b.duration, b.host_task_type, b.device_task_type, b.connection_id, a.op_name as op_name, "
            "-1 as ts_virtual_batch_id from {0} as a inner join {1} as b "
            "on a.stream_id = b.stream_id "
            "and a.task_id = b.task_id "
            "and a.batch_id = b.batch_id "
            "and a.context_id = b.context_id "
            "and a.device_id = {device_id} "
            "and b.start_time != {invalid_start} "
            "and (a.op_name like '%{pattern}' or {stream_id_condition}) "
            "order by start_time".format(
                DBNameConstant.TABLE_GE_TASK,
                DBNameConstant.TABLE_ASCEND_TASK,
                device_id=device_id,
                invalid_start=NumberConstant.INVALID_TASK_TIME,
                pattern=op_name_pattern,
                stream_id_condition=stream_id_condition,
            )
        )
        ascend_task_data = DBManager.fetch_all_data(self.cur, sql)
        return [self.ASCEND_TASK_TYPE(*data) for data in ascend_task_data]

    def get_ascend_task_data_with_stream_id(self: any, stream_ids: tuple) -> List[ASCEND_TASK_TYPE]:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ASCEND_TASK):
            return []
        stream_id_condition = "stream_id in ({})".format(",".join(map(str, stream_ids)))
        sql = (
            "SELECT model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, "
            "host_task_type, device_task_type, connection_id, 'N/A' as op_name, batch_id as ts_virtual_batch_id "
            "from {0} where {stream_id_condition} ".format(
                DBNameConstant.TABLE_ASCEND_TASK, stream_id_condition=stream_id_condition
            )
        )
        ascend_task_data = DBManager.fetch_all_data(self.cur, sql)
        return [self.ASCEND_TASK_TYPE(*data) for data in ascend_task_data]

    def get_ascend_task_time_extremes(self: any, filter_type: list) -> Tuple[TopDownTask, TopDownTask]:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ASCEND_TASK):
            return (None, None)
        type_condition = "host_task_type not in ({})".format(",".join(map(lambda x: f"'{x}'", filter_type)))

        latest_sql = (
            f"SELECT * FROM {DBNameConstant.TABLE_ASCEND_TASK} "
            f"WHERE {type_condition} "
            f"AND device_task_type != '{Constant.TASK_TYPE_UNKNOWN}'"
            f"ORDER BY (start_time + duration) DESC "
            f"LIMIT 1"
        )
        latest_result = DBManager.fetch_all_data(self.cur, latest_sql, dto_class=TopDownTask)
        latest_task = latest_result[0] if latest_result else None

        earliest_sql = (
            f"SELECT * FROM {DBNameConstant.TABLE_ASCEND_TASK} "
            f"WHERE {type_condition} "
            f"AND device_task_type != '{Constant.TASK_TYPE_UNKNOWN}'"
            f"ORDER BY start_time ASC "
            f"LIMIT 1"
        )
        earliest_result = DBManager.fetch_all_data(self.cur, earliest_sql, dto_class=TopDownTask)
        earliest_task = earliest_result[0] if earliest_result else None

        return latest_task, earliest_task
