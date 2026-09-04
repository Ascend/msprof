# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging
from abc import ABC
from collections import Counter

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class HbmModel(BaseModel, ABC):
    """
    acsq task model class
    """

    SCALE = 0.000030517578125  # equal to HBMC(256) / HBM_EVENT(8) / KILOBYTE(1024.0) / KILOBYTE(1024.0)

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []

    def create_table(self: any) -> None:
        """
        create Hbm table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                self.drop_tab()
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, ConfigManager.TABLES_TRAINING)
            DBManager.execute_sql(self.conn, sql)

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_HBM_ORIGIN, data_list)

    def drop_tab(self: any) -> None:
        """
        drop exists table
        :return: None
        """
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS HBMOriginalData')
        DBManager.execute_sql(self.conn, 'DROP TABLE IF EXISTS HBMbwData')

    def insert_bw_data(self: any, event_type: list) -> None:
        """
        insert HBM bandwidth data
        :param event_type: event type
        :return: None
        """
        device_id_sql = "select distinct(device_id) from HBMOriginalData where replayId=0;"
        device_list = DBManager.fetch_all_data(self.cur, device_id_sql)
        try:
            for device in device_list:
                if len(event_type) not in (1, 2):  # hbmProfilingEvents are read and/or write
                    logging.error("insert_bw_data failed, event_type(%s) is invalid.", str(event_type))
                    raise ProfException(ProfException.PROF_SYSTEM_EXIT)
                sql = (
                    'select device_id,timestamp,counts,hbmId,event_type from HBMOriginalData '
                    'where device_id=? group by timestamp,event_type,hbmId,counts order by timestamp'
                )
                bw_data = DBManager.fetch_all_data(self.cur, sql, (device[0],))
                if not bw_data:
                    continue
                bw_data, check_len = self._filter_abnormal_timestamps(bw_data, device[0])
                if check_len <= 0 or not bw_data:
                    continue
                data = self._get_hbm_data(bw_data, check_len)
                insert_sql = 'INSERT INTO HBMbwData values (?,?,?,?,?)'
                DBManager.executemany_sql(self.conn, insert_sql, data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            logging.error('Failed to insert HBM bandwidth data.')
        finally:
            pass

    @staticmethod
    def _filter_abnormal_timestamps(bw_data: list, device_id: any) -> tuple:
        """
        统计每个 timestamp 的数据条数，以众数（出现次数最多的条数）为基准，
        去除条数与众数不一致的 timestamp 数据，避免丢数据导致错位。
        :param bw_data: 按 timestamp,hbmId,event_type 排序后的原始数据
        :param device_id: 设备 id
        :return: (过滤后的数据, 每个 timestamp 的条数众数)
        """
        ts_counts = Counter(row[1] for row in bw_data)
        count_freq = Counter(ts_counts.values())
        max_freq = max(count_freq.values())
        # 众数平局时取较大的条数，避免丢弃更多有效数据
        check_len = max(cnt for cnt, freq in count_freq.items() if freq == max_freq)
        filtered = [row for row in bw_data if ts_counts[row[1]] == check_len]
        if len(filtered) != len(bw_data):
            logging.warning(
                "HBM data loss: device_id=%s total=%s check_len=%s removed=%s.",
                device_id,
                len(bw_data),
                check_len,
                len(bw_data) - len(filtered),
            )
        return filtered, check_len

    def _get_hbm_data(self: any, bw_data: list, check_len: int) -> list:
        data = []
        for i, _ in enumerate(bw_data):
            if bw_data[i][1] - bw_data[i - check_len][1]:
                dur_time = bw_data[i][1] - bw_data[i - check_len][1]
                tmp_counts = bw_data[i][2] * self.SCALE / dur_time * Constant.TIME_RATE
                sys_counts = max((tmp_counts, 0))
                item = (bw_data[i][0], bw_data[i][1], sys_counts, bw_data[i][3], bw_data[i][4])
            else:
                item = (bw_data[i][0], bw_data[i][1], bw_data[i][2], bw_data[i][3], bw_data[i][4])
            data.append(item)
        return data
