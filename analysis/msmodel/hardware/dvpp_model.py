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
from abc import ABC

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class DvppModel(BaseModel, ABC):
    """
    acsq task model class
    """

    TIME_RATE = 1000000.0
    ROUND_NUMBER = 3
    PERCENTAGE = 100

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.aic_profiling_events = None  # ai core pmu event
        self.aiv_profiling_events = None

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_DVPP_ORIGIN, data_list)
        self._create_index()

    def report_data(self: any, has_dvpp_id: bool) -> None:
        """
        summary data of dvpp
        :param has_dvpp_id: dvpp id
        :return:
        """
        try:
            if DBManager.judge_table_exist(self.cur, 'DvppOriginalData'):
                self._create_dvpp_report_data(has_dvpp_id)
                if not has_dvpp_id:
                    self._create_dvpp_tree_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err))

    def _create_index(self: any) -> None:
        """
        create index
        :return: None
        """
        if DBManager.judge_table_exist(self.cur, "DvppOriginalData"):
            if not DBManager.judge_index_exist(self.cur, "timestamp_dvpp"):
                self.cur.execute("CREATE INDEX timestamp_dvpp ON DvppOriginalData(timestamp)")
            if not DBManager.judge_index_exist(self.cur, "engineid_dvpp"):
                self.cur.execute("CREATE INDEX engineid_dvpp ON DvppOriginalData(engineid)")
            if not DBManager.judge_index_exist(self.cur, "enginetype_dvpp"):
                self.cur.execute("CREATE INDEX enginetype_dvpp ON DvppOriginalData(enginetype)")

    def _get_dvpp_tree_data(self: any, total_device_id: list, enginetype: list) -> list:
        result_data = []
        for devid in total_device_id:
            for etype in enginetype:
                sql = "select distinct(engineid) from DvppOriginalData where device_id = ? and enginetype = ?"
                engine_id = DBManager.fetch_all_data(self.cur, sql, (devid[0], etype[0]))
                for eid in engine_id:
                    result_data.append((devid[0], etype[0], eid[0]))
        return result_data

    def _create_dvpp_tree_data(self: any) -> None:
        """
        create dvpp tree data table and insert data
        :return: None
        """
        if not DBManager.judge_table_exist(self.cur, 'DvppTreeData'):
            create_sql = "create table DvppTreeData (device_id Int, engineType Int, engineId Int)"
            DBManager.execute_sql(self.conn, create_sql)

        total_device_id_sql = "SELECT DISTINCT(device_id) FROM DvppOriginalData WHERE replayid IS 0;"
        total_device_id = DBManager.fetch_all_data(self.cur, total_device_id_sql)

        enginetype_sql = "SELECT DISTINCT(enginetype) FROM DvppOriginalData WHERE replayid IS 0;"
        enginetype = DBManager.fetch_all_data(self.cur, enginetype_sql)
        result_data = self._get_dvpp_tree_data(total_device_id, enginetype)

        insert_sql = "insert into DvppTreeData values (?,?,?)"
        DBManager.executemany_sql(self.conn, insert_sql, result_data)

    def _get_dvpp_report_data(self: any, has_dvpp_id: bool) -> list:
        target_data = []
        select_device_sql = "select distinct(device_id) from DvppOriginalData where replayId = 0"
        device_ids = DBManager.fetch_all_data(self.cur, select_device_sql)

        select_dvpp_sql = "select distinct(dvppId) from DvppOriginalData where replayId = 0 and device_id = ?"
        for device_id in device_ids:
            if has_dvpp_id:
                dvpp_ids = DBManager.fetch_all_data(self.cur, select_dvpp_sql, (device_id[0],))
            else:
                dvpp_ids = [(0,)]
            for dvpp_id in dvpp_ids:
                self.__get_dvpp_data_by_dvpp_id(device_id, dvpp_id, target_data)
        return target_data

    def _create_dvpp_report_data(self: any, has_dvpp_id: bool) -> None:
        """
        create dvpp summary data
        :return: None
        """
        if not DBManager.judge_table_exist(self.cur, 'DvppReportData'):
            sql = DBManager.sql_create_general_table('DvppReportDataMap', 'DvppReportData', self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)
        target_data = self._get_dvpp_report_data(has_dvpp_id)
        # target data has a length of 7, so I use "?," * 6 + "?" below
        insert_sql = "insert into DvppReportData values({})".format("?," * 6 + "?")
        DBManager.executemany_sql(self.conn, insert_sql, target_data)

    def __get_dvpp_data_by_dvpp_id(self: any, device_id: int, dvpp_id: int, target_data: list) -> None:
        """
        get dvpp_data by dvpp_id
        :return: None
        """
        type_sql = (
            "select distinct(engineType) from DvppOriginalData where replayId = 0 and device_id = ? and dvppId = ?"
        )
        engine_types = DBManager.fetch_all_data(self.cur, type_sql, (device_id[0], dvpp_id[0]))
        for engine_type in engine_types:
            ids_sql = (
                "select distinct(engineId) from DvppOriginalData where engineType = ? and device_id = ? and dvppId = ?"
            )
            engine_ids = DBManager.fetch_all_data(self.cur, ids_sql, (engine_type[0], device_id[0], dvpp_id[0]))
            for engine_id in engine_ids:
                all_time_sql = (
                    'select max(allTime)-min(allTime) from DvppOriginalData '
                    'where engineType = ? and engineId = ? and device_id = ? and '
                    'dvppId = ? order by rowid;'
                )
                all_time = DBManager.fetch_all_data(
                    self.cur, all_time_sql, (engine_type[0], engine_id[0], device_id[0], dvpp_id[0])
                )[0][0]

                all_frame_sql = (
                    'select max(allFrame)-min(allFrame) from '
                    'DvppOriginalData where engineType = ? and '
                    'engineId = ? and device_id = ?  and dvppId = ? order by rowid;'
                )
                all_frame = DBManager.fetch_all_data(
                    self.cur, all_frame_sql, (engine_type[0], engine_id[0], device_id[0], dvpp_id[0])
                )[0][0]

                utilization_sql = (
                    'select allutilization from '
                    'DvppOriginalData where engineType = ? and '
                    'engineId = ? and device_id = ?  and dvppId = ? order by rowid desc limit 1;'
                )
                utilization = DBManager.fetchone(
                    self.cur, utilization_sql, (engine_type[0], engine_id[0], device_id[0], dvpp_id[0])
                )

                all_utilization = str(utilization[0]).strip('%') if utilization else '0'
                target_data.append(
                    (dvpp_id[0], device_id[0], engine_type[0], engine_id[0], all_time, all_frame, all_utilization)
                )
