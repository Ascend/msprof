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
from abc import ABC
from functools import reduce
from operator import add

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class TscpuModel(BaseModel, ABC):
    """
    acsq task model class
    """

    DEFAULT_NIC_FUNC_ID = 0
    ROUND_NUMBER = 3
    PERCENTAGE = 100
    BYTE = 8
    NETWORK_HEADER_TAG = 'rxPacket/s'
    TABLES_PATH = ConfigManager.TABLES_TRAINING
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.device_id = InfoConfReader().get_device_list()[0] if InfoConfReader().get_device_list() else '0'
        self.conn = None
        self.cur = None

    def init(self: any) -> bool:
        """
        create db and tables
        """
        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.cur.execute("PRAGMA page_size=8192")
        self.cur.execute("PRAGMA journal_mode=WAL")
        self.create_table()
        return True

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_TSCPU_ORIGIN, data_list)
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_TS_CPU_EVENT):
            self._create_ts_event_count_table()
        self._insert_ts_event_count()
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_TS_CPU_HOT_INS):
            self._create_ts_hot_ins_table()
        self._insert_ts_hot_ins()

    def get_pmu_event_name(self: any) -> any:
        """
        get pmu event name
        :return: pmu events
        """
        event_sql = 'select distinct(event) from TsOriginalData order by rowid;'
        pmu_events = DBManager.fetch_all_data(self.cur, event_sql)
        if pmu_events:
            pmu_events = reduce(add, pmu_events)
        return pmu_events

    def _create_ts_event_count_table(self: any, table_name: str = DBNameConstant.TABLE_TS_CPU_EVENT) -> None:
        """
        create ts event count table
        :param table_name: table name
        :return: None
        """
        pmu_events = self.get_pmu_event_name()
        sql = (
            "CREATE TABLE IF NOT EXISTS  " + table_name + "(func text,callstack text,"
            "module text,common text,pid INT,"
            "tid INT,core INT," + ",".join(pmu_event.replace('0x', 'r') + " INT" for pmu_event in pmu_events) + ")"
        )
        DBManager.execute_sql(self.conn, sql)

    def _insert_ts_event_count(self: any) -> None:
        """
        insert data into event count table
        :return: None
        """
        pmu_events = self.get_pmu_event_name()
        insert_statement = (
            "INSERT INTO EventCount SELECT function,"
            "callstack,'/var/tsch_fw' as module,"
            "'/var/tsch_fw' as common,-1 as pid,-1 as tid, 1 as core,"
        )
        group_statement = " FROM TsOriginalData GROUP BY function,callstack;"
        case_list = []
        for pmu_event in pmu_events:
            case_list.append(
                "SUM(CASE WHEN event is '{0}' THEN count ELSE 0 END) as {1}".format(
                    pmu_event, pmu_event.replace('0x', 'r')
                )
            )
        sql = insert_statement + ",".join(case_list) + group_statement
        DBManager.execute_sql(self.conn, sql)

    def _create_ts_hot_ins_table(self: any, table_name: str = DBNameConstant.TABLE_TS_CPU_HOT_INS) -> None:
        """
        create hot_ins table
        :param table_name: table name
        :return: None
        """
        pmu_events = self.get_pmu_event_name()
        sql = (
            "CREATE TABLE IF NOT EXISTS  "
            + table_name
            + "(ip text,function text,module text,pid INT,tid INT,core INT,"
            + ",".join(pmu_event.replace('0x', 'r') + " INT" for pmu_event in pmu_events)
            + ")"
        )
        DBManager.execute_sql(self.conn, sql)

    def _insert_ts_hot_ins(self: any) -> None:
        """insert data into hot_ins table"""
        pmu_events = self.get_pmu_event_name()
        insert_statement = (
            "INSERT INTO HotIns SELECT pc,function,'/var/tsch_fw' as module,-1 as pid,-1 as tid, 1 as core,"
        )
        group_statement = " FROM TsOriginalData GROUP BY pc,function;"
        case_list = []
        for pmu_event in pmu_events:
            case_list.append(
                "SUM(CASE WHEN event is '{0}' THEN count ELSE 0 END) as {1}".format(
                    pmu_event, pmu_event.replace('0x', 'r')
                )
            )
        sql = insert_statement + ",".join(case_list) + group_statement
        DBManager.execute_sql(self.conn, sql)
