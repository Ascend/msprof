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

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from common_func.platform.chip_manager import ChipManager
from common_func.constant import Constant
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager
from viewer.get_l2_cache_data import get_l2_cache_data, modify_l2_cache_headers, add_op_name, process_hit_rate

NAMESPACE = 'viewer.get_l2_cache_data'
columns = ['task_id', 'stream_id', 'task_type', 'hit_rate', 'victim_rate']


class TestL2Cache(unittest.TestCase):

    def test_get_l2_cache_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_l2_cache_data('', "L2CacheSummary", columns)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_l2_cache_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS L2CacheSummary" \
                     "(modelname, model_id, replayid, load_start, load_end, fusion_name, fusion_op_nums, " \
                     "op_names, stream_id, memory_input, memory_output, memory_weight, memory_workspace, " \
                     "memory_total, task_num, task_ids)"
        data = (("resnet50", 1, 0, 118433101720, 118540073322, "conv1conv1_relu", 4,
                 "scale_conv1,bn_conv1,conv1,conv1_relu", 5, 1706208, 1605664, 100544, 0, 3412416, 1, 4),)
        insert_model_sql = "insert into {0} values ({value})".format(
            "L2cacheSummary", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_L2CACHE, create_sql, insert_model_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.get_filtered_table_headers', return_value=columns), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.modify_l2_cache_headers'):
            res = get_l2_cache_data('', "L2CacheSummary", columns)
        self.assertEqual(res[2], 0)
        db_manager.destroy(test_sql)

    def test_modify_l2_cache_headers(self):
        headers = ["r08", "r0_a", "r09", "0xb", "0xc", "0xd", "0x54", "0x55"]
        modify_l2_cache_headers(headers)

    def test_add_op_name_1(self):
        l2_cache_header = ["Task Id", "Stream Id"]
        l2_cache_data = [[1, 2], [2, 3]]
        op_dict = {'1-2': 1,
                   '2-3': 2}
        res = add_op_name(l2_cache_header, l2_cache_data, op_dict)
        self.assertEqual(res, True)

    def test_add_op_name_2(self):
        l2_cache_header = {"Task": 0, "Stream": 1}
        l2_cache_data = [[1, 2], [2, 3]]
        op_dict = {}
        res = add_op_name(l2_cache_header, l2_cache_data, op_dict)
        self.assertEqual(res, False)

    def test_process_hit_rate_when_chip_si_not_stars_then_return_origin_l2_data(self):
        l2_cache_header = ["Stream Id", "Task Id", "Hit Rate", "Victim Rate"]
        l2_cache_data = [("", 2, 0.9, 0.8), (2, 3, 1.1, 0.7)]
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        ret = process_hit_rate(l2_cache_header, l2_cache_data)
        self.assertEqual(ret, l2_cache_data)

    def test_process_hit_rate_when_chip_is_chip_v6_1_0_then_return_origin_l2_data(self):
        l2_cache_header = ["Stream Id", "Task Id", "Hit Rate", "Victim Rate"]
        l2_cache_data = [(1, 2, Constant.NA, 0.8), (2, 3, Constant.NA, 0.7)]
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        ret = process_hit_rate(l2_cache_header, l2_cache_data)
        self.assertEqual(ret, l2_cache_data)


    def test_process_hit_rate_when_chip_si_not_stars_then_refresh_hit_rate(self):
        l2_cache_header = ["Stream Id", "Task Id", "Hit Rate", "Victim Rate"]
        l2_cache_data = [(1, 2, 0.9, 0.8), (2, 3, 1.1, 0.7)]
        ChipManager().chip_id = ChipModel.CHIP_V1_1_1
        ret = process_hit_rate(l2_cache_header, l2_cache_data)
        self.assertEqual(ret, [(1, 2, "N/A", 0.8), (2, 3, "N/A", 0.7)])


if __name__ == '__main__':
    unittest.main()
