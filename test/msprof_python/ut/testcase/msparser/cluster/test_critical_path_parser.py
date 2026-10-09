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
from unittest import mock

from msparser.cluster.critical_path_parser import CriticalPathParser

NAMESPACE = 'msparser.cluster.critical_path_parser'


class TestCriticalPathParser(unittest.TestCase):
    def test_run(self):
        test_events = [
            {"op_name": "MatMul", "task_type": "AI_CORE", "tid": 2, "ts": 1, "es": 3, "dur": 2.},
            {"op_name": "allReduce", "task_type": "COMMUNICATION", "tid": 8, "ts": 4, "es": 5, "dur": 1}
        ]
        with mock.patch(NAMESPACE + '.CriticalPathParser.parse_compute_ops'), \
                mock.patch(NAMESPACE + '.CriticalPathParser.parse_hccl_ops'):
            critical_path_parser = CriticalPathParser([], {})
            critical_path_parser.events = test_events
            top_op = critical_path_parser.run()
            self.assertEqual(top_op, ('allReduce',))
