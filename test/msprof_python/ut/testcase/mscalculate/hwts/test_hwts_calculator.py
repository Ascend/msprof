#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.hwts.hwts_calculator import HwtsCalculator
from msmodel.interface.base_model import BaseModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.hwts_log import HwtsLogBean

NAMESPACE = 'mscalculate.hwts.hwts_calculator'


class TestHwtsCalculator(unittest.TestCase):
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0']}

    def test_ms_run(self):
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.HwtsCalculator.calculate'), \
                mock.patch(NAMESPACE + '.HwtsCalculator.save'):
            check = HwtsCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_calculate_when_parse_all_file_then_parse(self):
        with mock.patch(NAMESPACE + '.HwtsCalculator.is_need_parse_all_file', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=False):
            check = HwtsCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_called_once()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        with mock.patch(NAMESPACE + '.HwtsCalculator.is_need_parse_all_file', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = HwtsCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_calculate_when_parse_by_iter_then_parse(self):
        with mock.patch(NAMESPACE + '.HwtsCalculator.is_need_parse_all_file', return_value=False):
            check = HwtsCalculator(self.file_list, CONFIG)
            check._parse_by_iter = mock.Mock()
            check.calculate()
            check._parse_by_iter.assert_called_once()

    def test_prep_data(self):
        start_3_2 = b'\x00\x00\xd3k\x00\x00\x02\x00\xc9o\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        end_3_2 = b'\x11\x00\xd3k\x00\x00\x02\x00\xc4\x80\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        end_3_3 = b'1\x00\xd3k\x00\x00\x03\x00\xcf\x9d\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        start_3_4 = b'@\x00\xd3k\x00\x00\x04\x00\x0e\xa4\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        start_3_5 = b'`\x00\xd3k\x00\x00\x05\x00\xe4\xab\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        end_3_5 = b'q\x00\xd3k\x00\x00\x05\x00\xeb\xac\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': '1150', "hwts_frequency": "38.4"}]}

        with mock.patch(NAMESPACE + '.HwtsCalculator.calculate'), \
                mock.patch(NAMESPACE + '.HwtsCalculator.save'):
            check = HwtsCalculator(self.file_list, CONFIG)
            check._log_data.append(HwtsLogBean.decode(start_3_2))
            check._log_data.append(HwtsLogBean.decode(end_3_2))
            check._log_data.append(HwtsLogBean.decode(end_3_3))
            check._log_data.append(HwtsLogBean.decode(start_3_4))
            check._log_data.append(HwtsLogBean.decode(start_3_5))
            check._log_data.append(HwtsLogBean.decode(start_3_5))
            check._log_data.append(HwtsLogBean.decode(end_3_5))
            check._prep_data()

    def test__reform_data(self):
        prep_data_res = [[1, 2, 1, 2], [3, 4, 3, 4]]
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}

        ProfilingScene().init("")

        with mock.patch(NAMESPACE + '.HwtsCalculator.calculate'), \
                mock.patch('msmodel.iter_rec.iter_rec_model' + '.HwtsIterModel.get_batch_list',
                           return_value=prep_data_res), \
                mock.patch('msmodel.interface.base_model' + '.BaseModel.check_table', return_value=True), \
                mock.patch('msmodel.interface.base_model' + '.BaseModel.finalize'), \
                mock.patch('common_func.msprof_step' + '.MsprofStep.get_step_data'), \
                mock.patch('common_func.msprof_step' + '.MsprofStep.get_model_and_index_id_by_iter_id',
                           return_value=(1, 1)), \
                mock.patch('common_func.iter_recorder.IterRecorder.set_current_iter_id'), \
                mock.patch('common_func.msprof_step' + '.MsprofStep.get_step_data'):
            check = HwtsCalculator(self.file_list, CONFIG)

            ProfilingScene()._scene = Constant.SINGLE_OP
            res = check._reform_data(prep_data_res)
            self.assertEqual(len(res), 2)

            model_mock = mock.Mock
            model_mock.__enter__ = BaseModel.__enter__
            model_mock.__exit__ = BaseModel.__exit__
            hwts_iter_model = model_mock()
            hwts_iter_model.init = mock.Mock(return_value=None)
            hwts_iter_model.get_batch_list = mock.Mock(return_value=[[0], [0]])
            check._iter_model = hwts_iter_model
            with mock.patch(NAMESPACE + '.HwtsCalculator.is_need_parse_all_file', return_value=False):
                res = check._reform_data(prep_data_res)
        self.assertEqual(len(res), 2)

    def test_save(self):
        check = HwtsCalculator(self.file_list, CONFIG)
        check._log_data = [[1, 2, 1, 2], [3, 4, 3, 4]]
        with mock.patch("msmodel.task_time.hwts_log_model.HwtsLogModel.init"), \
                mock.patch("msmodel.task_time.hwts_log_model.HwtsLogModel.flush"), \
                mock.patch("msmodel.task_time.hwts_log_model.HwtsLogModel.finalize"), \
                mock.patch("common_func.utils.Utils.obj_list_to_list"), \
                mock.patch(NAMESPACE + ".HwtsCalculator._reform_data"), \
                mock.patch(NAMESPACE + ".HwtsCalculator._prep_data"):
            check.save()
