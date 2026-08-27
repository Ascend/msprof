#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
import gc
import os
import re
import unittest

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from constant.constant import CONFIG, clear_dt_project
from constant.info_json_construct import InfoJsonReaderManager, InfoJson
from mscalculate.tiling_block_num.block_num_calculator import BlockNumCalculator
from msconfig.config_manager import ConfigManager
from msmodel.ge.ge_info_model import GeInfoViewModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'mscalculate.tiling_block_num.block_num_calculator'


class TestKfcCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_BlockNumCalculator')
    DEVICE_SQLITE_DIR = os.path.join(DIR_PATH, 'PROF', 'device_0', 'sqlite')
    HOST_SQLITE_DIR = os.path.join(DIR_PATH, 'PROF', 'host', 'sqlite')
    DATABASE_CPP = os.path.realpath(os.path.join(
        os.path.dirname(__file__), '..', '..', '..', '..', '..', '..',
        'analysis', 'csrc', 'infrastructure', 'db', 'database.cpp'))

    def get_task_info_columns(self):
        if not os.path.exists(self.DATABASE_CPP):
            self.fail('database.cpp not found: ' + self.DATABASE_CPP)
        with open(self.DATABASE_CPP, encoding='utf-8') as file:
            content = file.read()
        matched = re.search(r'const TableColumns TaskInfo = \{(.*?)\};', content, re.S)
        if matched is None:
            self.fail('parse TaskInfo table definition from database.cpp failed')
        columns = re.findall(r'\{"(\w+)",\s*SQL_(\w+)_TYPE\}', matched.group(1))
        if not columns:
            self.fail('parse TaskInfo columns from database.cpp failed')
        return columns

    def construct_ge_info_db(self):
        create_sql = DBManager.sql_create_general_table(
            DBNameConstant.TABLE_GE_TASK + "Map", DBNameConstant.TABLE_GE_TASK, ConfigManager.TABLES)
        items = ConfigManager.get(ConfigManager.TABLES).items(DBNameConstant.TABLE_GE_TASK + "Map")
        columns = [name for name, _ in items]
        defaults = {name: ("" if col_type.startswith("TEXT") else 0) for name, col_type in items}
        common = {"model_id": 0, "op_state": 0, "device_id": 0, "context_id": 4294967295,
                  "op_flag": "1", "batch_id": 0, "tensor_num": 2, "grid_dim": "NA", "block_dim": "NA"}
        rows = (
            {"op_name": "op001", "stream_id": 1, "task_id": 1, "block_num": 65535,
             "mix_block_num": 4294967295, "task_type": "AI_CORE", "op_type": "Matmul",
             "index_id": 1, "thread_id": 1, "timestamp": 123456, "hashid": "123"},
            {"op_name": "op002", "stream_id": 1, "task_id": 1, "block_num": 32, "mix_block_num": 0,
             "task_type": "MIX_AIC", "op_type": "StridedSliceD", "index_id": 0, "thread_id": 120040,
             "timestamp": 458597374830, "batch_id": 1, "hashid": "234"},
            {"op_name": "op003", "stream_id": 1, "task_id": 2, "block_num": 65535,
             "mix_block_num": 4294967295, "task_type": "AI_CORE", "op_type": "StridedSliceD",
             "index_id": 0, "thread_id": 120040, "timestamp": 458597374830, "batch_id": 1, "hashid": "345"},
        )
        data = tuple(
            tuple({**defaults, **common, **row}[name] for name in columns)
            for row in rows
        )
        db_open = DBOpen(DBNameConstant.DB_GE_INFO, sqlite_dir=self.HOST_SQLITE_DIR)
        db_open._connect_db()
        db_open.create_table(create_sql)
        db_open.insert_data(DBNameConstant.TABLE_GE_TASK, data)
        db_open._destroy_db_connect()

    def construct_ts_track_db(self):
        create_block_num_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_BLOCK_NUM + \
                                "(stream_id, task_id, block_num,timestamp)"
        block_num_data = (
            (1, 1, 8, 1000),
            (1, 2, 131080, 2000)
        )

        create_flip_num_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_DEVICE_TASK_FLIP + \
                              "(stream_id, task_id, timestamp, flip_num)"
        flip_num_data = ((1, 1, 1500, 1),)
        db_open = DBOpen(DBNameConstant.DB_STEP_TRACE, sqlite_dir=self.DEVICE_SQLITE_DIR)
        db_open._connect_db()
        db_open.create_table(create_flip_num_sql)
        db_open.create_table(create_block_num_sql)
        db_open.insert_data(DBNameConstant.TABLE_BLOCK_NUM, block_num_data)
        db_open.insert_data(DBNameConstant.TABLE_DEVICE_TASK_FLIP, flip_num_data)
        db_open._destroy_db_connect()

    def setUp(self) -> None:
        if os.path.exists(self.DEVICE_SQLITE_DIR):
            clear_dt_project(self.DEVICE_SQLITE_DIR)
        os.makedirs(self.DEVICE_SQLITE_DIR)

        if os.path.exists(self.HOST_SQLITE_DIR):
            clear_dt_project(self.HOST_SQLITE_DIR)
        os.makedirs(self.HOST_SQLITE_DIR)

    def tearDown(self) -> None:
        # 清理BlockNumCalculator中model链接，避免后续数据清理失败
        gc.collect()
        if os.path.exists(self.DIR_PATH):
            clear_dt_project(self.DIR_PATH)

    def test_task_info_map_consistent_with_cpp_definition(self: any) -> None:
        cpp_columns = [name for name, _ in self.get_task_info_columns()]
        py_columns = ConfigManager.get(ConfigManager.TABLES).options(DBNameConstant.TABLE_GE_TASK + "Map")
        self.assertEqual(cpp_columns, py_columns)

    def test_ms_run_should_return_when_contain_block_num_data(self: any) -> None:
        self.construct_ts_track_db()
        self.construct_ge_info_db()
        InfoJsonReaderManager(InfoJson(devices='0')).process()
        CONFIG.update({"result_dir": os.path.join(self.DIR_PATH, 'PROF', 'device_0')})
        check = BlockNumCalculator({}, CONFIG)
        check.ms_run()
        with GeInfoViewModel(os.path.join(self.DIR_PATH, 'PROF', 'device_0'), [DBNameConstant.TABLE_GE_TASK]) as _model:
            data = _model.get_ge_info_by_device_id(DBNameConstant.TABLE_GE_TASK, '0')
        self.assertEqual(len(data), 3)
        self.assertEqual({(8, 0), (32, 0), (8, 16)}, set((datum.block_num, datum.mix_block_num) for datum in data))