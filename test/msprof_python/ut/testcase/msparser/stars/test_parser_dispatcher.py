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
import configparser
import unittest
from unittest import mock

from msparser.interface.istars_parser import IStarsParser
from msparser.stars.parser_dispatcher import ParserDispatcher

NAMESPACE = 'msparser.stars.parser_dispatcher'


class TestParserDispatcher(unittest.TestCase):
    def test_init(self):
        result_dir = 'ada.py'
        with mock.patch(NAMESPACE + '.ParserDispatcher.build_parser_map'):
            key = ParserDispatcher(result_dir)
            key.init()

    def test_dispatch_1(self):
        func_type = 'is'
        data = '123'
        with mock.patch('msparser.interface.istars_parser.IStarsParser.handle'):
            key = ParserDispatcher(result_dir='ada')
            key.parser_map = {'is': IStarsParser}
            key.dispatch(func_type, data)

    def test_dispatch_2(self):
        func_type = 'self'
        data = '111'
        with mock.patch(NAMESPACE + '.logging.error'):
            key = ParserDispatcher(result_dir='ada')
            key.dispatch(func_type, data)

    def test_flush_all_parser(self):
        with mock.patch('msparser.interface.istars_parser.IStarsParser.flush'):
            key = ParserDispatcher(result_dir='ada')
            key.parser_map = {'fftspmuparser': IStarsParser}
            key.flush_all_parser()

    def test_build_parser_map(self):
        with mock.patch('configparser.ConfigParser.sections', return_value=['AsqTaskParser']), \
                mock.patch('configparser.ConfigParser.get', return_value='1'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True):
            check = ParserDispatcher('test')
            check.cfg_parser = configparser.ConfigParser(interpolation=None)
            check.build_parser_map()


if __name__ == '__main__':
    unittest.main()

