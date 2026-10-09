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
import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project
from msparser.cluster.host_sys_usage_parser import HostSysUsageParser

NAMESPACE = 'msparser.cluster.host_sys_usage_parser'


class TestHostSysUsageParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HostSysUsageParser')
    params = {"collection_path": DIR_PATH, "npu_id": 0, "model_id": 0}

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        os.makedirs(os.path.join(self.DIR_PATH, 'query'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_process_should_generate_json_file(self):
        common_info = {
            'pid': 1,
            'cpu_nums': 192,
            'cpu_sampling_interval': 100,
            'mem_sampling_interval': 100
        }
        sys_cpu_data = [[100, 100, 100, 100, 123456789], [100, 100, 100, 100, 123458789]]
        pid_cpu_data = [[100, 100, 123456789], [100, 100, 123458789]]
        sys_mem_data = [[1024, 512, 123456789], [1024, 512, 123458789]]
        pid_mem_data = [[1, 2, 3, 123456789], [1, 2, 3, 123458789]]
        with mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_dir_path', return_value=self.DIR_PATH), \
            mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_common_info', return_value=common_info), \
            mock.patch(NAMESPACE + '.HostSysUsageParser._get_all_pid', return_value=([123], [456])), \
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.init', return_value=True), \
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.init', return_value=True), \
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.get_sys_cpu_data', return_value=sys_cpu_data), \
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.get_pid_cpu_data', return_value=pid_cpu_data),\
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.get_sys_mem_data', return_value=sys_mem_data),\
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.get_pid_mem_data', return_value=pid_mem_data), \
             mock.patch('common_func.file_manager.check_path_valid'):
            check = HostSysUsageParser(self.params)
            check.process()

            sys_cpu_data_file = os.path.join(self.DIR_PATH, "query", "host_sys_cpu_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(True, os.path.exists(sys_cpu_data_file))
            pid_cpu_data_file = os.path.join(self.DIR_PATH, "query", "host_pid_cpu_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(True, os.path.exists(pid_cpu_data_file))
            sys_mem_data_file = os.path.join(self.DIR_PATH, "query", "host_sys_mem_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(True, os.path.exists(sys_mem_data_file))
            pid_mem_data_file = os.path.join(self.DIR_PATH, "query", "host_pid_mem_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(True, os.path.exists(pid_mem_data_file))

    def test_process_should_not_generate_json_file_when_zeros_data(self):
        common_info = {
            'pid': 1,
            'cpu_nums': 192,
            'cpu_sampling_interval': 100,
            'mem_sampling_interval': 100
        }
        sys_cpu_data = [[100, 100, 100, 100, 123456789], [0, 0, 0, 0, 123458789]]
        pid_cpu_data = [[0, 0, 123456789], [100, 100, 123458789]]
        sys_mem_data = [[0, 512, 123456789], [1024, 512, 123458789]]
        pid_mem_data = [[1, 2, 3, 123456789], [1, 2, 3, 123458789]]
        with mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_dir_path', return_value=self.DIR_PATH), \
            mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_common_info', return_value=common_info), \
            mock.patch(NAMESPACE + '.HostSysUsageParser._get_all_pid', return_value=([123], [456])), \
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.init', return_value=True), \
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.init', return_value=True), \
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.get_sys_cpu_data', return_value=sys_cpu_data),\
            mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.get_pid_cpu_data', return_value=pid_cpu_data),\
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.get_sys_mem_data', return_value=sys_mem_data),\
            mock.patch('msmodel.hardware.sys_mem_model.SysMemModel.get_pid_mem_data', return_value=pid_mem_data), \
             mock.patch('common_func.file_manager.check_path_valid'):
            check = HostSysUsageParser(self.params)
            check.process()

            sys_cpu_data_file = os.path.join(self.DIR_PATH, "query", "host_sys_cpu_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(False, os.path.exists(sys_cpu_data_file))
            pid_cpu_data_file = os.path.join(self.DIR_PATH, "query", "host_pid_cpu_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(False, os.path.exists(pid_cpu_data_file))
            sys_mem_data_file = os.path.join(self.DIR_PATH, "query", "host_sys_mem_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(False, os.path.exists(sys_mem_data_file))
            pid_mem_data_file = os.path.join(self.DIR_PATH, "query", "host_pid_mem_usage_{}_{}.json".format(
                self.params["npu_id"], self.params["model_id"]))
            self.assertEqual(True, os.path.exists(pid_mem_data_file))

    def test_process_should_not_generate_json_file_when_host_common_info_not_exist(self):
        with mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_dir_path', return_value=""):
            check = HostSysUsageParser(self.params)
            check.process()
        with mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_dir_path', return_value=self.DIR_PATH), \
                mock.patch(NAMESPACE + '.HostSysUsageParser._get_host_common_info', return_value=None):
            check.process()

        npu_id = "npu_id"
        model_id = "model_id"
        query = "query"
        sys_cpu_data_file = os.path.join(self.DIR_PATH, query, "host_sys_cpu_usage_{}_{}.json".format(
            self.params.get(npu_id, ""), self.params.get(model_id, "")))
        self.assertEqual(False, os.path.exists(sys_cpu_data_file))
        pid_cpu_data_file = os.path.join(self.DIR_PATH, query, "host_pid_cpu_usage_{}_{}.json".format(
            self.params.get(npu_id, ""), self.params.get(model_id, "")))
        self.assertEqual(False, os.path.exists(pid_cpu_data_file))
        sys_mem_data_file = os.path.join(self.DIR_PATH, query, "host_sys_mem_usage_{}_{}.json".format(
            self.params.get(npu_id, ""), self.params.get(model_id, "")))
        self.assertEqual(False, os.path.exists(sys_mem_data_file))
        pid_mem_data_file = os.path.join(self.DIR_PATH, query, "host_pid_mem_usage_{}_{}.json".format(
            self.params.get(npu_id, ""), self.params.get(model_id, "")))
        self.assertEqual(False, os.path.exists(pid_mem_data_file))


if __name__ == '__main__':
    unittest.main()
