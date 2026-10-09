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
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBOpen
from viewer.peripheral_report import get_peripheral_dvpp_data
from viewer.peripheral_report import get_peripheral_nic_data
from viewer.peripheral_report import format_nic_summary

NAMESPACE = 'viewer.peripheral_report'
configs = {"headers": "Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)",
           "columns": "duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,txPacket,txErrorRate,txDroppedRate,funcId"}


class TestHardwareInfoView(unittest.TestCase):
    def test_get_peripheral_dvpp_data_1(self):
        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.GetTableData.get_data_count_for_device', side_effect=sqlite3.Error):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_peripheral_dvpp_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppReportData" \
                     " (dvppid, device_id, enginetype, engineid, alltime, allframe, allutilization)"
        data = ((8, 0, "0", 3, 0.0, 0.0, "0.0%"),)
        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            _db_open.create_table(create_sql)
            _db_open.insert_data("DvppReportData", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.format_nic_summary', return_value=0):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res[2], 1)

    def test_get_peripheral_nic_data_1(self):
        with DBOpen("nic.db") as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_peripheral_nic_data('', "NicReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)
        with DBOpen("nic.db") as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.GetTableData.get_table_data_for_device', return_value=[]), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.GetTableData.get_data_count_for_device', side_effect=sqlite3.Error):
                res = get_peripheral_nic_data('', "NicReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_peripheral_nic_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS NicReportData" \
                     " (rowid, device_id, duration, bandwidth, rxbandwidth, txbandwidth, rxpacket, rxerrorrate, " \
                     "rxdroppedrate, txpacket, txerrorrate, txdroppedrate, funcid)"
        data = ((1, 0, 12303986860.0, 100000, 0.0, 0.0, 0.0, "0%", "0%", 0.0, "0%", "0%", 0),)
        with DBOpen("nic.db") as _db_open:
            _db_open.create_table(create_sql)
            _db_open.insert_data("NicReportData", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.format_nic_summary', return_value=0):
                res = get_peripheral_nic_data('', "NicReportData", 0, configs)
            self.assertEqual(res[2], 1)

    def test_format_nic_summary(self):
        data = [
            [
                15082097764880, '8.2', '4.9', '93.2', '44.23465', '34.111234', '34.624',
                '23.2352415', '77.00000', '28.4212', 1
            ]
        ]
        headers = [
            'Timestamp(us)', 'Bandwidth(MB/s)', 'Rx Bandwidth efficiency(%)', 'rxPacket/s', 'rxError rate(%)',
            'rxDropped rate(%)', 'Tx Bandwidth efficiency(%)', 'txPacket/s', 'txError rate(%)', 'txDropped rate(%)',
            'funcId'
        ]
        expect = [['15082097774.880\t', 8.2, 4.9, 93.2, 44.235, 34.111, 34.624, 23.235, 77.0, 28.421, 1]]
        res = format_nic_summary(data, headers)
        self.assertEqual(res, expect)



if __name__ == '__main__':
    unittest.main()
