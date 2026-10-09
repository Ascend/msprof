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

from common_func.msprof_exception import ProfException
from common_func.ms_multi_process import MsMultiProcess


NAMESPACE = 'common_func.ms_multi_process'
CONFIG = {'result_dir': 'output'}


class TestMsMultiProcess(unittest.TestCase):

    def test_run_raise_profexception(self) -> None:
        for exception in [ProfException(13, "data exceed limit"), ProfException(2), RuntimeError()]:
            with mock.patch(NAMESPACE + '.MsMultiProcess.process_init'), \
                    mock.patch(NAMESPACE + '.MsMultiProcess.ms_run', side_effect=exception):
                test = MsMultiProcess(CONFIG)
                test.run()


if __name__ == '__main__':
    unittest.main()
