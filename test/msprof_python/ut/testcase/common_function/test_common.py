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

import pytest

from common_func.common import get_data_file_memory, check_free_memory
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.msprof_exception import ProfException

NAMESPACE = 'common_func.common'


def test_get_data_file_memory():
    res_1 = get_data_file_memory(" ")
    unittest.TestCase().assertEqual(res_1, Constant.MAX_READ_FILE_BYTES)
    with mock.patch('os.listdir', return_value=["1"]), \
            mock.patch('os.path.getsize', return_value=1048576):
        res = get_data_file_memory("./")
        unittest.TestCase().assertEqual(res, 1)


def test_check_free_memory():
    with mock.patch(NAMESPACE + '.get_data_file_memory', return_value=1), \
            mock.patch(NAMESPACE + '.warn'), \
            mock.patch(NAMESPACE + '.get_free_memory', return_value=100):
        check_free_memory("./")
    with mock.patch(NAMESPACE + '.get_data_file_memory', return_value=1):
        check_free_memory("./")


def test_fetch_all_data():
    expect_res = []
    curs = mock.Mock()
    curs.execute.return_value = ValueError
    res = DBManager.fetch_all_data(curs, "")
    unittest.TestCase().assertEqual(expect_res, res)

    curs2 = mock.Mock(sqlite3.Cursor)
    curs2.fetchmany.return_value = [(0,)] * 10000
    curs2.execute.return_value = curs2
    curs2.fetchone.return_value = (0, 'main', 'api_event.db')
    with pytest.raises(ProfException) as error:
        DBManager.fetch_all_data(curs2, "")
        unittest.TestCase().assertEqual(error.value, 13)


if __name__ == '__main__':
    unittest.main()
