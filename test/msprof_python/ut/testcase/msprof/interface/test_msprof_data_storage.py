#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
import json
import os
import shutil
import tempfile
import unittest
from datetime import datetime
from datetime import timezone
from unittest import mock

from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.file_slice_helper import FileSliceHelper
from common_func.profiling_scene import ExportMode
from msinterface.msprof_data_storage import MsprofDataStorage

NAMESPACE = 'msinterface.msprof_data_storage'


def create_msprof_json_data(path: str):
    if not os.path.exists(path):
        os.makedirs(path)
    device_path = os.path.join(path, "device_0")
    if not os.path.exists(device_path):
        os.mkdir(device_path)
    output = os.path.join(path, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
    if not os.path.exists(output):
        os.mkdir(output)
    json_file = os.path.join(output, 'msprof_20250215160320.json')
    content = ('[{"name": "process_name", "pid": 1084473760, "tid": 0, "ph": "M", "args": {"name": "Ascend Hardware"}},'
               ' {"name": "process_labels", "pid": 1084473760, "tid": 0, "ph": "M", "args": {"labels": "NPU"}}, '
               '{"name": "process_sort_index", "pid": 1084473760, "tid": 0, "ph": "M", "args": {"sort_index": 13}},'
               ' {"name": "thread_name", "pid": 1084473760, "tid": 70001, "ph": "M", "args": {"name":'
               ' "Step Trace(Model ID:1)"}}, {"name": "thread_sort_index", "pid": 1084473760, "tid": 70001, "ph": "M",'
               ' "args": {"sort_index": 70001}}, {"name": "Reduce_1_0", "pid": 1084473760, "tid": 70001, "ts":'
               ' "1730943468310229.220", "dur": 2217.88, "ph": "X", "cat": "Reduce", "args": {"Iteration ID": 1,'
               ' "Reduce End 0": "1730943468312447100", "Reduce Start 0": "1730943468310229220"}}]')
    # 创建并打开文件，模式为 'w'（写入）
    with open(json_file, 'w') as file:
        # 写入内容到文件中
        file.write(content)


class TestMsprofDataStorage(unittest.TestCase):

    def test_slice_data_list(self):
        data_list = [
            {'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
            {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
            {
                'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'
            },
            {
                'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'
            }
        ]
        with mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)):
            key = MsprofDataStorage()
            result = key.slice_data_list(data_list)
        self.assertEqual(len(result[1][0]), 4)
        data_list = [
            {'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
            {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
            {
                'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'
            },
            {
                'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'
            }
        ]
        with mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.get_slice_times', return_value=2):
            key = MsprofDataStorage()
            result = key.slice_data_list(data_list)
        self.assertEqual(len(result[1][0]), 3)

    def test_export_summary_data_1(self):
        headers = 2
        data = [2]
        params = {"export_format": "csv"}
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name', return_value="test_01_1"), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch(NAMESPACE + '.create_csv', return_value=1):
            key = MsprofDataStorage()
            result = key.export_summary_data(headers, data, params)
        json_dump = '{"status": 1, "info": "bak or mkdir json dir failed", "data": ""}'
        self.assertEqual(result, json_dump)

    def test_export_summary_data_2(self):
        headers = 1
        data = [1]
        params = {"export_format": "json"}
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name', return_value="test_0111"), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch(NAMESPACE + '.create_json', return_value=1):
            key = MsprofDataStorage()
            result = key.export_summary_data(headers, data, params)
        self.assertEqual(result, 1)

    def test_export_summary_data_3(self):
        headers = None
        data = 2
        params = {"export_format": "1"}
        key = MsprofDataStorage()
        result = key.export_summary_data(headers, data, params)
        self.assertEqual(result, 2)

    def test_export_timeline_data_to_json_1(self):
        result = {'test1': 1, 'status': 2}
        params = {"data_type": 1}
        key = MsprofDataStorage()
        res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, json.dumps(result))

    def test_export_timeline_data_to_json_2(self):
        result = {'test1': 1}
        params = {"data_type": 1}
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name', return_value=1), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch('common_func.path_manager.PathManager.get_device_count', return_value=2), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        params = {
            "data_type": 'str', "device_id": 1, "model_id": 1, "iter_id": 1,
            "export_type": "summary", "export_format": "json", "project": 123
        }
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch('common_func.path_manager.PathManager.get_device_count', return_value=2), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            with mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value="train"), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
                key = MsprofDataStorage()
                result = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": 0, "data": [1]}')
        self.assertEqual(result, '{"status": 0, "data": [true]}')

    def test_export_timeline_data_should_return_directly_when_input_contains_status(self):
        result = json.loads('{"status": "123"}')
        params = {"data_type": 2}
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=False):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": "123"}')

    def test_export_timeline_data_to_json_4(self):
        result = None
        params = {"data_type": 3}
        key = MsprofDataStorage()
        res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": 2, "info": "Unable to get 3 data. Maybe the data is not collected, '
                              'or the data may fail to be analyzed."}')

    def test_read_slice_config(self):
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch('os.fdopen', mock.mock_open(
                    read_data='{"slice_switch": "on","slice_file_size": 0,"slice_method": 0}')):
            key = MsprofDataStorage()
            res = key.read_slice_config()
            self.assertEqual(res, ('off', 0, 0))

    def test_timeline_dir(self):
        with mock.patch(NAMESPACE + '.PathManager.get_timeline_dir', return_value='test'), \
                mock.patch('os.listdir', return_value=['acl_0_0_1.json']), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.remove'):
            MsprofDataStorage.clear_timeline_dir({'data_type': 'acl'})

    def test_get_slice_times1(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            key = MsprofDataStorage()
            key.data_list = [{'test': 'test'} for _ in range(10000000)]
            key.tid_set = {i for i in range(100)}
            key.get_slice_times('a', 1)

    def test_get_slice_times2(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            key = MsprofDataStorage()
            key.data_list = []
            key.tid_set = {i for i in range(10000000)}
            key.get_slice_times('a', 1)

    def test_slice_msprof_json_for_so_with_empty_path(self):
        MsprofDataStorage.slice_msprof_json_for_so('', dict())

    def test_slice_msprof_json_for_so_with_size_lower_than_200(self):
        with mock.patch('os.path.getsize', return_value=100), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)):
            MsprofDataStorage.slice_msprof_json_for_so("test", dict())

    def test_slice_msprof_json_for_so(self):
        tmp_dir = tempfile.mkdtemp()
        slice_dir = os.path.join(tmp_dir, "test_slice_for_so")
        params = {
            StrConstant.PARAM_RESULT_DIR: slice_dir,
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: os.path.join(slice_dir, 'mindstudio_profiler_output'),
            StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE,
            StrConstant.PARAM_DATA_TYPE: 'msprof'
        }
        create_msprof_json_data(slice_dir)
        trace_file = os.path.join(slice_dir, 'mindstudio_profiler_output/msprof_20250215160320.json')
        with mock.patch('os.path.getsize', return_value=300), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)):
            MsprofDataStorage.slice_msprof_json_for_so(trace_file, params)
        shutil.rmtree(tmp_dir)


if __name__ == '__main__':
    unittest.main()
