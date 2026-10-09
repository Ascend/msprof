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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.singleton import singleton


@singleton
class GeLogicStreamSingleton:
    def __init__(self: any) -> None:
        self.use_flag = True
        self.max_length = 0
        self.stream_id_mapping = []
        self.project_path = ""

    def load_info(self, project_path):
        self.clear()
        self.project_path = project_path
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_LOGIC_STREAM_INFO)
        if not DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_LOGIC_STREAM_INFO):
            self.use_flag = False
            return
        ge_stream_result = self.get_ge_logic_stream_data()
        stream_mapping_list = ge_stream_result[0]
        self.max_length = ge_stream_result[1][0]
        if self.max_length > NumberConstant.DEFAULT_STREAM_ID:
            self.use_flag = False
            logging.error("The physical stream_id %d reported exceeds the maximum value.", self.max_length)
            return
        self.stream_id_mapping = [None] * (self.max_length + 1)
        for mspping in stream_mapping_list:
            self.add_stream_id_mapping(mspping[0], mspping[1])

    def add_stream_id_mapping(self, physic_stream, logic_stream):
        if physic_stream <= self.max_length and self.stream_id_mapping[physic_stream] is None:
            self.stream_id_mapping[physic_stream] = logic_stream

    def get_ge_logic_stream_data(self: any) -> list:
        ge_stream_result = []
        ge_conn, ge_curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_GE_LOGIC_STREAM_INFO)
        if not (ge_conn and ge_curs):
            DBManager.destroy_db_connect(ge_conn, ge_curs)
            # logic_stream 没上报，不做合并
            self.use_flag = False
            return ge_stream_result

        sql = "SELECT distinct physic_stream, logic_stream FROM {}".format(DBNameConstant.TABLE_GE_LOGIC_STREAM_INFO)
        ge_stream_result = DBManager.fetch_all_data(ge_curs, sql)
        sql = "SELECT MAX(physic_stream) FROM {}".format(DBNameConstant.TABLE_GE_LOGIC_STREAM_INFO)
        max_physic_stream = DBManager.fetchone(ge_curs, sql)
        DBManager.destroy_db_connect(ge_conn, ge_curs)
        return [ge_stream_result, max_physic_stream]

    def get_logic_stream_id(self, physic_stream):
        if self.use_flag is False or self.max_length == 0:
            return physic_stream
        if physic_stream > self.max_length or self.stream_id_mapping[physic_stream] is None:
            return physic_stream
        return self.stream_id_mapping[physic_stream]

    def clear(self: any) -> None:
        self.use_flag = True
        self.max_length = 0
        self.stream_id_mapping = []
        self.project_path = ""
