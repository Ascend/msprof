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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel
from msmodel.interface.view_model import ViewModel


class ClusterStepTraceModel(BaseModel):
    """
    Step trace model for cluster scene.
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_STEP_TRACE, table_list)

    def create_table(self: any) -> None:
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                DBManager.drop_table(self.conn, table_name)
            sql = DBManager.sql_create_general_table(f'{table_name.split("_")[0]}Map', table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def get_model_info(self: any, table_name: str) -> list:
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        sql = (
            'select t0.model_id, t0.max_index, group_concat(t0.iteration_id ) from (select t.* from'
            '(select iteration_id, model_id, ge_tag, (select count( * ) + 1 from {0} as t2 where '
            't2.model_id = t1.model_id and t2.iteration_time > t1.iteration_time ) as top,'
            '(select count(0) from {0} as t3 where t3.model_id = t1.model_id ) as max_index '
            'from {0} as t1 ) as t where top <= 5 and ge_tag = 1 order by model_id, top)t0 '
            'group by model_id, max_index'.format(table_name)
        )
        return DBManager.fetch_all_data(self.cur, sql)


class ClusterStepTraceViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_CLUSTER_STEP_TRACE, [])

    def get_sql_data(self: any, sql: str, param: tuple = None, dto_class: any = None) -> list:
        return DBManager.fetch_all_data(self.cur, sql, param, dto_class)

    def get_model_id_with_iterations(self: any, table_name: str) -> list:
        sql = "select model_id, count(distinct iteration_id) from {} group by model_id".format(table_name)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_iter_start_end(self: any, iteration_id: int, model_id: int, table_name: str) -> list:
        sql = (
            "select iteration_end - iteration_time, iteration_end from {} "
            "where iteration_id = ? and model_id = ?".format(table_name)
        )
        data = DBManager.fetch_all_data(self.cur, sql, (iteration_id, model_id))
        return data

    def judge_table_exist(self: any, table_name: str) -> bool:
        return DBManager.judge_table_exist(self.cur, table_name)
