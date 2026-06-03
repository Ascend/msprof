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
from msmodel.acl.acl_model import AclModel
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.acl_dto import AclDto


class TestAclModel(unittest.TestCase):
    """AclModel 单元测试类"""

    def setUp(self):
        """测试前准备"""
        self.mock_params = {
            "result_dir": "/fake/path/to/results",
            "iter_id": "0-10"
        }

        # 创建模拟的数据库连接和游标
        self.mock_conn = Mock(spec=sqlite3.Connection)
        self.mock_cursor = Mock(spec=sqlite3.Cursor)
        self.mock_conn.cursor.return_value = self.mock_cursor

        # 创建 AclModel 实例
        with patch.object(ViewModel, '__init__', return_value=None):
            with patch.object(DBManager, 'create_connect_db') as mock_get_conn:
                mock_get_conn.return_value = self.mock_conn
                self.acl_model = AclModel(self.mock_params)
                self.acl_model.cur = self.mock_cursor

    def tearDown(self):
        """测试后清理"""
        pass

    def test_get_timeline_data_sql_correct(self):
        """测试 get_timeline_data 方法生成正确的 SQL"""
        # 准备模拟数据
        mock_data = [
            ("aclMemcpy", 1000, 50, 1001, 2001, 0),
            ("aclOpCompile", 1100, 100, 1001, 2002, 1)
        ]

        # 模拟 fetch_all_data
        with patch.object(DBManager, 'fetch_all_data', return_value=mock_data) as mock_fetch:
            result = self.acl_model.get_timeline_data()

            # 验证 SQL 正确
            expected_sql = (
                "select api_name, start_time, (end_time-start_time) "
                "as output_duration, process_id, thread_id, api_type "
                f"from {DBNameConstant.TABLE_ACL_DATA} order by start_time"
            )

            # 验证 fetch_all_data 被正确调用
            mock_fetch.assert_called_once_with(self.mock_cursor, expected_sql)

            # 验证返回数据
            self.assertEqual(result, mock_data)

    def test_get_timeline_data_empty_result(self):
        """测试 get_timeline_data 处理空结果"""
        with patch.object(DBManager, 'fetch_all_data', return_value=[]):
            result = self.acl_model.get_timeline_data()
            self.assertEqual(result, [])

    def test_get_summary_data_sql_correct(self):
        """测试 get_summary_data 方法生成正确的 SQL"""
        mock_data = [
            ("aclMemcpy", 0, 1000, 50, 1001, 2001),
            ("aclOpCompile", 1, 1100, 100, 1001, 2002)
        ]

        with patch.object(DBManager, 'fetch_all_data', return_value=mock_data) as mock_fetch:
            result = self.acl_model.get_summary_data()

            expected_sql = (
                "select api_name, api_type, start_time, (end_time-start_time) "
                "as output_duration, process_id, thread_id "
                f"from {DBNameConstant.TABLE_ACL_DATA} order by start_time asc"
            )

            mock_fetch.assert_called_once_with(self.mock_cursor, expected_sql)
            self.assertEqual(result, mock_data)

    def test_get_acl_op_execute_data_returns_dto_objects(self):
        """测试 get_acl_op_execute_data 返回 AclDto 对象"""
        # 准备模拟的 AclDto 对象
        mock_dto_1 = Mock(spec=AclDto)
        mock_dto_1.start_time = 1000
        mock_dto_1.end_time = 1100
        mock_dto_1.thread_id = 2001

        mock_dto_2 = Mock(spec=AclDto)
        mock_dto_2.start_time = 1200
        mock_dto_2.end_time = 1300
        mock_dto_2.thread_id = 2002

        mock_dtos = [mock_dto_1, mock_dto_2]

        with patch.object(DBManager, 'fetch_all_data', return_value=mock_dtos) as mock_fetch:
            result = self.acl_model.get_acl_op_execute_data()

            # 验证 SQL
            expected_sql = (
                f"select start_time, end_time, thread_id from {DBNameConstant.TABLE_ACL_DATA} where "
                "api_name='aclopCompileAndExecute' or api_name='aclopCompileAndExecuteV2' order by start_time"
            )

            mock_fetch.assert_called_once_with(
                self.mock_cursor,
                expected_sql,
                dto_class=AclDto
            )

            # 验证返回 AclDto 对象
            self.assertEqual(len(result), 2)
            self.assertIsInstance(result[0], Mock)  # 因为模拟了 AclDto
            self.assertEqual(result[0].start_time, 1000)

    def test_get_acl_op_compile_data_returns_dto_objects(self):
        """测试 get_acl_op_compile_data 返回 AclDto 对象"""
        mock_dto = Mock(spec=AclDto)
        mock_dto.start_time = 1500
        mock_dto.end_time = 1600
        mock_dto.thread_id = 2003

        with patch.object(DBManager, 'fetch_all_data', return_value=[mock_dto]) as mock_fetch:
            result = self.acl_model.get_acl_op_compile_data()

            expected_sql = (
                f"select start_time, end_time, thread_id from {DBNameConstant.TABLE_ACL_DATA} where "
                "api_name='OpCompile' order by start_time"
            )

            mock_fetch.assert_called_once_with(
                self.mock_cursor,
                expected_sql,
                dto_class=AclDto
            )

            self.assertEqual(len(result), 1)
            self.assertEqual(result[0].start_time, 1500)

    def test_get_acl_op_execute_data_no_results(self):
        """测试 get_acl_op_execute_data 处理无结果情况"""
        with patch.object(DBManager, 'fetch_all_data', return_value=[]):
            result = self.acl_model.get_acl_op_execute_data()
            self.assertEqual(result, [])

    def test_get_acl_op_compile_data_no_results(self):
        """测试 get_acl_op_compile_data 处理无结果情况"""
        with patch.object(DBManager, 'fetch_all_data', return_value=[]):
            result = self.acl_model.get_acl_op_compile_data()
            self.assertEqual(result, [])

    def test_implements_correct_interfaces(self):
        """测试 AclModel 实现了正确的接口"""
        # 验证继承关系
        self.assertIsInstance(self.acl_model, ViewModel)
        self.assertIsInstance(self.acl_model, IAnalysisModel)

        # 验证方法存在
        self.assertTrue(hasattr(self.acl_model, 'get_timeline_data'))
        self.assertTrue(hasattr(self.acl_model, 'get_summary_data'))
        self.assertTrue(hasattr(self.acl_model, 'get_acl_op_execute_data'))
        self.assertTrue(hasattr(self.acl_model, 'get_acl_op_compile_data'))


if __name__ == '__main__':
    unittest.main()
