#!/usr/bin/python3
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
from msmodel.interface.view_model import ViewModel


class StreamExpandSpecModel(ParserModel):
    """
    Model for Node Attr Info Parser
    """

    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_STREAM_EXPAND_SPEC, [DBNameConstant.TABLE_STREAM_EXPAND_SPEC])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_STREAM_EXPAND_SPEC) -> None:
        """
        insert data to table
        :param data_list: stream expand spec data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)


class StreamExpandSpecViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_STREAM_EXPAND_SPEC, [])

    def get_stream_expand_spec_data(self) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STREAM_EXPAND_SPEC):
            return []
        sql = "SELECT expand_status FROM {}".format(DBNameConstant.TABLE_STREAM_EXPAND_SPEC)
        return DBManager.fetch_all_data(self.cur, sql)
