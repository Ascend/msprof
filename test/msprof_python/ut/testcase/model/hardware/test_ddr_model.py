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
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import INFO_JSON
from msmodel.hardware.ddr_model import DdrModel
from sqlite.db_manager import DBManager
from constant.info_json_construct import InfoJson

NAMESPACE = 'msmodel.hardware.ddr_model'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}


class TestDdrModel(unittest.TestCase):

    def test_drop_tab(self):
        InfoJson(devices='0').apply()
        db_manager = DBManager()
        res = db_manager.create_table('ddr.db')
        key = DdrModel('test', 'ddr.db', ['DDRMetricData', 'DDROriginalData'])
        key.conn, key.cur = res[0], res[1]
        key.drop_tab()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            key = DdrModel('test', 'ddr.db', ['DDRMetricData', 'DDROriginalData'])
            key.conn, key.cur = res[0], res[1]
            key.drop_tab()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DdrModel.insert_data_to_db'):
            InfoJson(devices='0').apply()
            key = DdrModel('test', 'ddr.db', ['DDRMetricData', 'DDROriginalData'])
            key.flush([])

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DdrModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = DdrModel('test', 'hbm.db', ['DDRMetricData', 'DDROriginalData'])
            check.create_table()

    def test_insert_metric_data(self):
        create_sql = "create table IF NOT EXISTS DDROriginalData (device_id INT,replayid INT,timestamp REAL," \
                     "counts INT,unit VARCHAR,event VARCHAR)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('DDROriginalData')
        data = ((0, 0, 2, 1, 0, 5),)
        create_sql_1 = "create table IF NOT EXISTS DDRMetricData (device_id INT,replayid INT,timestamp REAL," \
                       "flux_read REAL,flux_write REAL)"
        insert_sql_1 = "insert into {} values (?,?,?,?,?)".format('DDRMetricData')
        data_1 = ((0, 3, 2, 3260340952538.83, 3260341066054.46),)
        db_manager = DBManager()
        db_manager.create_table("ddr.db", create_sql, insert_sql, data)
        res = db_manager.create_table('ddr.db', create_sql_1, insert_sql_1, data_1)
        with mock.patch(NAMESPACE + '.ConfigMgr.get_ddr_bit_width', return_value=256), \
                mock.patch(NAMESPACE + '.DdrModel.calculate_data',
                           return_value=[
                               [5, 0, 176167361616990.0, 595.0927734375, 30.517578125],
                               [5, 0, 176167461567990.0, 83.33062692907025, 56.44061977087522]]
                           ):
            key = DdrModel('test', 'hbm.db', ['DDRMetricData', 'DDROriginalData'])
            InfoConfReader._info_json = INFO_JSON
            key.conn, key.cur = res[0], res[1]
            key.insert_metric_data(0)
        res[1].execute("drop table DDRMetricData")
        res[0].commit()
        res[1].execute("create table IF NOT EXISTS DDRMetricData (device_id INT,replayid INT,timestamp REAL,"
                       "flux_read REAL,flux_write REAL,fluxid_read REAL,fluxid_write REAL)")
        with mock.patch(NAMESPACE + '.ConfigMgr.get_ddr_bit_width', return_value=256), \
                mock.patch(NAMESPACE + '.DdrModel.calculate_data',
                           return_value=[
                               [5, 0, 176167361616990.0, 595.0927734375, 30.517578125, 1, 2],
                               [5, 0, 176167461567990.0, 83.33062692907025, 56.44061977087522, 3, 4]]
                           ):
            key = DdrModel('test', 'hbm.db', ['DDRMetricData', 'DDROriginalData'])
            InfoConfReader._info_json = INFO_JSON
            key.conn, key.cur = res[0], res[1]
            key.insert_metric_data(1)
        res[1].execute("drop table DDRMetricData")
        res[1].execute("drop table DDROriginalData")
        res[0].commit()
        db_manager.destroy(res)

    def test_calculate_data_1(self):
        ddr_data = [[1, 2, 3, 4], [4, 5, 6, 7], [7, 8, 9, 9]]
        start_time = 0
        InfoConfReader()._info_json = INFO_JSON
        InfoConfReader()._info_json["platform_version"] = 0
        key = DdrModel('test', 'ddr.db', ['DDRMetricData', 'DDROriginalData'])
        result = key.calculate_data(ddr_data, start_time)
        data = [[1, 2, 3, 20345.052083333332], [4, 5, 6, 35603.841145833336], [7, 8, 9, 45776.3671875]]
        self.assertEqual(result, data)


if __name__ == '__main__':
    unittest.main()
