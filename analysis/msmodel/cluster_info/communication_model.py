# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
import logging
from dataclasses import dataclass
from dataclasses import field

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from msmodel.interface.view_model import ViewModel
from mscalculate.hccl.hccl_task import HcclTask


@dataclass
class OpTaskData:
    """op 与其 task 的聚合：op 侧字段(op_name/group_name/start/end) + 归属的 task 列表"""

    op_name: str = Constant.NA
    group_name: str = Constant.NA
    start: int = Constant.DEFAULT_VALUE
    end: int = Constant.DEFAULT_VALUE
    tasks: list = field(default_factory=list)


class CommunicationModel(ViewModel):
    """
    get hccl operators data from db
    """

    def __init__(self, collection_path):
        super().__init__(
            collection_path,
            DBNameConstant.DB_HCCL_SINGLE_DEVICE,
            [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, DBNameConstant.TABLE_KFC_TASK],
        )

    def get_all_events_from_db(self: any, conditions: dict, top_hccl_ops: tuple = None) -> dict:
        """
        get hccl op names
        :return: {(op_id, iter_id): OpTaskData}
        """
        tasks = []
        params = (conditions.get('iter_end', 0), conditions.get('iter_start', float('inf')))
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
            sql = self._build_task_sql(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)
            tasks = DBManager.fetch_all_data(self.cur, sql, params, dto_class=HcclTask)
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_TASK):
            sql = self._build_task_sql(DBNameConstant.TABLE_KFC_TASK)
            tasks += DBManager.fetch_all_data(self.cur, sql, params, dto_class=HcclTask)
        if not tasks:
            logging.error("Fail to connect %s, hccl parser is interrupted", DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            raise ProfException(ProfException.PROF_INVALID_CONNECT_ERROR)
        return self._group_tasks_by_op(tasks, top_hccl_ops)

    @staticmethod
    def _build_task_sql(task_table: str) -> str:
        """task 表已无 op_name 列，只按时间窗过滤；top_hccl_ops 筛选由 _group_tasks_by_op 按 op_name 匹配完成"""
        return "select * from {} where timestamp < ? and timestamp >= ?".format(task_table)

    def _group_tasks_by_op(self: any, tasks: list, top_hccl_ops: tuple = None) -> dict:
        """按 (op_id, iter_id) 把 task 归属到对应 op，返回 {key: OpTaskData}"""
        op_info = {}
        for op_table in (DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE, DBNameConstant.TABLE_KFC_OP):
            if not DBManager.judge_table_exist(self.cur, op_table):
                continue
            rows = DBManager.fetch_all_data(
                self.cur, "select connection_id, iter_id, op_name, group_name, start, end from {}".format(op_table)
            )
            for connection_id, iter_id, op_name, group_name, start, end in rows:
                if top_hccl_ops is not None and op_name not in top_hccl_ops:
                    continue
                op_info[(connection_id, iter_id)] = (op_name, group_name, start, end)
        op_task_map = {}
        for task in tasks:
            key = (task.op_id, task.iter_id)
            info = op_info.get(key)
            if info is None:
                # 无对应 op 的 task 不合法，跳过不处理
                continue
            op_task = op_task_map.get(key)
            if op_task is None:
                op_task = OpTaskData(op_name=info[0], group_name=info[1], start=info[2], end=info[3])
                op_task_map[key] = op_task
            op_task.tasks.append(task)
        return op_task_map

    def get_all_communication_data(self: any) -> list:
        data = []
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
            sql = "select * from {}".format(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)
            data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_TASK):
            sql = "select * from {}".format(DBNameConstant.TABLE_KFC_INFO)
            data += DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)
        return data
