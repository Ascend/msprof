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
import unittest

from common_func.db_name_constant import DBNameConstant
from constant.constant import CONFIG, clear_dt_project
from constant.info_json_construct import InfoJsonReaderManager, InfoJson
from mscalculate.tiling_block_num.block_num_calculator import BlockNumCalculator
from msmodel.ge.ge_info_model import GeInfoViewModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'mscalculate.tiling_block_num.block_num_calculator'


class TestKfcCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_BlockNumCalculator')
    DEVICE_SQLITE_DIR = os.path.join(DIR_PATH, 'PROF', 'device_0', 'sqlite')
    HOST_SQLITE_DIR = os.path.join(DIR_PATH, 'PROF', 'host', 'sqlite')

    def construct_ge_info_db(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_TASK + \
                     "(model_id, op_name, stream_id, task_id, block_num, mix_block_num," \
                     "op_state, task_type, op_type, index_id, thread_id, timestamp, batch_id," \
                     "tensor_num, input_formats, input_data_types, input_shapes," \
                     "output_formats, output_data_types, output_shapes, device_id, context_id, op_flag, hashid)"
        data = (
            (0, "op001", 1, 1, 65535, 4294967295, 0, 'AI_CORE', "Matmul", 1, 1, 123456, 0, 2, "", "",
             "", "", "", "", 0, 4294967295, "1", "123"),
            (0, "op002", 1, 1, 32, 0, 0, "MIX_AIC", "StridedSliceD", 0, 120040, 458597374830, 1, 2, "",
             "", "", "", "", "", 0, 4294967295, "1", "234"),
            (0, "op003", 1, 2, 65535, 4294967295, 0, "AI_CORE", "StridedSliceD", 0, 120040, 458597374830, 1, 2,
             "", "", "", "", "", "", 0, 4294967295, "1", "345")
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

    def test_ms_run_should_return_when_contain_block_num_data(self: any) -> None:
        self.construct_ts_track_db()
        self.construct_ge_info_db()
        InfoJsonReaderManager(InfoJson(devices='0')).process()
        CONFIG.update({"result_dir": os.path.join(self.DIR_PATH, 'PROF', 'device_0')})
        check = BlockNumCalculator({}, CONFIG)
        check.ms_run()
        with GeInfoViewModel(os.path.join(self.DIR_PATH, 'PROF', 'device_0'), [DBNameConstant.TABLE_GE_TASK]) as _model:
            data = _model.get_ge_info_by_device_id(DBNameConstant.TABLE_GE_TASK, '0')
        self.assertEqual({(8, 0), (32, 0), (8, 16)}, set((datum.block_num, datum.mix_block_num) for datum in data))