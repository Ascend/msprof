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
import struct
import shutil
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.lpm_info.lpm_info_parser import LpmInfoConvParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag
from common_func.file_manager import FdOpen

NAMESPACE = 'msparser.lpm_info.lpm_info_parser'


class TestLpmInfoParser(unittest.TestCase):
    file_list = {
        DataTag.LPM_INFO: [
            'lpmFreqConv.data.0.slice_0',
            'lpmInfoConv.data.0.slice_0'
        ],
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "lpm_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setup_class(cls):
        if not os.path.exists(cls.DIR_PATH):
            os.mkdir(cls.DIR_PATH)
        if not os.path.exists(cls.SQLITE_PATH):
            os.mkdir(cls.SQLITE_PATH)
        if not os.path.exists(cls.DATA_PATH):
            os.mkdir(cls.DATA_PATH)
            cls.make_lpm_freq_data()
            cls.make_lpm_conv_data()

    @classmethod
    def teardown_class(cls):
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_lpm_freq_data(cls):
        lpm_freq_data = (1, 0, 1097458820500, 800, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        struct_data = struct.pack(StructFmt.LPM_INFO_FMT, *lpm_freq_data)
        # 异常场景，新1-2包 老3-8包混装
        lpm_freq_data_with_aic_voltage = (1, 1, 1097458823663, 725, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        struct_data += struct.pack(StructFmt.LPM_INFO_FMT, *lpm_freq_data_with_aic_voltage)
        lpm_freq_data_with_bus_voltage = (1, 2, 1097458832744, 850, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        struct_data += struct.pack(StructFmt.LPM_INFO_FMT, *lpm_freq_data_with_bus_voltage)
        with FdOpen(os.path.join(cls.DATA_PATH, "lpmFreqConv.data.0.slice_0"), operate="wb") as f:
            f.write(struct_data)

    @classmethod
    def make_lpm_conv_data(cls):
        lpm_conv_data_0 = (
            (5, 0, 11380067259, 1850, 0, 11380601625, 800, 0, 11381162882, 1850, 0, 11438979204, 800, 0, 53996835776,
             1850, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        struct_data = struct.pack(StructFmt.LPM_INFO_FMT, *lpm_conv_data_0)
        lpm_conv_data_1 = (
            5, 1, 11380070430, 900, 0, 11380604804, 900, 0, 11381166064, 900, 0, 11438982395, 900, 0, 53996838910, 900,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0
        )
        struct_data += struct.pack(StructFmt.LPM_INFO_FMT, *lpm_conv_data_1)
        lpm_conv_data_2 = (
            3, 2, 11380114514, 850, 0, 11381919810, 900, 0, 53996838910, 900, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data += struct.pack(StructFmt.LPM_INFO_FMT, *lpm_conv_data_2)
        with FdOpen(os.path.join(cls.DATA_PATH, "lpmInfoConv.data.0.slice_0"), operate="wb") as f:
            f.write(struct_data)

    def init_data(self):
        if os.path.exists(self.DATA_PATH):
            shutil.rmtree(self.DATA_PATH)
        if not os.path.exists(self.DATA_PATH):
            os.mkdir(self.DATA_PATH)
            self.make_lpm_conv_data()
            self.make_lpm_freq_data()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.LpmInfoConvParser.parse'), \
                mock.patch(NAMESPACE + '.LpmInfoConvParser.save'):
            check = LpmInfoConvParser(self.file_list, self.CONFIG)
            check.ms_run()

    def test_save(self):
        check = LpmInfoConvParser(self.file_list, self.CONFIG)
        check.save()
        check = LpmInfoConvParser(self.file_list, self.CONFIG)
        check._lpm_conv_data.update(
            {
                0: [[1097624673997, 800], [1097627160884, 1800]],
                1: [[1097624673997, 725], [1097627164040, 785]],
                2: [[1097624673997, 850], [1097627138049, 930]]
            })
        check.save()

    def test_parse_should_return_freq_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        check = LpmInfoConvParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        freq_data = check._lpm_conv_data.get(0)
        self.assertEqual(7, len(freq_data))
        self.assertEqual([0, 100], freq_data[0])
        InfoConfReader()._info_json = {}
        self.init_data()

    def test_parse_should_return_aic_voltage_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        check = LpmInfoConvParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        aic_voltage_data = check._lpm_conv_data.get(1)
        self.assertEqual(7, len(aic_voltage_data))
        self.assertEqual([0, 900], aic_voltage_data[0])
        InfoConfReader()._info_json = {}
        self.init_data()

    def test_parse_should_return_bus_voltage_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        check = LpmInfoConvParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        aic_voltage_data = check._lpm_conv_data.get(1)
        self.assertEqual(7, len(aic_voltage_data))
        self.assertEqual([0, 900], aic_voltage_data[0])
        InfoConfReader()._info_json = {}
        self.init_data()

if __name__ == '__main__':
    unittest.main()