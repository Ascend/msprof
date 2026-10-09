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
from abc import ABC

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import float_calculate
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class HccsModel(BaseModel, ABC):
    """
    acsq task model class
    """

    TABLES_PATH = ConfigManager.TABLES_TRAINING

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.metric_data = []

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_HCCS_ORIGIN, data_list)

    def drop_tab(self: any) -> None:
        """
        drop exists table
        :return: None
        """
        drop_original_table_sql = 'DROP TABLE IF EXISTS HBMOriginalData'
        drop_bw_table_sql = 'DROP TABLE IF EXISTS HBMbwData'
        DBManager.execute_sql(self.conn, drop_original_table_sql)
        DBManager.execute_sql(self.conn, drop_bw_table_sql)

    def insert_metrics(self: any, device_id: int) -> None:
        """
        Insert metrics value into metrics table
        :return:None
        """
        self._calculate_metrics(device_id)
        try:
            if self.metric_data:
                sql = "INSERT INTO {0} VALUES ({1})".format(
                    DBNameConstant.TABLE_HCCS_EVENTS, '?,' * (len(self.metric_data[0]) - 1) + '?'
                )
                DBManager.executemany_sql(self.conn, sql, self.metric_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            del self.metric_data[:]

    def _insert_metrics_data(self: any, device_id: str, original_data: list) -> None:
        for item in zip(original_data[1:] + original_data[:1], original_data):
            _tx_rate = None
            _rx_rate = None
            _timestamp = item[0][0]
            time_diff = (item[0][0] - item[1][0]) / Constant.BYTE_NS_TO_MB_S

            # 如果存在数据溢出翻转的异常数据，则过滤这条
            if item[0][1] >= item[1][1]:
                _tx_throughout = item[0][1] - item[1][1]
                _tx_rate = float_calculate([_tx_throughout, time_diff], '/')

            if item[0][2] >= item[1][2]:
                _rx_throughput = item[0][2] - item[1][2]
                _rx_rate = float_calculate([_rx_throughput, time_diff], '/')

            if _tx_rate is not None and _rx_rate is not None:
                self.metric_data.append((device_id, _timestamp, _tx_rate, _rx_rate))

    def _calculate_metrics(self: any, device_id: int) -> None:
        """
        Calculate hccs hit rate and throughput.
        :param device_id: device id
        :return: None
        """
        sql = "SELECT timestamp, txAmount, rxAmount FROM {0} ORDER BY timestamp".format(
            DBNameConstant.TABLE_HCCS_ORIGIN
        )
        original_data = DBManager.fetch_all_data(self.cur, sql)
        try:
            if original_data:
                self._insert_metrics_data(device_id, original_data)
                if len(self.metric_data) > 1:
                    del self.metric_data[-1]
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass
