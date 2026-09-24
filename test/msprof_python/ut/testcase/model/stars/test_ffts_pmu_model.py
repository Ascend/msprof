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

from constant.ut_db_name_constant import DB_FFTS_PMU
from constant.ut_db_name_constant import TABLE_FFTS_PMU
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from sqlite.db_manager import DBOpen
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'msmodel.stars.ffts_pmu_model'


class TestFftsPmuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                        return_value=
                        ['total_time(ms)', 'total_cycles', 'vec_time', 'vec_ratio', 'mac_time', 'mac_ratio',
                         'scalar_time', 'scalar_ratio', 'mte1_time', 'mte1_ratio', 'mte2_time', 'mte2_ratio',
                         'mte3_time', 'mte3_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.FftsPmuModel._create_metric_table_by_head'):
            check = FftsPmuModel('test', 'test', ['test'])
            check.create_table()

    def test_create_metric_table(self):
        with DBOpen(DB_FFTS_PMU) as db_open:
            metrics = ['(ms)test', '(ms)abc']
            check = FftsPmuModel('test', 'test', ['test'])
            check.conn, check.cur = db_open.db_conn, db_open.db_curs
            check._create_metric_table_by_head(metrics, TABLE_FFTS_PMU)
            with mock.patch(NAMESPACE + '.logging.error'):
                check._create_metric_table_by_head(metrics, TABLE_FFTS_PMU)

    def test_update_pmu_list(self):
        data_1 = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        data_2 = [1, 2, 3, 4, 5, 6, 7, 10000, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 9, 8, 7]
        pmu_data_list = [FftsPmuBean(data_1), FftsPmuBean(data_2)]
        check = FftsPmuModel('test', 'test', ['test'])
        check.profiling_events = {
            'aic': ['total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio', 'mte1_ratio',
                    'mte2_ratio', 'mte3_ratio', 'icache_miss_rate'], 'aiv': []}
        check.update_pmu_list(pmu_data_list)

    def test_flush(self):
        data = b'\00' * 128
        with mock.patch(NAMESPACE + '.FftsPmuModel.insert_data_to_db'):
            check = FftsPmuModel('test', 'test', ['test'])
            InfoJson(DeviceInfo=[
                DeviceInfo(hwts_frequency='50', aic_frequency='1000', aiv_frequency='1800').device_info]).apply()
            check.flush([])
