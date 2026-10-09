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


class SysMemModel(BaseModel, ABC):
    """
    acsq task model class
    """

    @staticmethod
    def class_name() -> None:
        """
        class name
        """
        return "SysMemModel"

    def flush(self: any, data_list: dict) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        if data_list.get('sys_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_SYS_MEM, data_list.get('sys_data_list'))
        if data_list.get('pid_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_PID_MEM, data_list.get('pid_data_list'))

    def get_sys_mem_data(self: any) -> list:
        sql = "select memtotal,memfree,timestamp from {};".format(DBNameConstant.TABLE_SYS_MEM)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_pid_mem_data(self: any, pid: int) -> list:
        sql = "select size,resident,shared,timestamp from {} where pid={};".format(DBNameConstant.TABLE_PID_MEM, pid)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_all_pid(self: any) -> list:
        sql = "select pid from {};".format(DBNameConstant.TABLE_PID_MEM)
        return DBManager.fetch_all_data(self.cur, sql)
