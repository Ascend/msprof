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
import sqlite3
from abc import ABC

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel
from profiling_bean.db_dto.nic_dto import NicDto


class NicModel(BaseModel, ABC):
    """
    acsq task model class
    """

    DEFAULT_NIC_FUNC_ID = 0
    ROUND_NUMBER = 3
    PERCENTAGE = 100
    BYTE = 8
    NETWORK_HEADER_TAG = 'rxPacket/s'

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.device_id = InfoConfReader().get_device_list()[0] if InfoConfReader().get_device_list() else '0'

    @staticmethod
    def _create_rec_db_sql_exec(curs: any, conn: any) -> None:
        sql = (
            "CREATE TABLE IF NOT EXISTS {} (device_id integer, timestamp real, "
            "rx_bandwidth_efficiency real, rx_packets real, "
            "rx_error_rate real, rx_dropped_rate real, "
            "tx_bandwidth_efficiency real, tx_packets real, "
            "tx_error_rate real, tx_dropped_rate real, func_id integer)".format(DBNameConstant.TABLE_NIC_RECEIVE)
        )
        DBManager.execute_sql(conn, sql)

    @staticmethod
    def _cal_nic_info_adapter(nic_info: dict) -> None:
        nic_info["tx_bytes"] = Constant.DEFAULT_COUNT
        nic_info["rx_bytes"] = Constant.DEFAULT_COUNT
        nic_info["rx_packet"] = Constant.DEFAULT_COUNT
        nic_info["rx_packet_second"] = Constant.DEFAULT_COUNT

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_NIC_ORIGIN, data_list)

    def report_data(self: any, device_id_list: list) -> None:
        """
        summary data of nic
        """
        try:
            if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_NIC_ORIGIN):
                self.create_nic_data_report()
                self.create_nic_tree_data()
                self._report_data_per_device(device_id_list)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def create_nic_data_report(self: any) -> None:
        """
        create nic report data table and insert data
        :return: None
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_NIC_REPORT):
            create_sql = DBManager.sql_create_general_table(
                DBNameConstant.TABLE_NIC_REPORT + 'Map', DBNameConstant.TABLE_NIC_REPORT, self.TABLES_PATH
            )
            DBManager.execute_sql(self.conn, create_sql)

        _sql = "select distinct(device_id) from {};".format(DBNameConstant.TABLE_NIC_ORIGIN)
        devices_id = DBManager.fetch_all_data(self.cur, _sql)
        _sql = "select distinct(funcId) from {};".format(DBNameConstant.TABLE_NIC_ORIGIN)
        func_ids = DBManager.fetch_all_data(self.cur, _sql)
        if not devices_id or not func_ids:
            return

        try:
            self._create_nic_data_report_per_device(devices_id, func_ids)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def create_nic_tree_data(self: any) -> None:
        """
        create nic tree data table and insert data
        :return: None
        """
        _table_name = 'NicTreeData'
        _sql = "SELECT DISTINCT(device_id) FROM {} WHERE replayid IS 0;".format(DBNameConstant.TABLE_NIC_ORIGIN)
        nic_device = DBManager.fetch_all_data(self.cur, _sql)
        DBManager.execute_sql(self.conn, "CREATE TABLE IF NOT EXISTS {} (device_id Int)".format(_table_name))
        DBManager.executemany_sql(self.conn, "INSERT INTO {} VALUES (?)".format(_table_name), nic_device)

    def get_func_list(self: any, device: int) -> list:
        """
        Get func lists in original data.
        :param device: device id
        :return:
        """
        _sql = 'SELECT DISTINCT(funcId) FROM {} WHERE device_id IS ?'.format(DBNameConstant.TABLE_NIC_ORIGIN)
        func_list = DBManager.fetch_all_data(self.cur, _sql, (device,))
        return func_list

    def get_nic_report_data(self: any, device: list, func_id: int) -> None:
        """
        get nic report data when data length is 1
        :param device: device id
        :param func_id: function id
        :return:
        """
        target_data = []
        tx_rate_dic = {
            'rx_errors_rate': Constant.DEFAULT_COUNT,
            'rx_drop_rate': Constant.DEFAULT_COUNT,
            'tx_errors_rate': Constant.DEFAULT_COUNT,
            'tx_drop_rate': Constant.DEFAULT_COUNT,
        }
        duration = self._duration_nic_report_sql_exec(device, func_id)
        bandwidth = self._bandwidth_nic_report_sql_exec(device, func_id)
        packet_data = self._packet_data_nic_report_sql_exec(device, func_id)

        rx_data = self._rx_data_nic_report_sql_exec(device, func_id)
        tx_data = self._tx_data_nic_report_sql_exec(device, func_id)
        data_list = [packet_data, duration, bandwidth, rx_data, tx_data]
        if any(map(lambda data: isinstance(data, EmptyClass), data_list)):
            return
        self._rx_rate_adapter(rx_data, tx_rate_dic)
        self._tx_rate_adapter(tx_data, tx_rate_dic)

        target_data.append(
            [
                device[0],
                duration,
                bandwidth,
                Constant.DEFAULT_COUNT,
                Constant.DEFAULT_COUNT,
                packet_data[0],
                tx_rate_dic.get('rx_errors_rate'),
                tx_rate_dic.get('rx_drop_rate'),
                packet_data[1],
                tx_rate_dic.get('tx_errors_rate'),
                tx_rate_dic.get('tx_drop_rate'),
                func_id,
            ]
        )
        DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_NIC_REPORT, target_data)

    def create_nicreceivesend_table(self: any, func_id: int) -> None:
        """
        provides data for get_nic_timeline method
        :param func_id: func id
        :return: None
        """
        nic_obj_list = self.create_nicreceivesend_data(func_id)

        target_data = []
        for nic_obj in nic_obj_list:
            if int(nic_obj.bandwidth):
                rx_eff = round(
                    float(int(nic_obj.rxbyte) * self.BYTE)
                    / (int(nic_obj.bandwidth) * Constant.KILOBYTE * Constant.KILOBYTE),
                    self.ROUND_NUMBER,
                )
                tx_eff = round(
                    float(int(nic_obj.txbyte) * self.BYTE)
                    / (int(nic_obj.bandwidth) * Constant.KILOBYTE * Constant.KILOBYTE),
                    self.ROUND_NUMBER,
                )

                target_data.append(
                    [
                        self.device_id,
                        nic_obj.timestamp,
                        rx_eff,
                        nic_obj.rx_packet,
                        nic_obj.rx_error_rate if nic_obj.rx_error_rate else Constant.DEFAULT_COUNT,
                        nic_obj.rx_dropped_rate if nic_obj.rx_dropped_rate else Constant.DEFAULT_COUNT,
                        tx_eff,
                        nic_obj.tx_packet if nic_obj.tx_packet else Constant.DEFAULT_COUNT,
                        nic_obj.tx_error_rate if nic_obj.tx_error_rate else Constant.DEFAULT_COUNT,
                        nic_obj.tx_dropped_rate if nic_obj.tx_dropped_rate else Constant.DEFAULT_COUNT,
                        func_id,
                    ]
                )

        self.create_receivesend_db(target_data)

    def create_nicreceivesend_data(self: any, func_id: int) -> list:
        """
        provides tx data for get_nic_timeline method
        :param func_id: func id
        :return: None
        """
        _sql = (
            "select rxpacket/1 as rx_packet, "
            "rxerrors/rxpackets as rx_error_rate, "
            "rxdropped/rxpackets as rx_dropped_rate, "
            "txpacket/1 as tx_packet, "
            "txerrors/txpackets as tx_error_rate,"
            "txdropped/txpackets as tx_dropped_rate, "
            "timestamp, "
            "bandwidth, "
            "rxbyte, "
            "txbyte "
            "from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(DBNameConstant.TABLE_NIC_ORIGIN)
        )
        nic_obj_list = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id), dto_class=NicDto)
        return nic_obj_list

    def create_receivesend_db(self: any, target_data: list) -> None:
        """
        create new database and insert values into it
        :param target_data: target data
        :return: None
        """
        conn, curs = DBManager.create_connect_db(
            PathManager.get_db_path(self.result_dir, DBNameConstant.DB_NIC_RECEIVE)
        )
        if not conn or not curs:
            return
        self._create_rec_db_sql_exec(curs, conn)
        sql = "CREATE INDEX IF NOT EXISTS timestamp ON {}(timestamp)".format(DBNameConstant.TABLE_NIC_RECEIVE)
        DBManager.execute_sql(conn, sql)

        sql = "insert into {} values({})".format(
            DBNameConstant.TABLE_NIC_RECEIVE, '?,' * (len(target_data[0]) - 1) + "?"
        )
        DBManager.executemany_sql(conn, sql, target_data)
        del target_data[:]
        DBManager.destroy_db_connect(conn, curs)

    def calculate_nic_report_data(self: any, device: list, func_id: int) -> None:
        """
        calculate nic report data when data length > 1
        :param device: device id
        :param func_id: func id
        :return:
        """
        try:
            if device:
                target_data = []
                nic_info = {}
                tx_packet_dic = {'tx_packet_second': Constant.DEFAULT_COUNT, 'tx_packet': Constant.DEFAULT_COUNT}
                self._cal_duration_sql_exec(nic_info, device, func_id)
                self._cal_bandwidth_sql_exec(nic_info, device, func_id)
                NicModel._cal_nic_info_adapter(nic_info)

                if float(nic_info.get("duration")) != Constant.DEFAULT_COUNT:
                    self._cal_rx_bytes_packet_info(nic_info, device, func_id)
                    self._cal_tx_packet_sql_exec(tx_packet_dic, device, func_id)
                    tx_packet_dic['tx_packet_second'] = round(
                        tx_packet_dic.get('tx_packet') / float(nic_info.get("duration")), self.ROUND_NUMBER
                    )

                rx_error_rate, rx_dropped_rate = self._cal_rx_rate_adapter(nic_info, device, func_id)
                tx_error_rate, tx_dropped_rate = self._cal_tx_rate_adapter(tx_packet_dic, device, func_id)

                target_data.append(
                    (
                        device[0],
                        nic_info.get("duration"),
                        nic_info.get("bandwidth"),
                        nic_info.get("rx_bytes"),
                        nic_info.get("tx_bytes"),
                        nic_info.get("rx_packet_second"),
                        rx_error_rate,
                        rx_dropped_rate,
                        tx_packet_dic.get('tx_packet_second'),
                        tx_error_rate,
                        tx_dropped_rate,
                        func_id,
                    )
                )
                DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_NIC_REPORT, target_data)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _duration_nic_report_sql_exec(self: any, device: list, func_id: int) -> any:
        _sql = "select max(timestamp) - min(timestamp) as duration from {} where device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        try:
            duration = round(float(self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]), self.ROUND_NUMBER)
        except sqlite3.Error:
            return EmptyClass()
        return duration

    def _bandwidth_nic_report_sql_exec(self: any, device: list, func_id: int) -> any:
        _sql = "select bandwidth from {} where device_id = ? AND funcId = ?;".format(DBNameConstant.TABLE_NIC_ORIGIN)
        try:
            bandwidth = self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]
        except sqlite3.Error:
            return EmptyClass()
        return bandwidth

    def _packet_data_nic_report_sql_exec(self: any, device: list, func_id: int) -> any:
        _sql = 'select rxpacket, txpacket from {} where replayId = 0 AND device_id = ? AND funcId = ?;'.format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        try:
            packet_data = self.cur.execute(_sql, (device[0], func_id)).fetchone()
        except sqlite3.Error:
            return EmptyClass()
        return packet_data

    def _rx_data_nic_report_sql_exec(self: any, device: list, func_id: int) -> any:
        _sql = 'select rxpackets,rxerrors,rxdropped from {} where replayId = 0 AND device_id = ? AND funcId = ?'.format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        try:
            rx_data = self.cur.execute(_sql, (device[0], func_id)).fetchone()
        except sqlite3.Error:
            return EmptyClass()
        return rx_data

    def _tx_data_nic_report_sql_exec(self: any, device: list, func_id: int) -> any:
        _sql = 'select txpackets,txerrors,txdropped from {} where replayId = 0 AND device_id = ? AND funcId = ?'.format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        try:
            tx_data = self.cur.execute(_sql, (device[0], func_id)).fetchone()
        except sqlite3.Error:
            return EmptyClass()
        return tx_data

    def _tx_rate_adapter(self: any, tx_data: list, tx_rate_dic: dict) -> None:
        if tx_data[0] != Constant.DEFAULT_COUNT:
            tx_rate_dic['tx_errors_rate'] = round(tx_data[1] / tx_data[0], self.ROUND_NUMBER)
            tx_rate_dic['tx_drop_rate'] = round(tx_data[2] / tx_data[0], self.ROUND_NUMBER)

    def _rx_rate_adapter(self: any, rx_data: list, tx_rate_dic: dict) -> None:
        if rx_data[0] != Constant.DEFAULT_COUNT:
            tx_rate_dic['rx_errors_rate'] = round(rx_data[1] / rx_data[0], self.ROUND_NUMBER)
            tx_rate_dic['rx_drop_rate'] = round(rx_data[2] / rx_data[0], self.ROUND_NUMBER)

    def _create_nic_timestamp_sql_exec(self: any, func_id: int) -> list:
        _sql = "select timestamp from {0} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        time_stamp = self.cur.execute(_sql, (self.device_id, func_id)).fetchall()
        return time_stamp

    def _create_nic_bandwidth_sql_exec(self: any, func_id: int) -> float:
        _sql = "select bandwidth from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        bandwidth = self.cur.execute(_sql, (self.device_id, func_id)).fetchone()[0]
        return bandwidth

    def _create_nic_rx_byte_sql_exec(self: any, func_id: int) -> list:
        _sql = "select rxbyte from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        rx_byte = self.cur.execute(_sql, (self.device_id, func_id)).fetchall()
        return rx_byte

    def _create_nic_tx_byte_sql_exec(self: any, func_id: int) -> list:
        _sql = "select txbyte from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        tx_byte = self.cur.execute(_sql, (self.device_id, func_id)).fetchall()
        return tx_byte

    def _cal_duration_sql_exec(self: any, nic_info: dict, device: list, func_id: int) -> None:
        _sql = (
            "select max(timestamp) - min(timestamp) as duration from {} "
            "where replayId IS 0 "
            "AND device_id = ? "
            "AND funcId = ?;".format(DBNameConstant.TABLE_NIC_ORIGIN)
        )
        nic_info["duration"] = round(
            float(self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]), self.ROUND_NUMBER
        )

    def _cal_bandwidth_sql_exec(self: any, nic_info: dict, device: list, func_id: int) -> None:
        _sql = "select bandwidth from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        nic_info["bandwidth"] = self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]

    def _cal_get_rx_tx_bytes(self: any, nic_info: dict, device: list, func_id: int) -> None:
        nic_info["rx_byte"] = self.cur.execute(
            "select max(rxbytes), min(rxbytes) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_NIC_ORIGIN
            ),
            (device[0], func_id),
        ).fetchone()
        rx_bytes_be_div = float((int(nic_info.get("rx_byte")[0]) - int(nic_info.get("rx_byte")[1])) * self.BYTE)
        rx_bytes_div = (
            float(nic_info.get("duration")) * int(nic_info.get("bandwidth")) * Constant.KILOBYTE * Constant.KILOBYTE
        )
        nic_info["rx_bytes"] = round(rx_bytes_be_div / rx_bytes_div, self.ROUND_NUMBER)

        _sql = "select max(txbytes), min(txbytes) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        nic_info["tx_byte"] = self.cur.execute(_sql, (device[0], func_id)).fetchone()
        tx_bytes_be_div = float((int(nic_info.get("tx_byte")[0]) - int(nic_info.get("tx_byte")[1])) * self.BYTE)
        tx_bytes_div = (
            float(nic_info.get("duration")) * int(nic_info.get("bandwidth")) * Constant.KILOBYTE * Constant.KILOBYTE
        )
        nic_info["tx_bytes"] = round(tx_bytes_be_div / tx_bytes_div, self.ROUND_NUMBER)

    def _cal_rx_packet_sql_exec(self: any, nic_info: dict, device: list, func_id: int) -> None:
        _sql = "select sum(rxpacket) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        nic_info["rx_packet"] = self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]
        nic_info["rx_packet_second"] = round(
            nic_info.get("rx_packet") / float(nic_info.get("duration")), self.ROUND_NUMBER
        )

    def _cal_tx_packet_sql_exec(self: any, tx_packet_dic: dict, device: list, func_id: int) -> None:
        _sql = "select sum(txpacket) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        tx_packet_dic['tx_packet'] = self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]

    def _cal_rx_error_sql_exec(self: any, nic_info: dict, device: list) -> str:
        _sql = "select sum(rxerrors)/count(rxerrors) from {} where replayId = 0 AND device_id = ?;".format(
            DBNameConstant.TABLE_NIC_ORIGIN
        )
        nic_info["rx_error"] = self.cur.execute(_sql, (device[0],)).fetchone()[0]
        rx_error_rate = str(
            (round(float(nic_info.get("rx_error")) / float(nic_info.get("rx_packet")), self.ROUND_NUMBER))
            * self.PERCENTAGE
        )
        return rx_error_rate

    def _cal_rx_drop_sql_exec(self: any, nic_info: dict, device: list, func_id: int) -> str:
        _sql = (
            "select sum(rxdropped)/count(rxdropped) from {} where replayId = 0 "
            "AND device_id = ? "
            "AND funcId = ?;".format(DBNameConstant.TABLE_NIC_ORIGIN)
        )
        nic_info["rx_dropped"] = self.cur.execute(_sql, (device[0], func_id)).fetchone()[0]
        rx_dropped_rate = str(
            (round(float(nic_info.get("rx_dropped")) / float(nic_info.get("rx_packet")), self.ROUND_NUMBER))
            * self.PERCENTAGE
        )
        return rx_dropped_rate

    def _cal_tx_error_sql_exec(self: any, tx_packet_dic: dict, device: list, func_id: int) -> str:
        tx_error = self.cur.execute(
            "select sum(txerrors)/count(txerrors) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_NIC_ORIGIN
            ),
            (device[0], func_id),
        ).fetchone()[0]
        tx_error_rate = str(
            (round(float(tx_error) / float(tx_packet_dic.get('tx_packet')), self.ROUND_NUMBER)) * self.PERCENTAGE
        )
        return tx_error_rate

    def _cal_tx_drop_sql_exec(self: any, tx_packet_dic: dict, device: list, func_id: int) -> str:
        tx_dropped = self.cur.execute(
            "select sum(txdropped) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_NIC_ORIGIN
            ),
            (device[0], func_id),
        ).fetchone()[0]
        tx_dropped_rate = str(
            (round(float(tx_dropped) / float(tx_packet_dic.get('tx_packet')), self.ROUND_NUMBER)) * self.PERCENTAGE
        )
        return tx_dropped_rate

    def _cal_rx_bytes_packet_info(self: any, nic_info: dict, device: list, func_id: int) -> None:
        if nic_info.get("bandwidth") != Constant.DEFAULT_COUNT:
            self._cal_get_rx_tx_bytes(nic_info, device, func_id)
        self._cal_rx_packet_sql_exec(nic_info, device, func_id)

    def _cal_rx_rate_adapter(self: any, nic_info: dict, device: list, func_id: int) -> tuple:
        if float(nic_info.get("rx_packet")) != Constant.DEFAULT_COUNT:
            rx_error_rate = self._cal_rx_error_sql_exec(nic_info, device)
            rx_dropped_rate = self._cal_rx_drop_sql_exec(nic_info, device, func_id)
            return rx_error_rate, rx_dropped_rate
        return "0", "0"

    def _cal_tx_rate_adapter(self: any, tx_packet_dic: dict, device: list, func_id: int) -> tuple:
        if float(tx_packet_dic.get('tx_packet')) != Constant.DEFAULT_COUNT:
            tx_error_rate = self._cal_tx_error_sql_exec(tx_packet_dic, device, func_id)
            tx_dropped_rate = self._cal_tx_drop_sql_exec(tx_packet_dic, device, func_id)
            return tx_error_rate, tx_dropped_rate
        return "0", "0"

    def _report_data_per_device(self: any, device_id_list: list) -> None:
        for device in set(device_id_list):
            func_list = self.get_func_list(device)
            for func_id in func_list:
                self.create_nicreceivesend_table(func_id[0])

    def _create_nic_data_report_per_device(self: any, devices_id: list, func_ids: list) -> None:
        for device in devices_id:
            for func_id in func_ids:
                _sql = 'SELECT COUNT(rowid) FROM {} WHERE device_id = ? AND funcId = ?'.format(
                    DBNameConstant.TABLE_NIC_ORIGIN
                )
                data_length = self.cur.execute(_sql, (device[0], func_id[0])).fetchone()[0]
                if data_length == 1:  # data has only one row and do not need to be calculated
                    self.get_nic_report_data(device, func_id[0])
                else:
                    self.calculate_nic_report_data(device, func_id[0])
