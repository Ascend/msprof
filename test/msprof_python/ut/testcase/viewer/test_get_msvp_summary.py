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
import json
import unittest
from unittest import mock

from sqlite.db_manager import DBOpen

from common_func.ms_constant.number_constant import NumberConstant
from viewer.get_msvp_summary import get_aicore_utilization
from viewer.get_msvp_summary import pre_check_pmu_events_interface
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'viewer.get_msvp_summary'
param = {'project': "", "project_path": '', "device_id": 0,
         "start_time": 0, "end_time": 9223372036854775807,
         "data_type": "llc_aicpu"}
sample_config = {"ai_core_profiling_mode": "bandwidth"}
info_json = {"DeviceInfo": [
    {"id": 0, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55",
     "ctrl_cpu_core_num": 1, "ctrl_cpu_endian_little": 1,
     "ts_cpu_core_num": 4, "ai_cpu_core_num": 14, "ai_core_num": 32,
     "ai_cpu_core_id": 2, "ai_core_id": 0,
     "aicpu_occupy_bitmap": 65532, "ctrl_cpu": "0",
     "ai_cpu": "2,3,4,5,6,7,8,9,10,11,12,13,14",
     "devFrequency": "2.7GHz", "aiv_num": 0, "hwts_frequency": "100",
     "ai_core_profiling_mode": "sample-based",
     "aic_frequency": "1000", "aiv_frequency": "1000"}]}


class TestMsvpSummary(unittest.TestCase):

    @staticmethod
    def setup_class():
        InfoJson(DeviceInfo=[DeviceInfo(aic_frequency="680")]).apply()

    def test_pre_check_pmu_events_interface(self):
        with mock.patch(NAMESPACE + '.path_check', side_effect=OSError):
            res = pre_check_pmu_events_interface("a")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value=None):
            res = pre_check_pmu_events_interface("")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=None):
            res = pre_check_pmu_events_interface("")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=info_json):
            res = pre_check_pmu_events_interface("")
        self.assertEqual(res[0], NumberConstant.SUCCESS)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=sample_config):
            res = pre_check_pmu_events_interface("")
        self.assertEqual(res[0], NumberConstant.ERROR)

    def test_get_aicore_utilization_1(self):
        func_map = {"type_db_match": {"ai_core_profiling": "aicore.db"},
                    "sample_config": {"ai_core_profiling_mode": "task_based"}}
        with DBOpen("aicore.db") as db_open:
            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                    mock.patch(NAMESPACE + '.pre_check_pmu_events_interface',
                               return_value=(NumberConstant.ERROR, "", "")):
                res = get_aicore_utilization("", NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                    mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
                res = get_aicore_utilization("", NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                    mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                               return_value=(db_open.db_conn, db_open.db_curs)):
                res = get_aicore_utilization("", NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

    def test_get_aicore_utilization_2(self):
        func_map = {"type_db_match": {"ai_core_profiling": "aicore.db"},
                    "sample_config": {"ai_core_profiling_mode": "sample-based"}}

        create_sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData" \
                     "(mode, replayid, timestamp, coreid, task_cyc, event1, event2, event3, event4, event5, event6, event7, event8)"
        data = ((1, 0, 176256335449860, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        with DBOpen("aicore.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("AICoreOriginalData", data)
            with mock.patch(NAMESPACE + '.path_check', return_value="test"), \
                    mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                               return_value=(db_open.db_conn, db_open.db_curs)):
                res = get_aicore_utilization("", NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)


if __name__ == '__main__':
    unittest.main()
