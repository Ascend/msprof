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
import unittest
from unittest import mock

from common_func.common import byte_per_us2_mb_pers, ns2_us
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.interconnection_view import InterConnectionView
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'viewer.interconnection_view'
configs = {}


class TestInterConnectionView(unittest.TestCase):
    def test_get_hccs_data_1(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_HCCS_EVENTS + \
                     " (device_id, timestamp, txthroughput, rxthroughput)"
        data = ((0, 176168489516040, 201, 201),
                (0, 176168589568290, 422, 422))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_HCCS_EVENTS, data)
        test_sql = db_manager.create_table("hccs.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql):
            key = InterConnectionView('', configs)
            res = key.get_hccs_data(0)
        test_sql = db_manager.connect_db("hccs.db")
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_HCCS_EVENTS))
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 2)

    def test_get_not_chip_v4_pcie_summary_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS PcieOriginalData" \
                     " (timestamp, device_id, tx_p_bandwidth_min, tx_p_bandwidth_max, " \
                     "tx_p_bandwidth_avg, tx_np_bandwidth_min, tx_np_bandwidth_max, " \
                     "tx_np_bandwidth_avg, tx_cpl_bandwidth_min, tx_cpl_bandwidth_max, " \
                     "tx_cpl_bandwidth_avg, tx_np_latency_min, tx_np_latency_max, " \
                     "tx_np_latency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max, " \
                     "rx_p_bandwidth_avg, rx_np_bandwidth_min, rx_np_bandwidth_max, " \
                     "rx_np_bandwidth_avg, rx_cpl_bandwidth_min, rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg)"
        data = ((176168304972690, 0, 0, 1022, 17, 0, 8, 0, 0, 0, 0, 303, 308, 50, 0, 20, 0, 0, 0, 0, 0, 32, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("PcieOriginalData", data)
        test_sql = db_manager.create_table("pcie.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql):
            key = InterConnectionView('', configs)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            res = key.get_pcie_summary_data()
        test_sql = db_manager.connect_db("pcie.db")
        (test_sql[1]).execute("drop Table PcieOriginalData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[0], ["Mode", "Min", "Max", "Avg"])
        self.assertEqual(res[2], 7)

    def test_get_chip_v4_pcie_summary_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS PcieOriginalData" \
                     " (timestamp, device_id, tx_p_bandwidth_min, tx_p_bandwidth_max, " \
                     "tx_p_bandwidth_avg, tx_np_bandwidth_min, tx_np_bandwidth_max, " \
                     "tx_np_bandwidth_avg, tx_cpl_bandwidth_min, tx_cpl_bandwidth_max, " \
                     "tx_cpl_bandwidth_avg, tx_np_latency_min, tx_np_latency_max, " \
                     "tx_np_latency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max, " \
                     "rx_p_bandwidth_avg, rx_np_bandwidth_min, rx_np_bandwidth_max, " \
                     "rx_np_bandwidth_avg, rx_cpl_bandwidth_min, rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg)"
        data = ((176168304972690, 0, 0, 1022, 17, 0, 8, 0, 0, 0, 0, 303, 308, 50, 0, 20, 0, 0, 0, 0, 0, 32, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("PcieOriginalData", data)
        test_sql = db_manager.create_table("pcie.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql):
            key = InterConnectionView('', configs)
            ChipManager().chip_id = ChipModel.CHIP_V4_1_0
            res = key.get_pcie_summary_data()
        test_sql = db_manager.connect_db("pcie.db")
        (test_sql[1]).execute("drop Table PcieOriginalData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[0], ["Mode", "Min", "Max", "Avg"])
        self.assertEqual(res[2], 7)

    def test_get_pcie_summary_data_return_empty_data(self):
        db = DBManager()
        test_sql = db.create_table('test.db')
        db.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            key = InterConnectionView('', configs)
            res = key.get_pcie_summary_data()
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            key = InterConnectionView('', configs)
            res = key.get_pcie_summary_data()
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            key = InterConnectionView('', configs)
            res = key.get_pcie_summary_data()
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)


if __name__ == '__main__':
    unittest.main()
