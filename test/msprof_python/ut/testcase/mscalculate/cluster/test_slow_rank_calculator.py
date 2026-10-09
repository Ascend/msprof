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

from mscalculate.cluster.slow_rank_calculator import SlowRankCalculator

NAMESPACE = 'mscalculate.cluster.slow_rank_calculator'


class TestSlowRankCalculator(unittest.TestCase):
    data = [
        {0: {
                "Communication Time Info": {
                    "Elapse Time(ms)": 645.2788699951171,
                    "Transit Time(ms)": 9.83598,
                    "Wait Time(ms)": 635.4030099999997,
                    "Synchronization Time(ms)": 463.94366,
                    "Wait Time Ratio": 0.9848,
                    "Synchronization Time Ratio": 0.9792
                }
            },
        1: {
                "Communication Time Info": {
                    "Elapse Time(ms)": 155.65027001953126,
                    "Transit Time(ms)": 10.8518,
                    "Wait Time(ms)": 144.75454000000008,
                    "Synchronization Time(ms)": 0.00033,
                    "Wait Time Ratio": 0.9303,
                    "Synchronization Time Ratio": 0.0
                }
            }
        }
    ]
    op_name_info = ['AllReduce_1']
    op_info = {t: {} for t in op_name_info}

    def test_run_prof_not_synchronized(self):
        cal = SlowRankCalculator(self.data, self.op_name_info)
        cal.run()
        cal.add_suggestions(self.op_info)

    def test_run_prof_inconsistent(self):
        self.data[0][0]["Communication Time Info"]["Synchronization Time Ratio"] = 0
        cal = SlowRankCalculator(self.data, self.op_name_info)
        cal.run()
        cal.add_suggestions(self.op_info)

    def test_run_no_slow(self):
        op_info = {t: {} for t in self.op_name_info}
        self.data[0][0]["Communication Time Info"]["Wait Time Ratio"] = 0
        self.data[0][1]["Communication Time Info"]["Wait Time Ratio"] = 0
        cal = SlowRankCalculator(self.data, self.op_name_info)
        cal.run()
        cal.add_suggestions(op_info)
