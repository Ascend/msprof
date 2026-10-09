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
from argparse import Namespace
from unittest import mock

from interface.get_msprof_info import MsprofInfoConstruct

NAMESPACE = 'interface.get_msprof_info'


class TestMsProfBasicInfo(unittest.TestCase):

    def test_main(self):
        args = ["get_msprof_info.py", "-dir", "test"]
        collection_path = {"collection_path": "test"}
        args_co = Namespace(**collection_path)
        with mock.patch('sys.argv', args), \
             mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
             mock.patch(NAMESPACE + '.MsprofInfoConstruct.load_basic_info_model'), \
             mock.patch('sys.exit'):
            check = MsprofInfoConstruct()
            result = check.main()
        self.assertEqual(result, None)

        args = ["get_msprof_info.py"]
        with mock.patch('sys.argv', args), \
             mock.patch('argparse.ArgumentParser.print_help'),\
             mock.patch('sys.exit'):
            check = MsprofInfoConstruct()
            result = check.main()
            self.assertEqual(result, None)


def test_main_1():
    args = ["msprof.py", "import" "-dir", "test"]
    collection_path = {"collection_path": "test"}
    args_co = Namespace(**collection_path)
    with mock.patch('sys.argv', args), \
         mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
         mock.patch('os.listdir', return_value=['start_info.0']), \
         mock.patch('profiling_bean.basic_info.msprof_basic_info.MsProfBasicInfo.init'), \
         mock.patch('profiling_bean.basic_info.msprof_basic_info.MsProfBasicInfo.run', return_value="{}"):
        MsprofInfoConstruct().main()

    args = ['msprof.py']
    with mock.patch('sys.argv', args):
        MsprofInfoConstruct().main()
