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

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from profiling_bean.db_dto.biu_perf_insrt_dto import BiuPerfInstrDto
from viewer.biu_perf.biu_perf_chip6_viewer import BiuPerfChip6Viewer

NAMESPACE = "msviewer.biu_perf.biu_perf_chip6_viewer"


class TestBiuPerfChip6Viewer(unittest.TestCase):

    def test_get_trace_timeline(self):
        configs = {StrConstant.SAMPLE_CONFIG_PROJECT_PATH: "test_project"}
        params = {}
        InfoConfReader()._info_json = {"pid": 0, "tid": 0, "DeviceInfo": [{"hwts_frequency": "50"}]}
        InfoConfReader()._host_mon = 0
        InfoConfReader()._dev_cnt = 0
        InfoConfReader()._local_time_offset = 0
        viewer = BiuPerfChip6Viewer(configs, params)
        data_list = [
            BiuPerfInstrDto(group_id=0, core_type='aic', block_id=0, timestamp=1000, duration=2000,
                            instruction="MTE1", checkpoint_info=None),
            BiuPerfInstrDto(group_id=0, core_type='aiv0', block_id=1, timestamp=3000, duration=4000,
                            instruction="MTE2", checkpoint_info=None),
            BiuPerfInstrDto(group_id=1, core_type='aiv1', block_id=0, timestamp=5000, duration=6000,
                            instruction="MTE3", checkpoint_info=None)
        ]
        result = viewer.get_trace_timeline(data_list)
        expected_meta_data = [
            ["process_name", 0, 0, TraceViewHeaderConstant.PROCESS_BIU_PERF],
            ["thread_name", 0, 0, "Group0-aic"],
            ["thread_name", 0, 1, "Group0-aiv0"],
            ["thread_name", 0, 2, "Group1-aiv1"],
        ]
        expected_column_trace_data = [
            ["MTE1", 0, 0, '20.000', 40.0, {"Core Type": "aic", "Block Id": 0}],
            ["MTE2", 0, 1, '60.000', 80.0, {"Core Type": "aiv0", "Block Id": 1}],
            ["MTE3", 0, 2, '100.000', 120.0, {"Core Type": "aiv1", "Block Id": 0}]
        ]
        expected_result = TraceViewManager.metadata_event(expected_meta_data)
        expected_result.extend(TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                                 expected_column_trace_data))
        self.assertEqual(expected_result, result)


if __name__ == "__main__":
    unittest.main()
