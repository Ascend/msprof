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
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest

from profiling_bean.db_dto.collective_communication_dto import CollectiveCommunicationDto


class TestCollectiveCommunicationDto(unittest.TestCase):
    def test_collective_communication_dto(self):
        check = CollectiveCommunicationDto()
        check.rank_id = 1
        check.compute_time = 123
        check.communication_time = 124
        check.stage_time = 125
        self.assertEqual([1, 123, 124, 125],
                         [check.rank_id, check.compute_time, check.communication_time, check.stage_time])
