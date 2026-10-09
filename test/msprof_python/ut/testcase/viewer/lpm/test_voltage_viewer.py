# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025. All rights reserved.
"""

import json

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.voltage.voltage_data_viewer_model import AicVoltageViewerModel
from msmodel.voltage.voltage_data_viewer_model import BusVoltageViewerModel
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.lpm.voltage_viewer import VoltageViewer
from constant.constant import clear_dt_project


class TestVoltageViewer(TestDirCRBaseModel):

    def setUp(self):
        super().setUp()
        InfoConfReader()._info_json = {
            "pid": 1,
            "DeviceInfo": [{
                "aic_frequency": "1850",
                "hwts_frequency": "50.000000"
            }],
            "CPU": [
                {"Id": 0, "Name": "HiSilicon", "Frequency": "99.999001", "Logical_CPU_Count": 1, "Type": "TaishanV110"}
            ]
        }
        InfoConfReader()._start_info = {StrConstant.COLLECT_TIME_BEGIN: "1"}
        InfoConfReader()._end_info = {}
        self.params = {
            "data_type": "msprof",
            "project": self.PROF_HOST_DIR,
            "device_id": 7, "export_type": "timeline", "iter_id": [4294967295, 1, 1], "export_format": "",
            "model_id": 4294967295
        }

        self._origin_chip_id = ChipManager().get_chip_id()
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0

    def tearDown(self):
        InfoConfReader()._info_json = {}
        InfoConfReader()._start_info = {}
        InfoConfReader()._end_info = {}
        clear_dt_project(self.DIR_PATH)
        ChipManager().chip_id = self._origin_chip_id

    def test_aic_voltage_data_viewer_model_get_data_return_ok(self):
        data = [
            [900, 2],
            [850, 3],
            [950, 4]
        ]

        with AicVoltageViewerModel(self.params) as aic_voltage_viewer_model:
            aic_voltage_viewer_model.create_table()
            aic_voltage_viewer_model.insert_data_to_db(DBNameConstant.TABLE_AIC_VOLTAGE, data)
            result = aic_voltage_viewer_model.get_data()

            self.assertEqual(len(result), 3)
            self.assertEqual(result[0], (900, 2))
            self.assertEqual(result[1], (850, 3))
            self.assertEqual(result[2], (950, 4))

    def test_bus_voltage_data_viewer_model_get_data_return_ok(self):
        data = [
            [900, 2],
            [850, 3],
            [950, 4]
        ]

        with BusVoltageViewerModel(self.params) as bus_voltage_viewer_model:
            bus_voltage_viewer_model.create_table()
            bus_voltage_viewer_model.insert_data_to_db(DBNameConstant.TABLE_BUS_VOLTAGE, data)
            result = bus_voltage_viewer_model.get_data()

            self.assertEqual(len(result), 3)
            self.assertEqual(result[0], (900, 2))
            self.assertEqual(result[1], (850, 3))
            self.assertEqual(result[2], (950, 4))

    def test_voltage_viewer_get_all_data_return_ok(self):
        """
        voltage_viewer.get_all_data()
        具体操作：插入几条数据，获取其中的最早的开始时间
        """
        aic_voltage_data = [
            [10, 850],
            [100000, 600],
            [150000000, 950],
        ]

        bus_voltage_data = [
            [10, 800],
            [100000, 600],
            [150000000, 900],
        ]

        with AicVoltageViewerModel(self.params) as aic_model, BusVoltageViewerModel(self.params) as bus_model:
            aic_model.create_table()
            bus_model.create_table()
            aic_model.insert_data_to_db(DBNameConstant.TABLE_AIC_VOLTAGE, aic_voltage_data)
            bus_model.insert_data_to_db(DBNameConstant.TABLE_BUS_VOLTAGE, bus_voltage_data)

            result = VoltageViewer(self.params).get_all_data()
            self.assertEqual(len(result), 6005)
            self.assertEqual(json.dumps(result[-1]),
                            '{"name": "process_name", "pid": 1, "tid": 0, "args": {"name": "Voltage Info"}, "ph": "M"}')
            self.assertEqual(result[0]['name'], "Aicore Voltage")
            self.assertEqual(result[0]['pid'], 1)
            self.assertEqual(result[0]['tid'], 0)
            self.assertEqual(result[0]['args']['mV'], 850.0)
            self.assertEqual(result[0]['ph'], "C")
