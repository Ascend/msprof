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

from profiling_bean.db_dto.msproftx_dto import MsprofTxDto, MsprofTxExDto


class TestMsprofTxDto(unittest.TestCase):
    def test_init(self: any) -> None:
        msproftx_dto = MsprofTxDto()

        msproftx_dto.pid = 0
        msproftx_dto.tid = 0
        msproftx_dto.category = 0
        msproftx_dto.event_type = 0
        msproftx_dto.payload_type = 0
        msproftx_dto.start_time = 0
        msproftx_dto.end_time = 0
        msproftx_dto.message_type = 0
        msproftx_dto.message = 0
        msproftx_dto.dur_time = 0

        self.assertEqual((msproftx_dto.pid,
              msproftx_dto.tid,
              msproftx_dto.category,
              msproftx_dto.event_type,
              msproftx_dto.payload_type,
              msproftx_dto.start_time,
              msproftx_dto.end_time,
              msproftx_dto.message_type,
              msproftx_dto.message,
              msproftx_dto.dur_time),
                         (0, 0, 0, 0, 0, 0, 0, 0, 0, 0))


class TestMsprofTxExDto(unittest.TestCase):
    def test_init(self: any) -> None:
        msproftx_ex_dto = MsprofTxExDto()
        msproftx_ex_dto.pid = 0
        msproftx_ex_dto.tid = 0
        msproftx_ex_dto.event_type = 'marker_ex'
        msproftx_ex_dto.start_time = 0
        msproftx_ex_dto.dur_time = 0
        msproftx_ex_dto.mark_id = 0
        msproftx_ex_dto.message = 'test'

        self.assertEqual((msproftx_ex_dto.pid,
                          msproftx_ex_dto.tid,
                          msproftx_ex_dto.event_type,
                          msproftx_ex_dto.start_time,
                          msproftx_ex_dto.message,
                          msproftx_ex_dto.mark_id,
                          msproftx_ex_dto.dur_time),
                         (0, 0, 'marker_ex', 0, 'test', 0, 0))

