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
function: test l2_cache_metric
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from mscalculate.l2_cache.l2_cache_metric import HitRateMetric

NAMESPACE = 'mscalculate.l2_cache.l2_cache_metric'

class TestHitRateMetric(unittest.TestCase):
    def test_calculate_hit_rate(self: any):
        hit_rate_metric = HitRateMetric(10, 35)
        self.assertEqual(hit_rate_metric._calculate_hit_rate(), 0.285714)
        hit_rate_metric = HitRateMetric(10, 0)
        self.assertEqual(hit_rate_metric._calculate_hit_rate(), 0)

class TestVictimRateMetric(unittest.TestCase):
    def test_calculate_victim_rate(self: any):
        victim_rate_metric = HitRateMetric(10, 35)
        self.assertEqual(victim_rate_metric._calculate_hit_rate(), 0.285714)
        victim_rate_metric = HitRateMetric(10, 0)
        self.assertEqual(victim_rate_metric._calculate_hit_rate(), 0)