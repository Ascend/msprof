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
from msmodel.interface.base_model import BaseModel


class MemcpyModel(BaseModel):
    """
    acsq memory copy model class
    """

    NAME = "name"
    RECEIVE_TIME = "receive_time"
    START_TIME = "start_time"
    END_TIME = "end_time"
    DURATION = "duration"
    STREAM_ID = "stream_id"
    TASK_ID = "task_id"
    TYPE = "type"

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        self.result_dir = result_dir
        self.conn = None
        self.cur = None
        self.table_list = table_list
        self.db_name = db_name

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

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush memcpy data to db
        :param table_name:table_name
        :param data_list:memcpy data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)

    def return_chip0_summary(self: any, table_name: str) -> list:
        """
        export chip 0 summary data from TsMemcpyCalculation
        :param table_name: TsMemcpyCalculation
        :return: list
        """
        columns_name = [
            "sum(%s)" % self.DURATION,
            self.TYPE,
            self.TASK_ID,
            self.STREAM_ID,
            "sum(%s)" % self.START_TIME + " - " + self.RECEIVE_TIME,
            "avg(%s)" % self.DURATION,
            "min(%s)" % self.DURATION,
            "max(%s)" % self.DURATION,
            "count(*)",
        ]
        group_column = [self.STREAM_ID, self.TASK_ID]

        if DBManager.judge_table_exist(self.cur, table_name):
            sql = "select {0} from {1} group by {2}".format(",".join(columns_name), table_name, ",".join(group_column))
            export_data = DBManager.fetch_all_data(self.cur, sql)
        else:
            export_data = []

        return export_data

    def return_not_chip0_summary(self: any, table_name: str) -> list:
        """
        export non chip 0 summary data from TsMemcpyCalculation
        :param table_name: TsMemcpyCalculation
        :return: list
        """
        columns_name = [
            self.NAME,
            self.TYPE,
            self.STREAM_ID,
            self.TASK_ID,
            self.DURATION,
            self.START_TIME,
            self.END_TIME,
        ]
        return self._get_export_data(columns_name, table_name)

    def return_task_scheduler_timeline(self: any, table_name: str) -> list:
        """
        export task scheduler timeline from TsMemcpyCalculation
        :param table_name: TsMemcpyCalculation
        :return: list
        """
        columns_name = [
            self.NAME,
            self.TYPE,
            self.RECEIVE_TIME,
            self.START_TIME,
            self.END_TIME,
            self.DURATION,
            self.STREAM_ID,
            self.TASK_ID,
        ]
        return self._get_export_data(columns_name, table_name)

    def _get_export_data(self: any, columns_name: list, table_name: str) -> list:
        if DBManager.judge_table_exist(self.cur, table_name):
            sql = "select {0} from {1}".format(",".join(columns_name), table_name)
            export_data = DBManager.fetch_all_data(self.cur, sql)
        else:
            export_data = []
        return export_data
