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
from msmodel.aic.aic_pmu_model import AicPmuModel
from msmodel.aic.aic_pmu_model import FftsV1PmuModel

NAMESPACE = 'msmodel.aic.aic_pmu_model'


class TestPcieModel(unittest.TestCase):

    def test_init(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.init'),\
                mock.patch(NAMESPACE + '.AicPmuModel.create_table'):
            check = AicPmuModel('test')
            check.init()

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.AicPmuModel.clear'),\
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.create_metric_table'):
            check = AicPmuModel('test')
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AicPmuModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AicPmuModel('test')
            check.flush([])

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AicPmuModel('test')
            check.clear()

    def test_ffts_v1_create_table_should_append_end_time_column(self):
        # FftsV1PmuModel(7/8/11 ffts非mix)在AIC表基础上于表尾追加end_time，供unified task-pmu读取wall-clock时间
        with mock.patch(NAMESPACE + '.AicPmuModel.create_table') as mock_base_create, \
                mock.patch(NAMESPACE + '.DBManager.execute_sql') as mock_execute:
            check = FftsV1PmuModel('test')
            check.create_table()
            mock_base_create.assert_called_once()
            alter_sql = mock_execute.call_args[0][1]
            self.assertIn('ALTER TABLE MetricSummary ADD COLUMN end_time INT', alter_sql)

