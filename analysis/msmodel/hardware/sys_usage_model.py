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

from abc import ABC

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class SysUsageModel(BaseModel, ABC):
    """
    acsq task model class
    """

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            table_map = "{0}DataMap".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def flush(self: any, data_list: dict) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        if data_list.get('sys_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_SYS_USAGE, data_list.get('sys_data_list'))
        if data_list.get('pid_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_PID_USAGE, data_list.get('pid_data_list'))

    def get_sys_cpu_data(self: any) -> list:
        sql = "select user,sys,iowait,idle,timestamp from {} where cpun = 'cpu';".format(DBNameConstant.TABLE_SYS_USAGE)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_pid_cpu_data(self: any, pid: int) -> list:
        sql = "select utime,stime,timestamp from {} where pid={};".format(DBNameConstant.TABLE_PID_USAGE, pid)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_all_pid(self: any) -> list:
        sql = "select pid from {};".format(DBNameConstant.TABLE_PID_USAGE)
        return DBManager.fetch_all_data(self.cur, sql)
