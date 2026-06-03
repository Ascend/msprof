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

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException

NAMESPACE = 'common_func.info_conf_reader'


class TestInfoConfReader(unittest.TestCase):
    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}
        info_reader._sample_json = {}
        info_reader._host_freq = None
        info_reader._local_time_offset = 0
        info_reader._start_info = {}
        info_reader._dev_cnt = 0.0

    def test_time_from_host_syscnt_should_return_original_time_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().time_from_host_syscnt(15000000000), 15000000000.0)

    def test_time_from_host_syscnt_should_return_timestamp_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().time_from_host_syscnt(15000000000), 5000000000.0)

    def test_get_host_duration_should_return_original_duration_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().get_host_duration(6000), 6000.0)

    def test_get_host_duration_should_return_computed_duration_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_duration(6000), 2000.0)

    def test_syscnt_from_dev_time_should_return_timestamp_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().get_host_syscnt_from_dev_time(15000000000), 15000000000.0)

    def test_syscnt_from_dev_time_should_return_sys_count_duration_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_syscnt_from_dev_time(15000000000), 45000000000.0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_not_None(self):
        InfoConfReader()._host_freq = 1000000000
        self.assertEqual(InfoConfReader().get_host_freq(), 1000000000)

    def test_get_host_freq_should_return_error_when_no_freq(self):
        InfoConfReader()._info_json = {'CPU': []}
        with pytest.raises(ProfException) as error:
            InfoConfReader().get_host_freq()
            self.assertEqual(error.value, 0)

    def test_get_host_freq_should_return_error_when_data_format_is_invalid(self):
        InfoConfReader()._info_json = {'CPU': 1}
        with pytest.raises(ProfException) as error:
            InfoConfReader().get_host_freq()
            self.assertEqual(error.value, 0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_is_invalid(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "str"}]}
        self.assertEqual(InfoConfReader().get_host_freq(), 1000000000.0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_freq(), 3000000000.0)

    def test_trans_into_local_time_should_return_local_time_when_time_fmt_is_not_default(self):
        InfoConfReader()._local_time_offset = 3000.0
        self.assertEqual(InfoConfReader().trans_into_local_time(3000000000.0, use_us=True), '3000003000.000')

    def test_get_dev_cnt_should_return_nano_second(self):
        InfoConfReader()._dev_cnt = 3000.000
        self.assertEqual(InfoConfReader().get_dev_cnt(), 3000000000000)

    def test_duration_from_syscnt_should_return_duration_time_when_hwts_frequency_is_1000(self):
        InfoConfReader()._info_json = {'DeviceInfo': [{'hwts_frequency': "1000"}]}
        self.assertEqual(15000000.0, InfoConfReader().duration_from_syscnt(15000000000))

    def test_get_device_list_when_only_1_device_id(self):
        InfoConfReader()._sample_json = {'devices': "5"}
        self.assertEqual(['5'], InfoConfReader().get_device_list())
        InfoConfReader()._sample_json = {}

    def test_get_device_list_when_device_id_with_space(self):
        InfoConfReader()._sample_json = {'devices': "5 , 7"}
        self.assertEqual(['5 ', ' 7'], InfoConfReader().get_device_list())
        InfoConfReader()._sample_json = {}

    def test_get_device_list_when_more_than_2_device_id(self):
        InfoConfReader()._sample_json = {'devices': "3,5,7"}
        self.assertEqual(['3', '5', '7'], InfoConfReader().get_device_list())
        InfoConfReader()._sample_json = {}

    def test_get_device_list_when_devices_is_empty(self):
        InfoConfReader()._sample_json = {'devices': ""}
        with pytest.raises(ProfException) as err:
            InfoConfReader().get_device_list()
        InfoConfReader()._sample_json = {}

    def test_get_device_list_when_no_devices(self):
        InfoConfReader()._sample_json = {}
        with pytest.raises(ProfException) as err:
            InfoConfReader().get_device_list()
        InfoConfReader()._sample_json = {}

    def test_get_device_list_when_devices_is_invalid(self):
        InfoConfReader()._sample_json = {'devices': "5,abc"}
        with pytest.raises(ProfException) as err:
            InfoConfReader().get_device_list()

        InfoConfReader()._sample_json = {'devices': "0,²"}
        with pytest.raises(ProfException) as err:
            InfoConfReader().get_device_list()

        InfoConfReader()._sample_json = {}

    def test_get_data_under_device_when_devices_is_valid(self):
        InfoConfReader()._info_json = {'DeviceInfo': [{"test_type": "expected_res", }]}
        self.assertEqual("expected_res", InfoConfReader().get_data_under_device('test_type'))

        InfoConfReader()._info_json = {}

    def test_get_data_under_device_when_devices_is_invalid(self):
        InfoConfReader()._info_json = {'DeviceInfo': "str"}
        self.assertEqual("", InfoConfReader().get_data_under_device('test_type'))

        InfoConfReader()._info_json = {'DeviceInfo': []}
        self.assertEqual("", InfoConfReader().get_data_under_device('test_type'))

        InfoConfReader()._info_json = {}
        self.assertEqual("", InfoConfReader().get_data_under_device('test_type'))

    def test_get_cann_version_should_return_cann_version_when_cann_version_is_valid(self):
        InfoConfReader()._info_json = {"cannVersion": "9.1.0"}
        self.assertEqual((9, 1), InfoConfReader().get_cann_version())
        InfoConfReader()._info_json = {"cannVersion": "9.1.0-beta.0"}
        self.assertEqual((9, 1), InfoConfReader().get_cann_version())
        InfoConfReader()._info_json = {"cannVersion": "9.1.T106"}
        self.assertEqual((9, 1), InfoConfReader().get_cann_version())
        InfoConfReader()._info_json = {}

    def test_get_cann_version_should_return_0_when_cann_version_is_invalid(self):
        InfoConfReader()._info_json = {"cannVersion": "invalid_version"}
        self.assertEqual((0, 0), InfoConfReader().get_cann_version())
        InfoConfReader()._info_json = {}
        self.assertEqual((0, 0), InfoConfReader().get_cann_version())


if __name__ == '__main__':
    unittest.main()
