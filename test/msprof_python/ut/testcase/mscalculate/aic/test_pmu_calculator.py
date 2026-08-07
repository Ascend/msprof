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
import collections
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from mscalculate.aic.pmu_calculator import PmuCalculator

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'mscalculate.aic.pmu_calculator'


def _make_mock_info_conf(device_id=0, host_start_cnt=123456789):
    """构造一个用于替换 InfoConfReader 单例的 mock 对象"""
    mock_instance = mock.MagicMock()
    mock_instance.get_device_id.return_value = device_id
    mock_instance.trans_from_start_info_raw_time_into_host_cnt.return_value = host_start_cnt
    return mock_instance


class TestPmuCalculator(unittest.TestCase):
    GeDataBean = collections.namedtuple('ge_data', ['task_type', 'task_id', 'stream_id', 'block_num', 'mix_block_num'])

    def test_init_param(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_block_num_from_ge'), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_config_params'):
            check = PmuCalculator()
            check.sample_config = {'result_dir': 'test', 'device_id': '0', 'iter_id': 1, 'job_id': 'job_default'}
            check.init_params()

    def test_get_block_num_from_ge(self):
        mock_info_conf = _make_mock_info_conf()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=mock_info_conf):
            key = PmuCalculator()
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_num = {'block_num': {'0-0': [20]}}
            key._freq = 1500
            key.get_block_num_from_ge()
        ProfilingScene().init('test')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        mock_info_conf2 = _make_mock_info_conf()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                           return_value=[self.GeDataBean('', 0, 0, 0, 0), self.GeDataBean("AI_CORE", 0, 0, 20, 40),
                                         self.GeDataBean('MIX_AIC', 0, 0, 20, 40)]), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                           return_value=[[1, 1]]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="step_info"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=mock_info_conf2):
            key = PmuCalculator()
            key._block_num = {'block_num': {'0-0': [20]}, 'mix_block_num': {'0-0': [20]}}
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._freq = 1500
            key.get_block_num_from_ge()

    def test_get_current_block(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('os.path.join', return_value='test\\test.text'):
            key = PmuCalculator()
            key._block_num = {'block_num': {'0-0': [20]}}
            result = key._get_current_block('block_num', self.GeDataBean('', 0, 0, 20, 40))
            self.assertEqual(result, 20)

    def test_get_config_params(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('os.listdir', return_value=['info.json.0']):
            key = PmuCalculator()
            InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 8, 'aiv_num': 8, 'aic_frequency': 1500}]}
            key._block_num = {'block_num': {'0-0': [20]}}
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_num = {'block_num': {'2-2': [22, 22]}}
            key._freq = 1500
            key.get_config_params()

    def test_get_block_num_data_sql_with_invalid_model_id(self):
        """当 model_id = INVALID_MODEL_ID 时，SQL 应包含 timestamp >= host_start_cnt 过滤条件"""
        fetch_calls = []

        def capture_fetch(curs, sql, param=None, dto_class=None):
            fetch_calls.append((sql, param))
            return []

        ProfilingScene().init('test')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        device_id = 0
        host_start_cnt = 123456789
        mock_info_conf = _make_mock_info_conf(device_id=device_id, host_start_cnt=host_start_cnt)
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', side_effect=capture_fetch), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=mock_info_conf), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                           return_value=[(1, NumberConstant.INVALID_MODEL_ID)]), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            key = PmuCalculator()
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_num = {'block_num': {'0-0': [20]}}
            key._freq = 1500
            key.get_block_num_from_ge()

        self.assertGreaterEqual(len(fetch_calls), 1, "fetch_all_data should be called at least once")
        found = False
        for sql, _ in fetch_calls:
            if f"timestamp >= {host_start_cnt}" in sql:
                found = True
                break
        self.assertTrue(found, f"SQL should contain 'timestamp >= {host_start_cnt}' when model_id is INVALID_MODEL_ID")

    def test_get_block_num_data_sql_without_invalid_model_id(self):
        """当 model_id != INVALID_MODEL_ID 时，SQL 不应包含 timestamp 过滤条件"""
        fetch_calls = []

        def capture_fetch(curs, sql, param=None, dto_class=None):
            fetch_calls.append((sql, param))
            return []

        ProfilingScene().init('test')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        valid_model_id = 42
        mock_info_conf = _make_mock_info_conf()
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', side_effect=capture_fetch), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=mock_info_conf), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                           return_value=[(1, valid_model_id)]), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            key = PmuCalculator()
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_num = {'block_num': {'0-0': [20]}}
            key._freq = 1500
            key.get_block_num_from_ge()

        self.assertGreaterEqual(len(fetch_calls), 1, "fetch_all_data should be called at least once")
        for sql, params in fetch_calls:
            self.assertNotIn("timestamp >=", sql,
                             "SQL should NOT contain 'timestamp >=' when model_id is not INVALID_MODEL_ID")
            self.assertEqual(len(params), 2,
                             f"Params should have 2 elements (model_id, iter_id), got {len(params)}: {params}")

    def test_get_block_num_data_sql_params_count(self):
        """fetch_all_data 的参数元组应当只有2个值: (model_id, iter_id)，不应包含 condition 字符串"""
        fetch_calls = []

        def capture_fetch(curs, sql, param=None, dto_class=None):
            fetch_calls.append((sql, param))
            return []

        ProfilingScene().init('test')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        mock_info_conf = _make_mock_info_conf()
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', side_effect=capture_fetch), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.InfoConfReader', return_value=mock_info_conf), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                           return_value=[(1, NumberConstant.INVALID_MODEL_ID), (2, 42)]), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            key = PmuCalculator()
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_num = {'block_num': {'0-0': [20]}}
            key._freq = 1500
            key.get_block_num_from_ge()

        self.assertGreaterEqual(len(fetch_calls), 2, "fetch_all_data should be called at least twice (one per iter)")
        for sql, params in fetch_calls:
            self.assertEqual(len(params), 2,
                             f"Params should have exactly 2 elements, got {len(params)}: {params}. SQL: {sql[:80]}...")


if __name__ == '__main__':
    unittest.main()
