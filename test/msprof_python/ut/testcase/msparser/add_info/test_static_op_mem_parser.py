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
import os
import shutil
import struct
import unittest
from unittest import mock

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FdOpen
from common_func.hash_dict_constant import HashDictData
from msparser.add_info.static_op_mem_parser import StaticOpMemParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.static_op_mem_parser'


class TestStaticOpMemParser(unittest.TestCase):
    file_list = {
        DataTag.STATIC_OP_MEM: [
            'unaging.additional.static_op_mem.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "static_op_mem")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0',
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setUpClass(cls):
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_static_op_mem_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_static_op_mem_data(cls):
        static_op_mem_data = [
            [47900023386, 223338552992, 5972190379961, 1280032, 7104496996232435956, 1, 6, 0, 0, 0] + [0] * 180,
            [47900023386, 223338552992, 5972190427855, 800032, 2493395835644572533, 8, 4294967294, 0, 0, 0] + [0] * 180
        ]
        with FdOpen(os.path.join(cls.DATA_PATH, "unaging.additional.static_op_mem.slice_0"), operate="wb") as f:
            for data in static_op_mem_data:
                bytes_data = struct.pack(StructFmt.STATIC_OP_MEM_FMT, *data)
                f.write(bytes_data)

    def test_ms_run_should_return_when_bin_data_is_lost(self):
        with mock.patch(NAMESPACE + '.StaticOpMemParser.parse'), \
                mock.patch(NAMESPACE + '.StaticOpMemParser.save'):
            StaticOpMemParser(self.file_list, self.CONFIG).ms_run()
            file_list = {DataTag.STATIC_OP_MEM: []}
            StaticOpMemParser(file_list, self.CONFIG).ms_run()

    def test_parse_should_parse_bin_data_when_bin_data_is_existed(self):
        check = StaticOpMemParser(self.file_list, self.CONFIG)
        check.parse()
        expected_length = 2
        expected_col_num = 6
        self.assertEqual(len(check._static_op_data), expected_length)
        self.assertEqual(len(check._static_op_data[0]), expected_col_num)

    def test_save_should_return_when_static_op_mem_is_empty(self):
        check = StaticOpMemParser(self.file_list, self.CONFIG)
        check._static_op_data = []
        check.save()

    def test_save_should_save_when_static_op_mem_is_not_empty(self):
        check = StaticOpMemParser(self.file_list, self.CONFIG)
        HashDictData('static_op_mem')._ge_hash_dict = {
            '7104496996232435956': 'sqrt_01',
            '2493395835644572533': 'trans_TransData_0'
        }
        check._static_op_data = [
            [7104496996232435956, 0, 0, 1, 6, 1280032],
            [2493395835644572533, 0, 0, 8, 26, 800032]
        ]
        check.save()
        db_path = os.path.join(self.SQLITE_PATH, DBNameConstant.DB_STATIC_OP_MEM)
        ret = DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STATIC_OP_MEM)
        self.assertTrue(ret, True)

if __name__ == '__main__':
    unittest.main()
