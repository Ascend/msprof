#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.ge_logic_stream_singleton import GeLogicStreamSingleton

NAMESPACE = 'common_func.ge_logic_stream_singleton'
PROJECT_PATH = "/PROF_TEST"
GE_LOGIC_STREAM_DB_PATH = "PROF_TEST/host/GeLogicStreamInfo.db"


class TestGeLogicStreamSingleton(unittest.TestCase):
    def test_ge_logic_stream_singleton_load_success_when_project_is_exist(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(1, 0), (2, 0), (3, 0)]), \
                mock.patch(NAMESPACE + ".DBManager.fetchone", return_value=(3,)), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            GeLogicStreamSingleton().load_info(PROJECT_PATH)

    def test_ge_logic_stream_singleton_load_failed_when_table_is_not_exist(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(1, 0), (2, 0), (3, 0)]), \
                mock.patch(NAMESPACE + ".DBManager.fetchone", return_value=(3)), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            GeLogicStreamSingleton().load_info(PROJECT_PATH)

    def test_get_logic_stream_id_success_when_table_is_exist(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(1, 0), (2, 0), (3, 0)]), \
                mock.patch(NAMESPACE + ".DBManager.fetchone", return_value=(3,)), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            GeLogicStreamSingleton().load_info(PROJECT_PATH)
            stream_id = GeLogicStreamSingleton().get_logic_stream_id(1)

    def test_get_logic_stream_id_success_when_physic_stream_is_too_large(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(1, 0), (2, 0), (3, 0)]), \
                mock.patch(NAMESPACE + ".DBManager.fetchone", return_value=(3,)), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            GeLogicStreamSingleton().load_info(PROJECT_PATH)
            stream_id = GeLogicStreamSingleton().get_logic_stream_id(5)
            self.assertEqual(stream_id, 5)

    def test_get_logic_stream_id_success_when_physic_stream_is_exceeds(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(1, 0), (2, 0), (3, 0)]), \
                mock.patch(NAMESPACE + ".DBManager.fetchone", return_value=(65539,)), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            GeLogicStreamSingleton().load_info(PROJECT_PATH)
            stream_id = GeLogicStreamSingleton().get_logic_stream_id(5)
            self.assertEqual(stream_id, 5)


if __name__ == '__main__':
    unittest.main()
