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

import os
import logging

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class CommunicationAnalyzerModel(BaseModel):
    """
    export analyzer result of communication to db
    """

    def __init__(self, export_path, export_tables):
        super().__init__(export_path, DBNameConstant.DB_COMMUNICATION_ANALYZER, export_tables)

        if DBNameConstant.TABLE_COMM_ANALYZER_MATRIX in export_tables:
            self.analyzer_type = "matrix"
        else:
            self.analyzer_type = "communication"
        self.conn = None
        self.cur = None

    @staticmethod
    def get_time_info(op_name, raw_time_info):
        time_info = op_name.split('@')
        for key, value in raw_time_info.items():
            if "Ratio" not in key:
                time_info.append(value)
        return time_info

    @staticmethod
    def get_band_info(op_name, raw_band_info):
        band_info_list = []
        for band_name, band_value in raw_band_info.items():
            transit_data = list(band_value.values())
            package_data = transit_data.pop()
            if sum(transit_data) == 0:
                continue
            if len(package_data) == 0:
                band_info = op_name.split('@')
                band_info.append(band_name)
                band_info.extend(transit_data)
                band_info.extend([0, 0, 0])
                band_info_list.append(band_info)
            elif len(package_data) >= 1:
                for key, value in package_data.items():
                    band_info = op_name.split('@')
                    band_info.append(band_name)
                    band_info.extend(transit_data)
                    band_info.append(key)
                    band_info.extend(value)
                    band_info_list.append(band_info)
        return band_info_list

    def init(self: any) -> bool:
        """
        create db and tables
        """
        self.conn, self.cur = DBManager.create_connect_db(os.path.join(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.create_table()
        return True

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

    def flush_communication_data_to_db(self, communication_info: dict) -> list:
        """
        :return:
        """
        if "Total HCCL Operators" in communication_info.keys():
            communication_info.pop("Total HCCL Operators")
        if self.analyzer_type == "communication":
            time_data, band_data = self.get_op_data(communication_info)
            self.insert_data_to_db(DBNameConstant.TABLE_COMM_ANALYZER_TIME, time_data)
            self.insert_data_to_db(DBNameConstant.TABLE_COMM_ANALYZER_BAND, band_data)
        elif self.analyzer_type == "matrix":
            matrix_data = self.get_matrix_data(communication_info)
            self.insert_data_to_db(DBNameConstant.TABLE_COMM_ANALYZER_MATRIX, matrix_data)

    def get_op_data(self, raw_info: dict):
        time_data = []
        band_data = []
        if raw_info is None:
            logging.warning("The communication info is empty.")
            return time_data, band_data
        for op_name, comm_info in raw_info.items():
            if '@' not in op_name:
                logging.error("invalid communication info format.")
                continue
            if 'Communication Time Info' in comm_info.keys():
                time_data.append(self.get_time_info(op_name, comm_info['Communication Time Info']))
            if 'Communication Bandwidth Info' in comm_info.keys():
                band_data.extend(self.get_band_info(op_name, comm_info['Communication Bandwidth Info']))
        return time_data, band_data

    def get_matrix_data(self, raw_info: dict):
        matrix_data = []
        if raw_info is None:
            logging.warning("The communication info is empty.")
            return matrix_data
        for op_name, comm_info in raw_info.items():
            if '@' in op_name:
                op_name, group_name = op_name.split('@')
            else:
                logging.error("invalid communication info format.")
                continue
            for connection, band_value in comm_info.items():
                matrix_info = [op_name, group_name]
                if '-' in connection:
                    matrix_info.extend(connection.split('-'))
                else:
                    logging.error("invalid communication matrix info format.")
                matrix_info.extend(list(band_value.values()))
                matrix_data.append(matrix_info)
        return matrix_data
