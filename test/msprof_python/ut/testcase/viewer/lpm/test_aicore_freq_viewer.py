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
Copyright Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
"""

import json

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.api.api_data_viewer_model import ApiDataViewModel
from msmodel.freq.freq_data_viewer_model import FreqDataViewModel
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.lpm.ai_core_freq_viewer import AiCoreFreqViewer
from constant.constant import clear_dt_project

NAMESPACE = 'msmodel.freq.freq_data_viewer_model'


class TestAiCoreFreqViewer(TestDirCRBaseModel):

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
        self.apis_data = [
        # struct_type TEXT, id TEXT, level TEXT, thread_id INTEGER, start INTEGER, end INTEGER, connection_id INTEGER
            ['ACL_OP', 'aclCreateTensorDesc1', 'acl1', 1, 'A', 4, 10, 1],
            ['ACL_OP_X', 'aclCreateTensorDesc2', 'acl2', 1, 'B', 5, 10, 1],
            ['ACL_OP_Z', 'aclCreateTensorDesc3', 'acl3', 1, 'C', 6, 10, 1]
        ]

    def tearDown(self):
        InfoConfReader()._info_json = {}
        InfoConfReader()._start_info = {}
        InfoConfReader()._end_info = {}
        clear_dt_project(self.DIR_PATH)

    def test_freq_data_viewer_model_get_data_return_ok(self):
        """
        目的：覆盖freq_data_viewer_model。py文件中get_data方法
        UT设计意图：获取 910B freq.db FreqParse表格中变频数据，并在timeline中呈现，为了呈现是否AICORE频率变化导致算子性能下降
        具体操作：插入几条变频数据，读取之后确认freq读取逻辑是否正确
        """
        data = [
            # syscnt INTEGER, frequency INTEGER
            [1850, 2],
            [1950, 3],
            [2000, 4]
        ]

        with FreqDataViewModel(self.params) as model:
            model.create_table()
            model.insert_data_to_db(DBNameConstant.TABLE_FREQ_PARSE, data)
            result = model.get_data()
            self.assertEqual(len(result), 3)
            self.assertEqual(result[0], (1850, 2))
            self.assertEqual(result[1], (1950, 3))
            self.assertEqual(result[2], (2000, 4))

    def test_api_data_viewer_model_earliest_start_time_return_ok(self):
        """
        目的：覆盖api_data_viewer_model.py 文件get_earliest_api方法
        UT设计意图：获取api_event.db 中的ApiData表格中最好的事件开始时间，作为aicore频率起始时间戳。
        具体操作：插入几条数据，获取其中的最早的开始时间
        """
        with ApiDataViewModel(self.params) as model:
            model.create_table()
            model.insert_data_to_db(DBNameConstant.TABLE_API_DATA, self.apis_data)
            result = model.get_earliest_api()
            self.assertEqual(result[0].start, 4)

    def test_aicore_freq_viewer_get_all_data_return_ok(self):
        """
        目的：覆盖ai_core_freq_viewer.py 文件get_all_data
        UT设计意图：获取api_event.db 中的ApiData表格中最好的事件开始时间，作为aicore频率起始时间戳。
        具体操作：插入几条数据，获取其中的最早的开始时间
        """
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        freq_data = [
            # frequency INTEGER, syscnt INTEGER
            [2, 1850]
        ]

        with FreqDataViewModel(self.params) as freq_model, ApiDataViewModel(self.params) as api_model:
            # mock freq.db 和 apidata数据
            freq_model.create_table()
            freq_model.insert_data_to_db(DBNameConstant.TABLE_FREQ_PARSE, freq_data)
            api_model.create_table()
            api_model.insert_data_to_db(DBNameConstant.TABLE_API_DATA, self.apis_data)
            query_freq = freq_model.get_data()
            query_api_start_time = api_model.get_earliest_api()
            self.assertEqual(len(query_freq), 1)
            self.assertEqual(query_api_start_time[0].start, 4)
            freq_list = AiCoreFreqViewer(self.params).get_all_data()
            self.assertEqual(len(freq_list), 3)
            self.assertEqual(json.dumps(freq_list[0]),
                '{"name": "process_name", "pid": 1, "tid": 0, "args": {"name": "AI Core Freq"}, "ph": "M"}')
            self.assertEqual(freq_list[1]['name'], "AI Core Freq")
            self.assertEqual(freq_list[1]['pid'], 1)
            self.assertEqual(freq_list[1]['tid'], 0)
            self.assertEqual(freq_list[1]['args']['MHz'], 1850.0)
            self.assertEqual(freq_list[1]['ph'], "C")
            self.assertEqual(freq_list[1]['name'], "AI Core Freq")
            self.assertEqual(freq_list[1]['pid'], 1)
            self.assertEqual(freq_list[1]['tid'], 0)
            self.assertEqual(freq_list[1]['args']['MHz'], 1850)
            self.assertEqual(freq_list[1]['ph'], "C")

    def test_aicore_freq_viewer_not_all_export_get_all_data_return_ok(self):
        """
        目的: 测试非全导场景下AiCoreFreqViewer.get_all_data函数
        UT设计意图：获取api_event.db 中的ApiData表格中最好的事件开始时间，作为aicore频率起始时间戳。
        具体操作：插入几条数据，获取其中的最早的开始时间
        """
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        freq_data = [
            # frequency INTEGER, syscnt INTEGER
            [2, 1850]
        ]

        with FreqDataViewModel(self.params) as freq_model, ApiDataViewModel(self.params) as api_model:
            origin_mode = ProfilingScene()._mode
            ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
            # mock freq.db 和 api_data数据
            freq_model.create_table()
            freq_model.insert_data_to_db(DBNameConstant.TABLE_FREQ_PARSE, freq_data)
            api_model.create_table()
            api_model.insert_data_to_db(DBNameConstant.TABLE_API_DATA, self.apis_data)
            query_freq = freq_model.get_data()
            query_api_start_time = api_model.get_earliest_api()
            self.assertEqual(len(query_freq), 1)
            self.assertEqual(query_api_start_time[0].start, 4)
            freq_list = AiCoreFreqViewer(self.params).get_all_data()
            self.assertEqual(len(freq_list), 3)
            self.assertEqual(json.dumps(freq_list[0]),
                '{"name": "process_name", "pid": 1, "tid": 0, "args": {"name": "AI Core Freq"}, "ph": "M"}')
            self.assertEqual(freq_list[1]['name'], "AI Core Freq")
            self.assertEqual(freq_list[1]['pid'], 1)
            self.assertEqual(freq_list[1]['tid'], 0)
            self.assertEqual(freq_list[1]['args']['MHz'], 1850.0)
            self.assertEqual(freq_list[1]['ph'], "C")
            self.assertTrue(isinstance(freq_list[1]['ts'], str))
            self.assertEqual(freq_list[2]['name'], "AI Core Freq")
            self.assertEqual(freq_list[2]['pid'], 1)
            self.assertEqual(freq_list[2]['tid'], 0)
            self.assertEqual(freq_list[2]['args']['MHz'], 1850)
            self.assertEqual(freq_list[2]['ph'], "C")
            self.assertTrue(isinstance(freq_list[2]['ts'], str))
            ProfilingScene().set_mode(origin_mode)

    def test_aicore_freq_viewer_when_no_freq_return_ok(self):
        """
        目的：覆盖ai_core_freq_viewer.py 文件get_all_data
        UT设计意图：当不上报变频数据时，InfoConfReader().get_dev_cnt()作为开始时间，以默认频率作为freq。
        """
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0

        with FreqDataViewModel(self.params) as freq_model, ApiDataViewModel(self.params) as api_model:
            # mock freq.db 和 api_data数据
            freq_model.create_table()
            query_freq = freq_model.get_data()
            self.assertEqual(len(query_freq), 0)
            freq_list = AiCoreFreqViewer(self.params).get_all_data()
            self.assertEqual(len(freq_list), 3)
            self.assertEqual(json.dumps(freq_list[0]),
                '{"name": "process_name", "pid": 1, "tid": 0, "args": {"name": "AI Core Freq"}, "ph": "M"}')
            self.assertEqual(freq_list[1]['name'], "AI Core Freq")
            self.assertEqual(freq_list[1]['pid'], 1)
            self.assertEqual(freq_list[1]['tid'], 0)
            self.assertEqual(freq_list[1]['args']['MHz'], 1850.0)
            self.assertEqual(freq_list[1]['ph'], "C")
            self.assertEqual(freq_list[1]['name'], "AI Core Freq")
            self.assertEqual(freq_list[1]['pid'], 1)
            self.assertEqual(freq_list[1]['tid'], 0)
            self.assertEqual(freq_list[1]['args']['MHz'], 1850)
            self.assertEqual(freq_list[1]['ph'], "C")

    def test_aicore_freq_viewer_no_lpm_data_return_ok(self):
        """
        目的：覆盖ai_core_freq_viewer.py 文件get_all_data
        UT设计意图：获取api_event.db 中的ApiData表格中最好的事件开始时间，作为aicore频率起始时间戳。
        具体操作：插入几条数据，获取其中的最早的开始时间
        """
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0

        with FreqDataViewModel(self.params) as freq_model:
            # api_data数据
            query_freq = freq_model.get_data()
            self.assertEqual(len(query_freq), 0)
