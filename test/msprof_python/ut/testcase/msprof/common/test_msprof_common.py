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
import os
import shutil
import unittest
from unittest import mock
from unittest.mock import Mock

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import add_all_file_complete
from common_func.msprof_common import analyze_collect_data
from common_func.msprof_common import check_collection_dir
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import check_path_char_valid
from common_func.msprof_common import get_info_by_key
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import get_valid_sub_path
from common_func.msprof_common import prepare_for_parse
from common_func.msprof_exception import ProfException
from constant.constant import INFO_JSON
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager

NAMESPACE = 'common_func.msprof_common'


def test_check_path_valid_error():
    with mock.patch(NAMESPACE + '.error'), \
            pytest.raises(ProfException) as err:
        path = ""
        is_output = True
        check_path_valid(path, is_output)
    unittest.TestCase().assertEqual(err.value.args[0], 1)


def test_check_path_valid_1():
    path = "home\\common"
    is_output = 1
    with mock.patch('os.path.exists', return_value=False):
        with mock.patch('os.makedirs'), \
                mock.patch('os.chmod'), \
                mock.patch(NAMESPACE + '.check_dir_writable'):
            check_path_valid(path, is_output)
        with mock.patch('os.makedirs', side_effect=OSError), \
                mock.patch(NAMESPACE + '.error'), \
                pytest.raises(ProfException) as err:
            check_path_valid(path, is_output)
    unittest.TestCase().assertEqual(err.value.args[0], 2)


def test_prepare_for_parse():
    output_path = 'home\\output'
    with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value='home\\output'), \
            mock.patch(NAMESPACE + '.PathManager.get_log_dir', return_value='home\\output'), \
            mock.patch(NAMESPACE + '.check_path_valid'), \
            mock.patch(NAMESPACE + '.PathManager.get_collection_log_path', return_value=1), \
            mock.patch(NAMESPACE + '.check_file_writable'), \
            mock.patch(NAMESPACE + '.logging.basicConfig'):
        prepare_for_parse(output_path)


def test_analyze_collect_data():
    collect_path = './'
    sample_config = {"result_dir": './', "tag_id": '123', "host_id": '127.0.0.1'}
    with mock.patch('os.path.exists', return_value=True):
        with mock.patch('os.listdir', return_value=["1"]), \
                mock.patch(NAMESPACE + '.prepare_for_parse'), \
                mock.patch(NAMESPACE + '.print_info'), \
                mock.patch(NAMESPACE + '.check_collection_dir'), \
                mock.patch('os.path.basename', return_value='123'), \
                mock.patch(NAMESPACE + '.update_sample_json'), \
                mock.patch(NAMESPACE + '.AI.project_preparation'), \
                mock.patch(NAMESPACE + '.AI.import_control_flow'), \
                mock.patch(NAMESPACE + '.FileDispatch.dispatch_parser'), \
                mock.patch(NAMESPACE + '.add_all_file_complete'):
            analyze_collect_data(collect_path, sample_config)


def test_get_info_by_key():
    path = 'home\\key'
    key = 1
    with mock.patch(NAMESPACE + '.check_dir_readable'), \
            mock.patch('os.listdir', return_value=('info.json.0',)):
        with mock.patch(NAMESPACE + '.re.match', return_value=True):
            InfoConfReader()._info_json = INFO_JSON
            InfoConfReader.INFO_PATTERN = r"^info\.json\.?(\d?)$"
            result = get_info_by_key(path, key)
        unittest.TestCase().assertEqual(result, None)
        with mock.patch(NAMESPACE + '.re.match', return_value=False):
            InfoConfReader.INFO_PATTERN = r"^info\.json\.?(\d?)$"
            get_info_by_key(path, key)


def test_add_all_file_complete():
    collect_path = './'
    add_all_file_complete(collect_path)
    os.mkdir('data', 0o640)
    with mock.patch('os.makedirs', side_effect=OSError), \
            mock.patch(NAMESPACE + '.error'):
        add_all_file_complete(collect_path)
    add_all_file_complete(collect_path)
    shutil.rmtree('data')


