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
import os
from abc import ABC

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.base_model import BaseModel


class DdrModel(BaseModel, ABC):
    """
    acsq task model class
    """

    FILE_NAME = os.path.basename(__file__)
    DDR_EVENT = 8
    TIME_RATE = 1000000.0
    ROUND_NUMBER = 3
    PERCENTAGE = 100

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.aic_profiling_events = None  # ai core pmu event
        self.aiv_profiling_events = None

    @classmethod
    def calculate_data(cls: any, ddr_data: any, start_time: any) -> list:
        """
        calculate time series data of ddr metrics
        :param ddr_data: ddr data
        :param start_time: start time
        :return:
        """
        metric_data = []
        ddr_total_first = []
        ddr_first = ddr_data[0]
        if len(ddr_first) >= 4:  # 4 is the minimum length for ddr
            ddr_total_first = cls._calculate_ddr_head_data(ddr_first, start_time)
        if len(ddr_data) >= 1:  # add ddr data from second place
            ddr_data_cal = []
            for index, ddr in enumerate(ddr_data[1:]):
                ddr_data_cal.append(cls._calculate_ddr_data(ddr, ddr_data[index]))
            metric_data = ddr_total_first + ddr_data_cal
        return metric_data

    @classmethod
    def _calculate_ddr_head_data(cls: any, ddr_first: any, start_time: int) -> list:
        bandwidth_list = []
        for ddr in ddr_first[3:]:
            if ddr_first[2] - start_time:
                bandwidth_list.append(cls._cal_ddr_bandwidth(ddr, ddr_first[2] - start_time))
            else:
                bandwidth_list.append(0)
        return [list(ddr_first[:3]) + bandwidth_list]

    @classmethod
    def _calculate_ddr_data(cls: any, ddr: any, ddr_data: any) -> list:
        if len(ddr) >= 4:
            bandwidth_list = []
            for metric in ddr[3:]:
                if ddr[2] - ddr_data[2]:
                    bandwidth_list.append(cls._cal_ddr_bandwidth(metric, ddr[2] - ddr_data[2]))
                else:
                    bandwidth_list.append(0)
            return list(ddr[:3]) + bandwidth_list
        return []

    @classmethod
    def _cal_ddr_bandwidth(cls: any, metric: any, ddr_delta_time: any) -> any:
        return (
            metric
            * ConfigMgr.get_ddr_bit_width()
            / (ddr_delta_time * NumberConstant.KILOBYTE * NumberConstant.KILOBYTE * cls.DDR_EVENT)
            * Constant.TIME_RATE
        )

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_DDR_ORIGIN, data_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                self.drop_tab()
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def drop_tab(self: any) -> None:
        """
        drop exists table
        :return: None
        """
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS DDROriginalData')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS DDRMetricData')

    def insert_metric_data(self: any, master_tag: int) -> None:
        """
        insert time series data of ddr metrics
        :return:None
        """
        device_list = DBManager.fetch_all_data(
            self.cur, "select distinct(device_id) from DDROriginalData where replayId=0;"
        )
        for device in device_list:
            ddr_events = ["flux_read", "flux_write"]
            if master_tag:
                ddr_events.extend(['fluxid_read', 'fluxid_write'])
                sql = (
                    'select device_id, replayid, timestamp, '
                    'sum(case event when ? then counts else 0 end) as read,'
                    ' sum(case when event = ? then counts else 0 end) as write,'
                    'sum(case when event = ? then counts else 0 end) as id_read, '
                    'sum(case when event = ? then counts else 0 end) as id_WRITE '
                    'from DDROriginalData '
                    'where device_id=? and replayId=0 group by timestamp;'
                )
                ddr_data = DBManager.fetch_all_data(
                    self.cur,
                    sql,
                    (
                        ddr_events[0],
                        ddr_events[1],
                        ddr_events[2],
                        ddr_events[3],
                        device[0],
                    ),
                )
                insert_sql = 'insert into DDRMetricData values(?,?,?,?,?,?,?)'
            else:
                sql = (
                    'select device_id, replayid, timestamp, '
                    'sum(case event when ? then counts else 0 end) as read,'
                    ' sum(case when event = ? then counts else 0 end) '
                    'as write from DDROriginalData '
                    'where device_id=? and replayId=0 group by timestamp;'
                )
                ddr_data = DBManager.fetch_all_data(
                    self.cur,
                    sql,
                    (
                        ddr_events[0],
                        ddr_events[1],
                        device[0],
                    ),
                )
                insert_sql = (
                    'insert into DDRMetricData(device_id, replayid, timestamp, flux_read, flux_write) values(?,?,?,?,?)'
                )
            if ddr_data:
                start_time = InfoConfReader().get_start_timestamp()
                try:
                    metric_data = self.calculate_data(ddr_data, start_time)
                except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
                    logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
                    return
                DBManager.executemany_sql(self.conn, insert_sql, metric_data)
