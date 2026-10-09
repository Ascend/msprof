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
from abc import ABCMeta

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.path_manager import PathManager
from msconfig.config_manager import ConfigManager


class BaseModel(metaclass=ABCMeta):
    """
    stars base model class. Used to operate db.
    """

    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        self.result_dir = result_dir
        self.conn = None
        self.cur = None
        self.table_list = table_list
        self.db_name = db_name

    def __enter__(self):
        self.init()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.finalize()

    def init(self: any) -> bool:
        """
        create db and tables
        """
        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        return True

    def check_db(self: any) -> bool:
        """
        check db exist
        """
        self.conn, self.cur = DBManager.check_connect_db(self.result_dir, self.db_name)
        if not (self.conn and self.cur):
            return False
        return True

    def check_table(self: any) -> bool:
        """
        check db exist
        """
        self.conn, self.cur = DBManager.check_connect_db(self.result_dir, self.db_name)
        if not (self.conn and self.cur):
            logging.warning("Failed to connect database, please check database of %s. ", self.db_name.split(".")[0])
            return False
        for table_name in self.table_list:
            if not DBManager.judge_table_exist(self.cur, table_name):
                logging.warning("Table %s not found.", table_name)
                return False
        return True

    def finalize(self: any) -> None:
        """
        release conn and cur
        """
        DBManager.destroy_db_connect(self.conn, self.cur)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def insert_data_to_db(self: any, table_name: str, data_list: list) -> None:
        """
        insert data into the table.
        :param table_name: table name
        :param data_list: data_list
        :return: None
        """
        if self.conn and data_list:
            sql = 'insert into {0} values ({1})'.format(table_name, "?," * (len(data_list[0]) - 1) + "?")
            if not DBManager.executemany_sql(self.conn, sql, data_list):
                logging.warning(
                    'insert data to table %s failed, please check.', table_name, exc_info=Constant.TRACE_BACK_SWITCH
                )

    def drop_table(self: any, table_name: str) -> None:
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)

    def get_all_data(self: any, table_name: str, dto_class: any = None) -> list:
        """
        get all data from db
        :param table_name:
        :param dto_class:
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        all_data_sql = "select * from {}".format(table_name)
        return DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=dto_class)
