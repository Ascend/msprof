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

from msparser.ge.ge_logic_stream_info_bean import GeLogicStreamInfoBean

NAMESPACE = 'msmodel.ge.ge_logic_stream_model'


class TestGeLogicStreamInfoBean(unittest.TestCase):
    def test_construct_should_return_success_when_db_check_ok(self):
        args = [23130, 0, 0, 0, 0, 0, 1, 2, *(120,) * 52]
        bean = GeLogicStreamInfoBean(args)
        self.assertEqual(bean.logic_stream_id, 1)
        self.assertEqual(bean._physic_logic_stream_id, [[120, 1], [120, 1]])


if __name__ == '__main__':
    unittest.main()
