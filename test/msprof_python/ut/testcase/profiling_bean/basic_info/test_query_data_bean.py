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

from profiling_bean.basic_info.query_data_bean import QueryDataBean

NAMESPACE = 'profiling_bean.basic_info.query_data_bean'
args = {"job_info": 0, "device_id": 0,
        "job_name": "0", "collection_time": 0, "model_id": 0, "iteration_id": 0}


class TestQueryDataBean(unittest.TestCase):
    def test_init(self):
        test = QueryDataBean(**args)
        self.assertEqual(test._job_info, 0)

    def test_job_info(self):
        test = QueryDataBean(**args).job_info
        self.assertEqual(test, 0)

    def test_job_name(self):
        test = QueryDataBean(**args).job_name
        self.assertEqual(test, "0")

    def test_model_id(self):
        test = QueryDataBean(**args).model_id
        self.assertEqual(test, 0)

    def test_device_id(self):
        test = QueryDataBean(**args).device_id
        self.assertEqual(test, 0)

    def test_iteration_id(self):
        test = QueryDataBean(**args).iteration_id
        self.assertEqual(test, 0)

    def test_collection_time(self):
        test = QueryDataBean(**args).collection_time
        self.assertEqual(test, 0)


if __name__ == '__main__':
    unittest.main()
