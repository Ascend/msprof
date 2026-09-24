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

from profiling_bean.stars.stars_common import StarsCommon
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'profiling_bean.struct_info.stars_common'


class TestStarsCommon(unittest.TestCase):

    def test_init(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check._stream_id, 2)

    def test_task_id_when_stream_id_flag_is_false(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        ret = check.task_id
        self.assertEqual(ret, 1)

    def test_task_id_when_stream_id_flag_is_true_and_task_id_no_changed(self):
        task_id, stream_id, timestamp = 1, 4100, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        ret = check.task_id
        self.assertEqual(ret, 1)

    def test_task_id_when_stream_id_flag_is_true_and_task_id_is_changed(self):
        task_id, stream_id, timestamp = 1, 53248, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        ret = check.task_id
        self.assertEqual(ret, 49153)

    def test_stream_id(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.stream_id, 2)

    def test_timestamp(self):
        InfoJson(DeviceInfo=[
            DeviceInfo(hwts_frequency='50', aic_frequency='1500', aiv_frequency='1500').device_info]).apply()
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.timestamp, 59.99999999999999)


if __name__ == '__main__':
    unittest.main()
