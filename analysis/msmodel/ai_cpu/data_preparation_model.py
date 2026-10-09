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
from msmodel.interface.parser_model import ParserModel


class DataPreparationModel(ParserModel):
    """
    data preparation model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS, table_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            sql = DBManager.sql_create_general_table('{}Map'.format(table_name), table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def flush_all(self: any, data_dict: dict) -> None:
        """
        insert all data preparation data to table
        :param data_dict: collected data preparation data
        :return:
        """
        for table_name in data_dict.keys():
            self.flush(data_dict.get(table_name, []), table_name)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_DATA_QUEUE) -> None:
        """
        insert data to table
        :param data_list: collected data preparation data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)
