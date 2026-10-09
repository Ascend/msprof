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

import sqlite3

from common_func.common import byte_per_us2_mb_pers, ns2_us
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager


class InterConnectionView:
    """
    view for inter connection
    """

    def __init__(self: any, result_dir: str, sample_config: dict) -> None:
        self.result_dir = result_dir
        self.sample_config = sample_config

    @staticmethod
    def get_domains_element(result_data: any, index: int) -> any:
        """
        get every domain element
        :param result_data: result data
        :param index: index
        :return: value of index
        """
        min_default_val = int('0xFFFFF', Constant.HEX_NUMBER)
        return result_data[index] if result_data[index] != min_default_val else 0

    @staticmethod
    def _get_hccs_sql_str() -> str:
        sql = (
            "SELECT round(MAX(txThroughput),{accuracy}), "
            "round(MIN(txThroughput), {accuracy}), round(AVG(txThroughput), {accuracy}), "
            "round(MAX(rxThroughput),{accuracy}), round(MIN(rxThroughput), {accuracy}), "
            "round(AVG(rxThroughput), {accuracy}) FROM {0} "
            "WHERE device_id IS ?".format(DBNameConstant.TABLE_HCCS_EVENTS, accuracy=NumberConstant.ROUND_THREE_DECIMAL)
        )
        return sql

    @staticmethod
    def _get_pcie_sql_str() -> str:
        if ChipManager().is_chip_v4():
            sql = (
                "select timestamp, device_id, tx_p_bandwidth_min,"
                "tx_p_bandwidth_max, tx_p_bandwidth_avg, tx_np_bandwidth_min,"
                "tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min,"
                "tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg, tx_np_latency_min,"
                "tx_np_latency_max, tx_np_latency_avg, rx_p_bandwidth_min,"
                "rx_p_bandwidth_max, rx_p_bandwidth_avg, rx_np_bandwidth_min,"
                "rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min,"
                "rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg from PcieOriginalData order by timestamp desc limit 1"
            )
        else:
            sql = (
                "select timestamp, device_id, round(AVG(tx_p_bandwidth_min), {accuracy}),"
                "round(AVG(tx_p_bandwidth_max), {accuracy}),round(AVG(tx_p_bandwidth_avg),"
                " {accuracy}),round(AVG(tx_np_bandwidth_min), {accuracy}),round(AVG("
                "tx_np_bandwidth_max"
                "), {accuracy}),round(AVG(tx_np_bandwidth_avg), {accuracy})"
                ",round(AVG(tx_cpl_bandwidth_min), {accuracy}),round(AVG("
                "tx_cpl_bandwidth_max), {accuracy}),round(AVG(tx_cpl_bandwidth_avg)"
                ", {accuracy}),round(AVG(tx_np_latency_min), {accuracy})"
                ",round(AVG(tx_np_latency_max), {accuracy})"
                ",round(AVG(tx_np_latency_avg), {accuracy})"
                ",round(AVG(rx_p_bandwidth_min), {accuracy}),round(AVG(rx_p_bandwidth_max)"
                ", {accuracy}),round(AVG(rx_p_bandwidth_avg), {accuracy}),"
                "round(AVG(rx_np_bandwidth_min), {accuracy})"
                ",round(AVG(rx_np_bandwidth_max), {accuracy}),"
                "round(AVG(rx_np_bandwidth_avg), {accuracy})"
                ",round(AVG(rx_cpl_bandwidth_min), {accuracy}),"
                "round(AVG(rx_cpl_bandwidth_max), {accuracy})"
                ",round(AVG(rx_cpl_bandwidth_avg), {accuracy}) from PcieOriginalData where "
                "tx_p_bandwidth_max >= tx_p_bandwidth_min".format(accuracy=NumberConstant.DECIMAL_ACCURACY)
            )
        return sql

    @staticmethod
    def _check_pcie_valid(curs):
        try:
            result_data = curs.execute(
                f"select * from {DBNameConstant.TABLE_PCIE} order by timestamp desc limit 1"
            ).fetchone()
        except sqlite3.Error:
            return False
        # check whether the result_data has at least 23 domains.
        if not (result_data and len(result_data) >= 23):
            return False
        return True

    def get_hccs_data(self: any, dev_id: str) -> tuple:
        """
        get hccs data
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HCCS)
        conn, curs = DBManager.check_connect_db_path(db_path)
        sql = InterConnectionView._get_hccs_sql_str()
        try:
            hccs_throughput = curs.execute(sql, (dev_id,)).fetchone()
        except sqlite3.Error:
            return MsvpConstant.MSVP_EMPTY_DATA
        finally:
            DBManager.destroy_db_connect(conn, curs)
        if not hccs_throughput:
            return MsvpConstant.MSVP_EMPTY_DATA
        for item in hccs_throughput:
            if item is None:
                return MsvpConstant.MSVP_EMPTY_DATA
        _result = [["Tx (MB/s)"] + list(hccs_throughput[0:3]), ["Rx (MB/s)"] + list(hccs_throughput[3:])]
        headers = ["Mode", "Max", "Min", "Average"]
        return headers, _result, len(_result)

    def get_pcie_summary_data(self: any) -> tuple:
        """
        get pcie data
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_PCIE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not (conn and curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_PCIE):
            return MsvpConstant.MSVP_EMPTY_DATA
        if not self._check_pcie_valid(curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        try:
            result_data = curs.execute(InterConnectionView._get_pcie_sql_str()).fetchone()
        except sqlite3.Error:
            return MsvpConstant.MSVP_EMPTY_DATA
        finally:
            DBManager.destroy_db_connect(conn, curs)
        table_data = [
            (
                "Tx_p_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 2)),
                byte_per_us2_mb_pers(result_data[3]),
                byte_per_us2_mb_pers(result_data[4]),
            ),
            (
                "Tx_np_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 5)),
                byte_per_us2_mb_pers(result_data[6]),
                byte_per_us2_mb_pers(result_data[7]),
            ),
            (
                "Tx_cpl_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 8)),
                byte_per_us2_mb_pers(result_data[9]),
                byte_per_us2_mb_pers(result_data[10]),
            ),
            (
                "Tx_latency_avg(us)",
                ns2_us(self.get_domains_element(result_data, 11)),
                ns2_us(result_data[12]),
                ns2_us(result_data[13]),
            ),
            (
                "Rx_p_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 14)),
                byte_per_us2_mb_pers(result_data[15]),
                byte_per_us2_mb_pers(result_data[16]),
            ),
            (
                "Rx_np_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 17)),
                byte_per_us2_mb_pers(result_data[18]),
                byte_per_us2_mb_pers(result_data[19]),
            ),
            (
                "Rx_cpl_avg(MB/s)",
                byte_per_us2_mb_pers(self.get_domains_element(result_data, 20)),
                byte_per_us2_mb_pers(result_data[21]),
                byte_per_us2_mb_pers(result_data[22]),
            ),
        ]
        table_header = ["Mode", "Min", "Max", "Avg"]
        return table_header, table_data, len(table_data)
