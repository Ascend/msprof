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
import struct
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from msparser.aicpu.aicpu_bin_data_parser import AicpuBinDataParser
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aicpu.aicpu_bin_data_parser'


def get_step_trace_data():
    step_trace_dto = StepTraceDto()
    step_trace_dto.step_start = 1
    step_trace_dto.step_end = 10
    step_trace_dto.iter_id = 1
    return step_trace_dto


class TestAicpuBinDataParser(unittest.TestCase):
    file_list = {DataTag.AI_CPU: ['.data.0.slice_0']}

    def test_read_binary_data(self):
        data = struct.pack("=HHHHQQQQQQQIIQQQQIIIHBBQHHHHQQQQQQQIIQQQQIIIHBBQ",
                           23130, 60, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 23130, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch('os.path.getsize', return_value=256), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data):
            with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True), \
                    mock.patch('common_func.utils.Utils.get_scene', return_value=''), \
                    mock.patch("common_func.file_manager.check_path_valid"), \
                    mock.patch('msmodel.step_trace.ts_track_model.TsTrackModel.get_step_trace_data',
                               return_value=[get_step_trace_data()]), \
                    mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
                check = AicpuBinDataParser(self.file_list, CONFIG)
                InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
                check.read_binary_data('DATA_PREPROCESS.AICPU.7.slice_0')
            self.assertEqual(check.ai_cpu_datas,
                             [[0, '0', 10.0, 20.0, '', 0.0, 0.0, 10.0, 0.0, 0.0]])

    def test_parse_ai_cpu(self):
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=True), \
                mock.patch(NAMESPACE + '.is_valid_original_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.AicpuBinDataParser.read_binary_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = AicpuBinDataParser(self.file_list, CONFIG)
            check.parse_ai_cpu()
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=False), \
             mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = AicpuBinDataParser(self.file_list, CONFIG)
            result = check.parse_ai_cpu()
        self.assertEqual(result, None)

    def test_save(self):
        with mock.patch('msmodel.ai_cpu.ai_cpu_model.AiCpuModel.init'), \
                mock.patch('msmodel.ai_cpu.ai_cpu_model.AiCpuModel.create_table'), \
                mock.patch('msmodel.ai_cpu.ai_cpu_model.AiCpuModel.flush'), \
                mock.patch('msmodel.ai_cpu.ai_cpu_model.AiCpuModel.finalize'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            InfoConfReader()._info_json = {"devices": '0'}
            check = AicpuBinDataParser(self.file_list, CONFIG)
            check.ai_cpu_datas = [123]
            check.save()

    def test_ms_run(self):
        ProfilingScene().init("")
        ProfilingScene()._scene = Constant.STEP_INFO
        with mock.patch(NAMESPACE + '.AicpuBinDataParser.parse_ai_cpu', side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = AicpuBinDataParser(self.file_list, CONFIG)
            check.ms_run()
