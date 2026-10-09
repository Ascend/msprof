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

import collections
import logging
import sqlite3
from abc import ABC

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from msconfig.config_manager import ConfigManager
from msmodel.interface.base_model import BaseModel


class RoceModel(BaseModel, ABC):
    """
    acsq task model class
    """

    DEFAULT_NIC_FUNC_ID = 0
    ROUND_NUMBER = 3
    PERCENTAGE = 100
    BYTE = 8
    NETWORK_HEADER_TAG = 'rxPacket/s'
    TABLES_PATH = ConfigManager.TABLES_TRAINING

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.device_id = InfoConfReader().get_device_list()[0] if InfoConfReader().get_device_list() else '0'

    @staticmethod
    def _init_roce_rx_data_with_index(index: int, rx_data: dict) -> None:
        rx_data["rxPacket/s"][index] = rx_data["rxPacket/s"][index][0]
        if not rx_data["rxError rate"][index][0]:
            rx_data["rxError rate"][index] = Constant.DEFAULT_COUNT
        else:
            rx_data["rxError rate"][index] = rx_data["rxError rate"][index][0]
        if not rx_data["rxDropped rate"][index][0]:
            rx_data["rxDropped rate"][index] = Constant.DEFAULT_COUNT
        else:
            rx_data["rxDropped rate"][index] = rx_data["rxDropped rate"][index][0]

    @staticmethod
    def _init_roce_tx_data_with_index(index: int, tx_data: dict) -> None:
        if not tx_data["txPacket/s"][index][0]:
            tx_data["txPacket/s"][index] = Constant.DEFAULT_COUNT
        else:
            tx_data["txPacket/s"][index] = tx_data["txPacket/s"][index][0]
        if not tx_data["txError rate"][index][0]:
            tx_data["txError rate"][index] = Constant.DEFAULT_COUNT
        else:
            tx_data["txError rate"][index] = tx_data["txError rate"][index][0]
        if not tx_data["txDropped rate"][index][0]:
            tx_data["txDropped rate"][index] = Constant.DEFAULT_COUNT
        else:
            tx_data["txDropped rate"][index] = tx_data["txDropped rate"][index][0]

    @staticmethod
    def _create_receive_send_db(conn: any, curs: any) -> None:
        sql = (
            "CREATE TABLE IF NOT EXISTS {} (device_id integer, timestamp real, "
            "rx_bandwidth_efficiency real, rx_packets real, "
            "rx_error_rate real, rx_dropped_rate real, "
            "tx_bandwidth_efficiency real, tx_packets real, "
            "tx_error_rate real, tx_dropped_rate real, func_id integer)".format(DBNameConstant.TABLE_ROCE_RECEIVE)
        )
        DBManager.execute_sql(conn, sql)

    @staticmethod
    def _create_timestamp_index(conn: any, curs: any) -> None:
        sql = "CREATE INDEX IF NOT EXISTS timestamp ON {}(timestamp)".format(DBNameConstant.TABLE_ROCE_RECEIVE)
        DBManager.execute_sql(conn, sql)

    @staticmethod
    def _insert_receive_send_data(conn: any, target_data: list) -> None:
        sql = "insert into {} values({})".format(
            DBNameConstant.TABLE_ROCE_RECEIVE, '?,' * (len(target_data[0]) - 1) + "?"
        )
        DBManager.executemany_sql(conn, sql, target_data)

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_ROCE_ORIGIN, data_list)

    def report_data(self: any, device_id_list: list) -> any:
        """
        summary data of roce
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ROCE_ORIGIN):
            return
        self.create_roce_data_report()
        self.create_roce_tree_data()
        try:
            for device in set(device_id_list):
                func_list = self.get_func_list(device)
                for func_id in func_list:
                    self.create_rocereceivesend_table(func_id[0])
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def create_roce_data_report(self: any) -> None:
        """
        create roce report data table and insert data
        :return: None
        """
        self._try_to_init_roce_table()
        device_ids = self._get_devices()
        func_ids = self._get_function_ids()
        if not device_ids or not func_ids:
            return
        try:
            self._do_roce_data_report(device_ids, func_ids)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def create_roce_tree_data(self: any) -> None:
        """
        create roce tree data table and insert data
        :return: None
        """
        _table_name = 'RoceTreeData'
        _sql = "SELECT DISTINCT(device_id) FROM {} WHERE replayid IS 0;".format(DBNameConstant.TABLE_ROCE_ORIGIN)
        roce_device = DBManager.fetch_all_data(self.cur, _sql)
        DBManager.execute_sql(self.conn, "CREATE TABLE IF NOT EXISTS {} (device_id Int)".format(_table_name))
        DBManager.executemany_sql(self.conn, "INSERT INTO {} VALUES (?)".format(_table_name), roce_device)

    def get_func_list(self: any, device: int) -> list:
        """
        Get func lists in original data.
        :param device: device id
        :return: func id
        """
        _sql = 'SELECT DISTINCT(funcId) FROM {} WHERE device_id IS ?'.format(DBNameConstant.TABLE_ROCE_ORIGIN)
        func_list = DBManager.fetch_all_data(self.cur, _sql, (device,))
        return func_list

    def get_roce_report_data(self: any, device: list, func_id: int) -> None:
        """
        get roce report data when data length is 1
        :param device: device id
        :param func_id: func id
        :return: None
        """
        roce_duration = self._get_roce_duration(device[0], func_id)
        roce_bandwidth = self._get_roce_bandwidth(device[0], func_id)
        roce_packet_data = self._get_roce_packet_data(device[0], func_id)
        roce_rx_data = self._get_roce_rx_data(device[0], func_id)
        roce_tx_rate_data = self._get_roce_tx_rate_data(device[0], func_id, roce_rx_data)
        self._insert_roce_data(device[0], func_id, roce_duration, roce_bandwidth, roce_packet_data, roce_tx_rate_data)

    def create_rocereceivesend_table(self: any, func_id: int) -> None:
        """
        provides data for get_roce_timeline method
        :param func_id: func id
        :return: None
        """
        time_stamp = self._get_roce_timestamp(func_id)
        bandwidth = self._get_roce_bandwidth(self.device_id, func_id)
        rx_byte = self._get_roce_rx_byte(func_id)
        tx_byte = self._get_roce_tx_byte(func_id)
        rx_data, tx_data = self.create_rocereceivesend_data(func_id)

        eff_dic = {'rx_eff': [], 'tx_eff': []}
        for i, _ in enumerate(time_stamp):
            if int(bandwidth):
                time_stamp[i] = time_stamp[i][0]
                eff_dic.get('rx_eff').append(
                    round(
                        float(int(rx_byte[i][0]) * self.BYTE)
                        / (int(bandwidth) * Constant.KILOBYTE * Constant.KILOBYTE),
                        self.ROUND_NUMBER,
                    )
                )
                RoceModel._init_roce_rx_data_with_index(i, rx_data)
                eff_dic.get('tx_eff').append(
                    round(
                        float(int(tx_byte[i][0]) * self.BYTE)
                        / (int(bandwidth) * Constant.KILOBYTE * Constant.KILOBYTE),
                        self.ROUND_NUMBER,
                    )
                )
                RoceModel._init_roce_tx_data_with_index(i, tx_data)
        rx_data["Rx Bandwidth efficiency(%)"] = eff_dic.get('rx_eff')
        tx_data["Tx Bandwidth efficiency(%)"] = eff_dic.get('tx_eff')

        target_data = list(
            zip(
                (self.device_id for _ in time_stamp),
                time_stamp,
                rx_data.get("Rx Bandwidth efficiency(%)"),
                rx_data.get("rxPacket/s"),
                rx_data.get("rxError rate"),
                rx_data.get("rxDropped rate"),
                tx_data.get("Tx Bandwidth efficiency(%)"),
                tx_data.get("txPacket/s"),
                tx_data.get("txError rate"),
                tx_data.get("txDropped rate"),
                (func_id for _ in time_stamp),
            )
        )

        self.create_receivesend_db(target_data)

    def create_rocereceivesend_data(self: any, func_id: int) -> tuple:
        """
        provides tx data for get_roce_timeline method
        :param func_id: func id
        :return: None
        """
        rx_data = collections.OrderedDict()
        tx_data = collections.OrderedDict()

        _sql = "select rxpacket/1 from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        rx_data["rxPacket/s"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))

        _sql = "select rxerrors/rxpackets from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        rx_data["rxError rate"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))

        _sql = "select rxdropped/rxpackets from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        rx_data["rxDropped rate"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))

        _sql = "select txpacket/1 from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_data["txPacket/s"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))

        _sql = "select txerrors/txpackets from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_data["txError rate"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))

        _sql = "select txdropped/txpackets from {} where replayId IS 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_data["txDropped rate"] = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))
        return rx_data, tx_data

    def create_receivesend_db(self: any, target_data: list) -> None:
        """
        create new database and insert values into it
        :param target_data: roce data
        :return: None
        """
        conn, curs = DBManager.create_connect_db(
            PathManager.get_db_path(self.result_dir, DBNameConstant.DB_ROCE_RECEIVE)
        )
        if not conn or not curs:
            return
        RoceModel._create_receive_send_db(conn, curs)
        RoceModel._create_timestamp_index(conn, curs)
        RoceModel._insert_receive_send_data(conn, target_data)
        del target_data[:]
        DBManager.destroy_db_connect(conn, curs)

    def calculate_roce_report_data(self: any, device: list, func_id: int) -> None:
        """
        calculate roce report data when data length > 1
        :param device: device id
        :param func_id: func id
        :return: None
        """
        roce_info = self._construct_roce_info(device[0], func_id)
        tx_packet_dic = {'tx_packet_second': Constant.DEFAULT_COUNT, 'tx_packet': Constant.DEFAULT_COUNT}
        self._init_roce_basic_info(roce_info, tx_packet_dic, device[0], func_id)
        rx_error_rate, rx_dropped_rate, tx_error_rate, tx_dropped_rate = self._init_roce_packet(
            roce_info, tx_packet_dic, device[0], func_id
        )
        self._insert_roce_report_data(
            device[0], func_id, roce_info, rx_error_rate, rx_dropped_rate, tx_packet_dic, tx_error_rate, tx_dropped_rate
        )

    def _try_to_init_roce_table(self: any) -> None:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ROCE_REPORT):
            create_sql = DBManager.sql_create_general_table(
                DBNameConstant.TABLE_ROCE_REPORT + 'Map', DBNameConstant.TABLE_ROCE_REPORT, self.TABLES_PATH
            )
            DBManager.execute_sql(self.conn, create_sql)

    def _get_devices(self: any) -> list:
        _sql = "select distinct(device_id) from {};".format(DBNameConstant.TABLE_ROCE_ORIGIN)
        devices_id = DBManager.fetch_all_data(self.cur, _sql)
        return devices_id

    def _get_function_ids(self: any) -> list:
        _sql = "select distinct(funcId) from {};".format(DBNameConstant.TABLE_ROCE_ORIGIN)
        func_ids = DBManager.fetch_all_data(self.cur, _sql)
        return func_ids

    def _do_roce_data_report(self: any, device_ids: list, func_ids: list) -> None:
        try:
            self._do_roce_data_report_helper(device_ids, func_ids)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _do_roce_data_report_helper(self: any, device_ids: list, func_ids: list) -> None:
        for device in device_ids:
            for func_id in func_ids:
                _sql = 'SELECT COUNT(rowid) FROM {} WHERE device_id = ? AND funcId = ?'.format(
                    DBNameConstant.TABLE_ROCE_ORIGIN
                )
                data_length = self.cur.execute(_sql, (device[0], func_id[0])).fetchone()[0]
                if data_length == 1:  # data has only one row and do not need to be calculated
                    self.get_roce_report_data(device, func_id[0])
                else:
                    self.calculate_roce_report_data(device, func_id[0])

    def _get_roce_duration(self: any, device_id: str, func_id: str) -> float:
        _sql = "select max(timestamp) - min(timestamp) as duration from {} where device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        duration = round(float(self.cur.execute(_sql, (device_id, func_id)).fetchone()[0]), self.ROUND_NUMBER)
        return duration

    def _get_roce_bandwidth(self: any, device_id: str, func_id: str) -> float:
        _sql = "select bandwidth from {} where device_id = ? AND funcId = ?;".format(DBNameConstant.TABLE_ROCE_ORIGIN)
        bandwidth = self.cur.execute(_sql, (device_id, func_id)).fetchone()[0]
        return bandwidth

    def _get_roce_packet_data(self: any, device_id: str, func_id: str) -> list:
        _sql = 'select rxpacket, txpacket from {} where replayId = 0 AND device_id = ? AND funcId = ?;'.format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        packet_data = self.cur.execute(_sql, (device_id, func_id)).fetchone()
        return packet_data

    def _get_roce_rx_data(self: any, device_id: str, func_id: str) -> list:
        _sql = 'select rxpackets,rxerrors,rxdropped from {} where replayId = 0 AND device_id = ? AND funcId = ?'.format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        rx_data = self.cur.execute(_sql, (device_id, func_id)).fetchone()
        return rx_data

    def _get_roce_tx_rate_data(self: any, device_id: str, func_id: str, rx_data: list) -> dict:
        tx_rate_dic = {
            'rx_errors_rate': Constant.DEFAULT_COUNT,
            'rx_drop_rate': Constant.DEFAULT_COUNT,
            'tx_errors_rate': Constant.DEFAULT_COUNT,
            'tx_drop_rate': Constant.DEFAULT_COUNT,
        }
        if rx_data[0] != Constant.DEFAULT_COUNT:
            tx_rate_dic['rx_errors_rate'] = round(rx_data[1] / rx_data[0], self.ROUND_NUMBER)
            tx_rate_dic['rx_drop_rate'] = round(rx_data[2] / rx_data[0], self.ROUND_NUMBER)

        _sql = 'select txpackets,txerrors,txdropped from {} where replayId = 0 AND device_id = ? AND funcId = ?'.format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_data = self.cur.execute(_sql, (device_id, func_id)).fetchone()

        if tx_data[0] != Constant.DEFAULT_COUNT:
            tx_rate_dic['tx_errors_rate'] = round(tx_data[1] / tx_data[0], self.ROUND_NUMBER)
            tx_rate_dic['tx_drop_rate'] = round(tx_data[2] / tx_data[0], self.ROUND_NUMBER)
        return tx_rate_dic

    def _insert_roce_data(self: any, *param: any) -> None:
        device_id, func_id, duration, bandwidth, packet_data, tx_rate_dic = param
        item = [
            device_id,
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

        _sql = "insert into {} values({})".format(DBNameConstant.TABLE_ROCE_REPORT, '?,' * (len(item) - 1) + "?")
        self.cur.executemany(_sql, [item])
        self.conn.commit()

    def _get_roce_timestamp(self: any, func_id: str) -> list:
        _sql = "select timestamp from {0} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        time_stamp = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))
        return time_stamp

    def _get_roce_rx_byte(self: any, func_id: str) -> list:
        _sql = "select rxbyte from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        rx_byte = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))
        return rx_byte

    def _get_roce_tx_byte(self: any, func_id: str) -> list:
        _sql = "select txbyte from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_byte = DBManager.fetch_all_data(self.cur, _sql, (self.device_id, func_id))
        return tx_byte

    def _init_rx_byte_in_roce_info(self: any, roce_info: dict, device_id: str, func_id: str) -> None:
        roce_info["rx_byte"] = self.cur.execute(
            "select max(rxbytes), min(rxbytes) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_ROCE_ORIGIN
            ),
            (device_id, func_id),
        ).fetchone()
        total_byes = float((int(roce_info.get("rx_byte")[0]) - int(roce_info.get("rx_byte")[1])) * self.BYTE)
        duration = float(roce_info.get("duration"))
        bandwidth = int(roce_info.get("bandwidth"))
        roce_info["rx_bytes"] = round(
            total_byes / (duration * bandwidth * Constant.KILOBYTE * Constant.KILOBYTE), self.ROUND_NUMBER
        )

    def _init_tx_byte_in_roce_info(self: any, roce_info: dict, device_id: str, func_id: str) -> None:
        _sql = "select max(txbytes), min(txbytes) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        roce_info["tx_byte"] = self.cur.execute(_sql, (device_id, func_id)).fetchone()
        total_byes = float((int(roce_info.get("tx_byte")[0]) - int(roce_info.get("tx_byte")[1])) * self.BYTE)
        duration = float(roce_info.get("duration"))
        bandwidth = int(roce_info.get("bandwidth"))
        roce_info["tx_bytes"] = round(
            total_byes / (duration * bandwidth * Constant.KILOBYTE * Constant.KILOBYTE), self.ROUND_NUMBER
        )

    def _init_rx_packet_in_roce_info(self: any, roce_info: dict, device_id: str, func_id: str) -> None:
        _sql = "select sum(rxpacket) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        roce_info["rx_packet"] = self.cur.execute(_sql, (device_id, func_id)).fetchone()[0]
        roce_info["rx_packet_second"] = round(
            roce_info.get("rx_packet") / float(roce_info.get("duration")), self.ROUND_NUMBER
        )

    def _init_tx_packet_in_tx_packet_dic(
        self: any, tx_packet_dic: dict, device_id: str, func_id: str, duration: float
    ) -> None:
        _sql = "select sum(txpacket) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        tx_packet_dic['tx_packet'] = self.cur.execute(_sql, (device_id, func_id)).fetchone()[0]
        tx_packet_dic['tx_packet_second'] = round(tx_packet_dic.get('tx_packet') / duration, self.ROUND_NUMBER)

    def _get_rx_error_rate(self: any, roce_info: dict, device_id: str) -> str:
        _sql = "select sum(rxerrors)/count(rxerrors) from {} where replayId = 0 AND device_id = ?;".format(
            DBNameConstant.TABLE_ROCE_ORIGIN
        )
        roce_info["rx_error"] = self.cur.execute(_sql, (device_id,)).fetchone()[0]

        rx_error_rate = str(
            (round(float(roce_info.get("rx_error")) / float(roce_info.get("rx_packet")), self.ROUND_NUMBER))
            * self.PERCENTAGE
        )
        return rx_error_rate

    def _get_rx_dropped_rate(self: any, roce_info: dict, device_id: str, func_id: str) -> str:
        _sql = (
            "select sum(rxdropped)/count(rxdropped) from {} where replayId = 0 "
            "AND device_id = ? "
            "AND funcId = ?;".format(DBNameConstant.TABLE_ROCE_ORIGIN)
        )
        roce_info["rx_dropped"] = self.cur.execute(_sql, (device_id, func_id)).fetchone()[0]
        rx_dropped_rate = str(
            (round(float(roce_info.get("rx_dropped")) / float(roce_info.get("rx_packet")), self.ROUND_NUMBER))
            * self.PERCENTAGE
        )
        return rx_dropped_rate

    def _get_tx_error_rate(self: any, tx_packet_dic: dict, device_id: str, func_id: str) -> str:
        tx_error = self.cur.execute(
            "select sum(txerrors)/count(txerrors) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_ROCE_ORIGIN
            ),
            (device_id, func_id),
        ).fetchone()[0]
        tx_error_rate = str(
            (round(float(tx_error) / float(tx_packet_dic.get('tx_packet')), self.ROUND_NUMBER)) * self.PERCENTAGE
        )
        return tx_error_rate

    def _get_tx_dropped_rate(self: any, tx_packet_dic: dict, device_id: str, func_id: str) -> str:
        tx_dropped = self.cur.execute(
            "select sum(txdropped) from {} where replayId = 0 AND device_id = ? AND funcId = ?;".format(
                DBNameConstant.TABLE_ROCE_ORIGIN
            ),
            (device_id, func_id),
        ).fetchone()[0]
        tx_dropped_rate = str(
            (round(float(tx_dropped) / float(tx_packet_dic.get('tx_packet')), self.ROUND_NUMBER)) * self.PERCENTAGE
        )
        return tx_dropped_rate

    def _insert_roce_report_data(self: any, *param: list) -> None:
        device_id, func_id, roce_info, rx_error_rate, rx_dropped_rate, tx_packet_dic, tx_error_rate, tx_dropped_rate = (
            param
        )
        item = (
            device_id,
            roce_info.get("duration"),
            roce_info.get("bandwidth"),
            roce_info.get("rx_bytes"),
            roce_info.get("tx_bytes"),
            roce_info.get("rx_packet_second"),
            rx_error_rate,
            rx_dropped_rate,
            tx_packet_dic.get('tx_packet_second'),
            tx_error_rate,
            tx_dropped_rate,
            func_id,
        )
        _sql = "insert into {} values({})".format(DBNameConstant.TABLE_ROCE_REPORT, '?,' * (len(item) - 1) + "?")
        self.cur.executemany(_sql, [item])
        self.conn.commit()

    def _construct_roce_info(self: any, device_id: str, func_id: str) -> dict:
        roce_info = {
            "duration": self._get_roce_duration(device_id, func_id),
            "bandwidth": self._get_roce_bandwidth(device_id, func_id),
            "tx_bytes": Constant.DEFAULT_COUNT,
            "rx_bytes": Constant.DEFAULT_COUNT,
            "rx_packet": Constant.DEFAULT_COUNT,
            "rx_packet_second": Constant.DEFAULT_COUNT,
        }
        return roce_info

    def _init_roce_basic_info(self: any, roce_info: dict, tx_packet_dic: dict, device_id: str, func_id: str) -> None:
        if float(roce_info.get("duration")) != Constant.DEFAULT_COUNT:
            if roce_info.get("bandwidth") != Constant.DEFAULT_COUNT:
                self._init_rx_byte_in_roce_info(roce_info, device_id, func_id)
                self._init_tx_byte_in_roce_info(roce_info, device_id, func_id)
                self._init_rx_packet_in_roce_info(roce_info, device_id, func_id)
                self._init_tx_packet_in_tx_packet_dic(
                    tx_packet_dic, device_id, func_id, float(roce_info.get("duration"))
                )

    def _init_roce_packet(self: any, roce_info: dict, tx_packet_dic: dict, device_id: str, func_id: str) -> tuple:
        rx_error_rate, rx_dropped_rate = "0", "0"
        tx_error_rate, tx_dropped_rate = "0", "0"
        if float(roce_info.get("rx_packet")) != Constant.DEFAULT_COUNT:
            rx_error_rate = self._get_rx_error_rate(roce_info, device_id)
            rx_dropped_rate = self._get_rx_dropped_rate(roce_info, device_id, func_id)

        if float(tx_packet_dic.get('tx_packet')) != Constant.DEFAULT_COUNT:
            tx_error_rate = self._get_tx_error_rate(tx_packet_dic, device_id, func_id)
            tx_dropped_rate = self._get_tx_dropped_rate(tx_packet_dic, device_id, func_id)
        rate_info = (rx_error_rate, rx_dropped_rate, tx_error_rate, tx_dropped_rate)
        return rate_info
