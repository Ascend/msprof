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
from abc import abstractmethod

import math

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager


class HostProfDataBase:
    """
    host prof data analysis base model
    """

    TABLES_PATH = ConfigManager.TABLES
    TABLE_CACHE_SIZE = 10000

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        self.result_dir = result_dir
        self.conn = None
        self.cur = None
        self.table_list = table_list
        self.db_name = db_name
        self.cache_data = []

    def __enter__(self):
        self.init()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.finalize()

    @staticmethod
    def recommend_value_for_data_list(data: list) -> float:
        """
        find 98th percent value for data list( >0)
        :param data: a series data ordered asc
        :return: recommend value
        """
        data = [datum[0] for datum in data if datum[0] > 0]
        return Utils.percentile(data, Constant.RECOMMEND_PERCENTILE, math.ceil) / Constant.RATIO_FOR_BEST_PERFORMANCE

    def init(self: any) -> bool:
        """
        init db
        :return: init result
        """

        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.create_table()
        return True

    def check_db(self: any) -> bool:
        """
        check db exist
        :return: result of checking db
        """
        self.conn, self.cur = DBManager.check_connect_db(self.result_dir, self.db_name)
        if not (self.conn and self.cur):
            return False
        return True

    def finalize(self: any) -> None:
        """
        release conn and cur
        :return: None
        """
        DBManager.destroy_db_connect(self.conn, self.cur)

    def create_table(self: any) -> None:
        """
        create table
        :return: None
        """
        fmt_table_map = "{0}Map"
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            table_map = fmt_table_map.format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def insert_single_data(self: any, data: list) -> None:
        """
        insert data
        :param data: data
        :return: None
        """
        self.cache_data.append(data)
        if len(self.cache_data) > self.TABLE_CACHE_SIZE:
            try:
                self.flush_data()
            except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
                logging.exception(error, exc_info=Constant.TRACE_BACK_SWITCH)
            finally:
                self.cache_data.clear()

    def get_recommend_value(self: any, value_name: str, table_name: str) -> list:
        """
        param: value's key, table name
        get recommend value
        :return: [peak value, recommend value]
        """
        value_info_sql = f"SELECT {value_name} from {table_name} order by {value_name} ASC"
        value_list = DBManager.fetch_all_data(self.cur, value_info_sql)
        if not value_list:
            return [Constant.NA, Constant.NA]
        peak_value = value_list[-1][0]
        recommend_value = self.recommend_value_for_data_list(value_list)
        return [peak_value, recommend_value]

    @abstractmethod
    def flush_data(self: any) -> any:
        """
        flush all data
        """
