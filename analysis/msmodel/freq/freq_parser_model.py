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

import logging

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.interface.parser_model import ParserModel


class FreqParserModel(ParserModel):
    """
    db operator for frequency parser
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_FREQ, table_list)

    @staticmethod
    def get_freq_data(project_path: str) -> list:
        """
        get frequency data
        """
        freq_db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_FREQ)
        freq_conn, freq_curs = DBManager.check_connect_db_path(freq_db_path)
        if not (freq_conn and freq_curs) or not DBManager.judge_table_exist(freq_curs, DBNameConstant.TABLE_FREQ_PARSE):
            DBManager.destroy_db_connect(freq_conn, freq_curs)
            return []
        sql = "select syscnt, freq from {}".format(DBNameConstant.TABLE_FREQ_PARSE)
        freq_data = DBManager.fetch_all_data(freq_curs, sql)
        DBManager.destroy_db_connect(freq_conn, freq_curs)
        if not isinstance(freq_data, list) or not freq_data or len(freq_data[0]) != 2:
            logging.error("The freq data format is error!")
            return []
        freq_data.sort(key=lambda item: item[0])
        return freq_data

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        if self.table_list:
            self.insert_data_to_db(self.table_list[0], data_list)
