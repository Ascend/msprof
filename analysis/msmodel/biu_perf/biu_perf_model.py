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

from common_func.db_manager import ClassRowType
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class BiuPerfModel(BaseModel):
    """
    db operator for runtime_api parser
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_BIU_PERF, table_list)

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush data to db
        :table_name: table name
        :param data_list: data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                DBManager.drop_table(self.conn, table_name)
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def get_all_data(self: any, table_name: str) -> list:
        """
        get all data from db
        :param cur:
        :param table_name:
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        self.cur.row_factory = ClassRowType.create_object
        all_data_sql = "select * from {}".format(table_name)
        return DBManager.fetch_all_data(self.cur, all_data_sql)

    def get_biu_flow_process(self: any) -> list:
        """
        get biu flow process for meta timeline. Pid and tid are unique, so min values are chosen
        """
        sql = "select 'process_name', min(pid), min(tid), unit_name from {} group by unit_name".format(
            DBNameConstant.TABLE_BIU_FLOW
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_biu_flow_thread(self: any) -> list:
        """
        get biu flow thread for meta timeline. Pid and tid are unique, so min values are chosen
        """
        sql = "select 'thread_name', min(pid), min(tid), flow_type from {} group by unit_name, flow_type".format(
            DBNameConstant.TABLE_BIU_FLOW
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_biu_cycles_process(self: any) -> list:
        """
        get biu cycles process for meta timeline. Pid and tid are unique, so min values are chosen
        """
        sql = "select 'process_name', min(pid), min(tid), unit_name from {} group by unit_name".format(
            DBNameConstant.TABLE_BIU_CYCLES
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_biu_cycles_thread(self: any) -> list:
        """
        get biu cycles thread for meta timeline. Pid and tid are unique, so min values are chosen
        """
        sql = "select 'thread_name', min(pid), min(tid), cycle_type from {} group by unit_name, cycle_type".format(
            DBNameConstant.TABLE_BIU_CYCLES
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_biu_flow_data(self: any) -> list:
        """
        get biu flow data
        """
        sql = "select flow_type, interval_start, pid, tid, flow from {}".format(DBNameConstant.TABLE_BIU_FLOW)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_biu_cycles_data(self: any) -> list:
        """
        get biu cycles data
        """
        sql = "select pid, tid, interval_start, duration, cycle_num, ratio from {}".format(
            DBNameConstant.TABLE_BIU_CYCLES
        )
        return DBManager.fetch_all_data(self.cur, sql)
