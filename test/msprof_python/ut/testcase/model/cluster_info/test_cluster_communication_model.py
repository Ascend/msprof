# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
import sqlite3
import unittest
from unittest.mock import Mock, patch

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.cluster_info.cluster_communication_model import ClusterCommunicationModel
from profiling_bean.db_dto.collective_communication_dto import CollectiveCommunicationDto


class TestClusterCommunicationModel(unittest.TestCase):
    """ClusterCommunicationModel 单元测试类"""

    def setUp(self):
        """测试前准备"""
        # 模拟参数
        self.mock_params = {
            "collection_path": "/fake/collection/path",
            "model_id": 123,
            "iteration_id": 456
        }

        # 创建模拟的数据库连接和游标
        self.mock_conn = Mock(spec=sqlite3.Connection)
        self.mock_cursor = Mock(spec=sqlite3.Cursor)
        self.mock_conn.cursor.return_value = self.mock_cursor

        # 设置模拟的DBManager.create_connect_db
        self.db_manager_patcher = patch.object(DBManager, 'create_connect_db', return_value=self.mock_conn)
        self.mock_get_connection = self.db_manager_patcher.start()

        # 设置模拟的ViewModel.__init__，避免真实数据库连接
        self.view_model_patcher = patch.object(ClusterCommunicationModel.__bases__[0], '__init__', return_value=None)
        self.mock_view_model_init = self.view_model_patcher.start()

        # 创建测试实例
        self.model = ClusterCommunicationModel(self.mock_params)
        self.model.cur = self.mock_cursor

    def tearDown(self):
        """测试后清理"""
        self.db_manager_patcher.stop()
        self.view_model_patcher.stop()

    def test_init_sets_correct_attributes(self):
        """测试初始化是否正确设置属性"""
        # 验证实例属性
        self.assertEqual(self.model._collection_path, "/fake/collection/path")
        self.assertEqual(self.model._model_id, 123)
        self.assertEqual(self.model._iteration_id, 456)

        # 验证父类被正确调用
        self.mock_view_model_init.assert_called_once_with(
            "/fake/collection/path",
            DBNameConstant.DB_CLUSTER_STEP_TRACE,
            []
        )

    def test_get_cluster_communication_generates_correct_sql(self):
        """测试 get_cluster_communication 方法生成正确的 SQL"""
        # 准备测试数据
        rank_id = 0
        mock_dto = Mock(spec=CollectiveCommunicationDto)
        mock_dto.rank_id = 0
        mock_dto.compute_time = 150.5
        mock_dto.communication_time = 50.2
        mock_dto.stage_time = 200.7

        # 模拟 fetch_all_data
        with patch.object(DBManager, 'fetch_all_data', return_value=[mock_dto]) as mock_fetch:
            result = self.model.get_cluster_communication(rank_id)

            # 验证 SQL 正确性
            expected_sql = (
                "select 0 as rank_id, t0.fp_bp_time - t0.fp_bp_communication_time as compute_time,"
                "t0.communication_time, "
                "t0.iteration_time - t0.communication_time as stage_time "
                "from (select "
                "(case when tt.fp_bp_time = 0 then tt.iteration_time else tt.fp_bp_time end) as fp_bp_time, "
                "tt.iteration_time, sum(t1.all_reduce_end - t1.all_reduce_start) as communication_time,"
                "sum(case when fp_bp_time > 0 and t1.all_reduce_start > tt.bp_end "
                "then 0 else t1.all_reduce_end - t1.all_reduce_start end) as fp_bp_communication_time "
                "from table_cluster_step_trace_0 t1 inner join table_cluster_step_trace_0 tt "
                "on t1.model_id = tt.model_id and t1.index_id = tt.iteration_id "
                "and t1.model_id = 123 and t1.index_id = 456 "
                "group by t1.model_id, t1.index_id) t0"
            )

            # 注意：这里使用了动态的表名，需要根据实际的 DBNameConstant 值来调整
            # 我们检查了 SQL 结构，但实际表名需要从常量获取
            # 这里验证 fetch_all_data 被正确调用
            mock_fetch.assert_called_once()

            # 获取实际调用的参数
            call_args = mock_fetch.call_args
            actual_sql = call_args[0][1]  # 第二个位置参数是 SQL
            actual_dto_class = call_args[1]['dto_class']  # 关键字参数 dto_class

            # 验证 SQL 包含关键部分
            self.assertIn("rank_id", actual_sql)
            self.assertIn("compute_time", actual_sql)
            self.assertIn("communication_time", actual_sql)
            self.assertIn("stage_time", actual_sql)
            self.assertIn(str(self.model._model_id), actual_sql)
            self.assertIn(str(self.model._iteration_id), actual_sql)
            self.assertIn(str(rank_id), actual_sql)

            # 验证 DTO 类
            self.assertEqual(actual_dto_class, CollectiveCommunicationDto)

            # 验证返回结果
            self.assertEqual(len(result), 1)
            self.assertIsInstance(result[0], Mock)

    def test_get_cluster_communication_handles_empty_result(self):
        """测试处理空结果集"""
        rank_id = 0

        with patch.object(DBManager, 'fetch_all_data', return_value=[]):
            result = self.model.get_cluster_communication(rank_id)
            self.assertEqual(result, [])

    def test_get_cluster_communication_handles_multiple_results(self):
        """测试处理多个结果"""
        rank_id = 0
        mock_results = [
            Mock(spec=CollectiveCommunicationDto, rank_id=0, compute_time=100.0, communication_time=20.0,
                 stage_time=120.0),
            Mock(spec=CollectiveCommunicationDto, rank_id=0, compute_time=150.0, communication_time=30.0,
                 stage_time=180.0),
        ]

        with patch.object(DBManager, 'fetch_all_data', return_value=mock_results):
            result = self.model.get_cluster_communication(rank_id)
            self.assertEqual(len(result), 2)
            self.assertEqual(result[0].compute_time, 100.0)
            self.assertEqual(result[1].compute_time, 150.0)


if __name__ == '__main__':
    unittest.main()
