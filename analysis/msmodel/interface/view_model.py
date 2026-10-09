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

import sqlite3

from common_func.db_manager import DBManager
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel


class ViewModel(BaseModel):
    """
    class used to calculate
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.conn = None
        self.cur = None
        self.result_dir = result_dir
        self.db_name = db_name
        self.table_list = table_list

    def init(self: any) -> bool:
        """
        create db and tables
        """
        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        return True

    def get_sql_data(self: any, sql: str, dto_class: any = None) -> list:
        """
        get data from db
        :param sql:
        :return:
        """
        return DBManager.fetch_all_data(self.cur, sql, dto_class=dto_class)

    def attach_to_db(self: any, db_name: str) -> bool:
        """
        attach to other database
        :param db_name: attach name
        :return: Ture or False
        """
        attach_db_path = PathManager.get_db_path(self.result_dir, db_name)
        conn_check, _ = DBManager.check_connect_db_path(attach_db_path)
        if isinstance(conn_check, sqlite3.Connection):
            conn_check.close()
            self.cur.execute(
                "attach database '{0}' as {1}".format(attach_db_path, "{}_attach".format(db_name.split(".")[0]))
            )
            return True
        return False