def test_get_path_dir():
    path = 'home\\dir'
    with mock.patch('os.path.join', return_value='home\\dir'), \
            mock.patch('os.path.realpath', return_value='home\\dir_path'), \
            mock.patch('os.path.isdir', return_value=True), \
            mock.patch('os.listdir', return_value=[".profilerv", "HCCL_PROF"]):
        result = get_path_dir(path)
        unittest.TestCase().assertEqual(result, ['.profilerv'])
    with mock.patch('os.path.join', return_value='home\\dir'), \
            mock.patch('os.path.realpath', return_value='home\\dir_path'), \
            mock.patch('os.path.isdir', return_value=True), \
            mock.patch('os.listdir', return_value=[".profiler", "HCCL_PROF"]), \
            mock.patch(NAMESPACE + '.error'), \
            pytest.raises(ProfException) as err:
        get_path_dir(path)
        unittest.TestCase().assertEqual(err.value.args, (2,))


def test_get_path_dir_should_skip_invalid_child_when_scan_fails():
    with mock.patch('os.listdir', side_effect=PermissionError), \
            mock.patch(NAMESPACE + '.warn') as mock_warn:
        unittest.TestCase().assertEqual(get_path_dir('invalid', skip_invalid=True), [])
        mock_warn.assert_called_once()


def test_get_path_dir_should_raise_path_error_when_root_scan_fails():
    with mock.patch('os.listdir', side_effect=PermissionError), \
            pytest.raises(ProfException) as err:
        get_path_dir('invalid')
    unittest.TestCase().assertEqual(err.value.code, ProfException.PROF_INVALID_PATH_ERROR)


def test_get_valid_sub_path_should_return_real_path_when_collect_path_and_sub_dir_exist():
    collect_path = '/host/host'
    sub_dir = 'msprof_export_folder'
    expected_path = '/host/host/msprof_export_folder'
    is_file = False
    with mock.patch('os.path.exists', return_value=True), \
         mock.patch('os.access', return_value=True), \
         mock.patch('os.path.isdir', return_value=True), \
         mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=0)):
            unittest.TestCase().assertEqual(get_valid_sub_path(collect_path, sub_dir, is_file),
                                        os.path.realpath(expected_path))


def test_get_valid_sub_path_should_return_equal_real_path_when_collect_path_and_sub_dir_exist():
    collect_path = '/host/collect_path'
    sub_dir = '/collect_path_folder'
    joined_path = os.path.join(collect_path, sub_dir)
    is_file = False
    with mock.patch('os.access', return_value=True), \
            mock.patch('os.path.exists', return_value=True), \
            mock.patch('os.path.isdir', return_value=True), \
            mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=0)):
        unittest.TestCase().assertEqual(get_valid_sub_path(collect_path, sub_dir, is_file),
                                        os.path.realpath(joined_path))


def test_get_valid_sub_path_should_skip_invalid_child_when_validation_fails():
    with mock.patch(NAMESPACE + '.check_path_valid',
                    side_effect=ProfException(ProfException.PROF_INVALID_PATH_ERROR)), \
            mock.patch(NAMESPACE + '.warn') as mock_warn:
        unittest.TestCase().assertEqual(get_valid_sub_path('root', 'invalid', False, skip_invalid=True), '')
        mock_warn.assert_called_once()


def test_check_collection_dir():
    collect_path = './'
    InfoJsonReaderManager(InfoJson(version='1.0')).process()
    with mock.patch('os.path.exists', return_value=False), \
            mock.patch(NAMESPACE + '.error'),\
            pytest.raises(ProfException) as err:
        check_collection_dir(collect_path)
    unittest.TestCase().assertEqual(err.value.code, 4)
    with mock.patch('os.path.exists', return_value=True), \
            mock.patch(NAMESPACE + '.check_dir_writable'), \
            mock.patch('common_func.common.get_data_file_memory', return_value=100), \
            mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value=["1"]):
        with mock.patch('os.listdir', return_value=False), \
                mock.patch(NAMESPACE + '.error'):
            ret = check_collection_dir(collect_path)
            unittest.TestCase().assertFalse(ret)


def test_check_path_char_valid_should_throw_exception_when_path_contains_escape_character():
    with pytest.raises(ProfException) as err:
        path = "/test/test_path\n11/"
        check_path_char_valid(path)
    unittest.TestCase().assertEqual("The path contains invalid character: '\\n'.", err.value.message)


if __name__ == '__main__':
    unittest.main()
