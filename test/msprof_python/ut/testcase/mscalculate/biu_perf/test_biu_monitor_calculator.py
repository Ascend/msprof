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

import functools
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.biu_perf.biu_monitor_calculator import BiuMonitorCalculator
from mscalculate.biu_perf.biu_monitor_calculator import MonitorCyclesCalculator
from mscalculate.biu_perf.biu_monitor_calculator import MonitorFlowCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.biu_perf.biu_monitor_calculator'


class TestBiuMonitorCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.BiuMonitorCalculator.calculate'), \
                mock.patch(NAMESPACE + '.BiuMonitorCalculator.save'):
            BiuMonitorCalculator(CONFIG).ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self):
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = BiuMonitorCalculator(CONFIG)
            check.calculate = mock.Mock()
            check.save = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()
            check.save.assert_not_called()


class TestMonitorFlowCalculator(unittest.TestCase):

    def test_calculate(self):
        class DataTest:
            stat_rcmd_num = 1
            stat_wcmd_num = 2
            stat_rlat_raw = 3
            stat_wlat_raw = 4
            stat_flux_rd = 5
            stat_flux_wr = 6
            stat_flux_rd_l2 = 7
            stat_flux_wr_l2 = 7
            timestamp = 7
            L2_cache_hit = 7
            core_id = 7
            group_id = 7
            core_type = 7

        with mock.patch(NAMESPACE + '.BiuPerfModel.get_all_data', return_value=[DataTest]):
            check = MonitorFlowCalculator(CONFIG)
            InfoConfReader().get_instr_profiling_freq = mock.Mock()
            InfoConfReader().get_instr_profiling_freq.return_value = 100
            check.calculate()

    def test_get_unit_name(self):
        check = MonitorFlowCalculator(CONFIG)
        self.assertEqual(check.get_unit_name(1, 1, '2'), 'biu_group1')

    def test_get_pid(self):
        check = MonitorFlowCalculator(CONFIG)
        self.assertEqual(check.get_pid(1, 1), 8)


class TestMonitorCyclesCalculator(unittest.TestCase):

    def test_calculate(self):
        class DataTest:
            vector_cycles = 1
            scalar_cycles = 2
            cube_cycles = 3
            lsu1_cycles = 4
            lsu2_cycles = 5
            lsu3_cycles = 6
            timestamp = 5
            core_id = 6
            group_id = 7
            core_type = 7

        with mock.patch(NAMESPACE + '.BiuPerfModel.get_all_data', return_value=[DataTest]):
            check = MonitorCyclesCalculator(CONFIG)
            InfoConfReader().get_instr_profiling_freq = mock.Mock()
            InfoConfReader().get_instr_profiling_freq.return_value = 100
            check.calculate()

    def test_get_unit_name(self):
        check = MonitorCyclesCalculator(CONFIG)
        self.assertEqual(check.get_unit_name(1, 1, '2'), '2_core1_group1')

    def test_get_pid(self):
        check = MonitorCyclesCalculator(CONFIG)
        self.assertEqual(check.get_pid(1, 1), 5)
        self.assertEqual(check.get_pid(26, 1), 7)
        self.assertEqual(check.get_pid(27, 1), 6)
        self.assertEqual(check.get_pid(-1, 1), 0)

    def test_save(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            MonitorCyclesCalculator(CONFIG).save()
        with mock.patch(NAMESPACE + '.logging.warning'),\
                mock.patch(NAMESPACE + '.BiuPerfModel.create_table'),\
                mock.patch(NAMESPACE + '.BiuPerfModel.flush'):
            check = MonitorCyclesCalculator(CONFIG)
            check.data = [123]
            check.save()


class TestGetInstrProfiling(unittest.TestCase):
    def test_get_instr_profiling_error(self):
        sample_json = {}
        self.assertRaises(Exception, functools.partial(
            InfoConfReader()._get_instr_profiling_frequency_from_sample, sample_json)
        )

    def test_get_instr_profiling_invalid_value(self):
        sample_json = {"instr_profiling_freq": -100}
        self.assertRaises(Exception, functools.partial(
            InfoConfReader()._get_instr_profiling_frequency_from_sample, sample_json)
        )

    def test_get_instr_profiling_camel(self):
        sample_json = {"instrProfilingFreq": 200}
        self.assertEqual(InfoConfReader()._get_instr_profiling_frequency_from_sample(sample_json), 200)

    def test_get_instr_profiling_snake(self):
        sample_json = {"instr_profiling_freq": 100}
        self.assertEqual(InfoConfReader()._get_instr_profiling_frequency_from_sample(sample_json), 100)


if __name__ == '__main__':
    unittest.main()
