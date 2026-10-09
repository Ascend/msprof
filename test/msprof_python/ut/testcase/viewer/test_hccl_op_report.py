#!/usr/bin/python3
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

from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBOpen
from viewer.hccl_op_report import ReportHcclStatisticData

NAMESPACE = 'viewer.hccl_op_report'


class TestReportHcclStatisticData(unittest.TestCase):

    def test_report_hccl_op_should_return_MSVP_EMPTY_DATA_when_db_is_none(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = ReportHcclStatisticData.report_hccl_op('', [])
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_report_hccl_op_should_return_tuple_when_exist_HcclOpReport_table(self):
        create_sql = "CREATE TABLE IF NOT EXISTS HcclOpReport" \
                     "(op_type TEXT, occurrences NUMERIC, total_time NUMERIC, " \
                     "min NUMERIC, avg NUMERIC, max NUMERIC, ratio TEXT)"
        data = ((1, 4, 10.0, 1.0, 2.5, 4.0, 100.0),)
        with DBOpen("hccl.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("HcclOpReport", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)):
                res = ReportHcclStatisticData.report_hccl_op('', [])
            self.assertEqual(res[2], 1)
