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
from unittest.mock import Mock, patch, call

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msparser.interface.iparser import IParser
from msparser.iter_rec.ts_timeline_parser import TsTimelineRecParser
from profiling_bean.prof_enum.data_tag import DataTag


class TestTsTimelineRecParser(unittest.TestCase):
    """TsTimelineRecParser 单元测试类"""

    def setUp(self):
        """测试前准备"""
        # 模拟输入参数
        self.mock_file_list = {
            DataTag.TS_TRACK: ["/path/to/trace1.rec", "/path/to/trace2.rec"]
        }

        self.mock_sample_config = {
            "result_dir": "/tmp/test_project",
            "mode": "analysis"
        }

        # 模拟数据库连接和游标
        self.mock_trace_conn = Mock(spec=sqlite3.Connection)
        self.mock_trace_curs = Mock(spec=sqlite3.Cursor)
        self.mock_trace_conn.cursor.return_value = self.mock_trace_curs
        self.mock_trace_conn.commit = Mock()

        self.mock_runtime_conn = Mock(spec=sqlite3.Connection)
        self.mock_runtime_curs = Mock(spec=sqlite3.Cursor)
        self.mock_runtime_conn.cursor.return_value = self.mock_runtime_curs

        # 模拟父类的初始化

        with patch.object(MsMultiProcess, '__init__', return_value=None) as mock_ms_init:
            # 创建测试实例
            self.parser = TsTimelineRecParser(self.mock_file_list, self.mock_sample_config)
            mock_ms_init.assert_called_once()

    def tearDown(self):
        """测试后清理"""
        # 清理所有补丁
        patch.stopall()

    def test_init_sets_correct_attributes(self):
        """测试初始化是否正确设置属性"""
        # 验证实例属性
        self.assertEqual(self.parser._sample_config, self.mock_sample_config)
        self.assertEqual(self.parser._project_path, "/tmp/test_project")
        self.assertEqual(self.parser._file_list, self.mock_file_list)
        self.assertEqual(self.parser.ts_timeline_update_data, [])
        self.assertIsNone(self.parser.trace_conn)
        self.assertIsNone(self.parser.trace_curs)

    def test_check_runtime_db_success(self):
        """测试_check_runtime_db方法成功情况"""
        # 模拟表和游标存在
        with patch.object(DBManager, 'judge_table_exist', return_value=True) as mock_judge:
            result = TsTimelineRecParser._check_runtime_db(
                self.mock_runtime_conn,
                self.mock_runtime_curs
            )

            mock_judge.assert_called_once_with(
                self.mock_runtime_curs,
                DBNameConstant.TABLE_RUNTIME_TIMELINE
            )
            self.assertTrue(result)

    def test_check_runtime_db_failure_missing_conn(self):
        """测试_check_runtime_db方法 - 缺少连接"""
        # 连接为None
        result = TsTimelineRecParser._check_runtime_db(None, self.mock_runtime_curs)
        self.assertFalse(result)

        # 游标为None
        result = TsTimelineRecParser._check_runtime_db(self.mock_runtime_conn, None)
        self.assertFalse(result)

        # 两者都为None
        result = TsTimelineRecParser._check_runtime_db(None, None)
        self.assertFalse(result)

    def test_check_runtime_db_failure_table_not_exist(self):
        """测试_check_runtime_db方法 - 表不存在"""
        with patch.object(DBManager, 'judge_table_exist', return_value=False):
            result = TsTimelineRecParser._check_runtime_db(
                self.mock_runtime_conn,
                self.mock_runtime_curs
            )
            self.assertFalse(result)

    def test_get_ts_timeline_update_data_valid_input(self):
        """测试_get_ts_timeline_update_data方法 - 有效输入"""
        # 准备测试数据
        db_path = "/tmp/test.db"
        time_ranges = [
            (1, 100, 1000, 2000),  # (index_id, model_id, step_start, step_end)
            (2, 100, 3000, 4000)
        ]

        # 模拟数据库查询结果
        mock_ai_core_nums = [(5,), (8,)]

        # 模拟工具函数
        with patch.object(DBManager, 'add_new_column') as mock_add_col:
            with patch.object(Utils, 'generator_to_list') as mock_gen_list:
                # 模拟第一次generator_to_list调用
                mock_gen_list.side_effect = [
                    [(1000, 2000), (3000, 4000)],  # step_ranges
                    [  # update_data
                        (5, 1, 100),
                        (8, 2, 100)
                    ]
                ]

                # 模拟游标执行
                mock_fetch_results = [Mock(fetchone=Mock(return_value=mock_ai_core_nums[0])),
                                      Mock(fetchone=Mock(return_value=mock_ai_core_nums[1]))]
                self.mock_runtime_curs.execute.side_effect = mock_fetch_results

                # 调用静态方法
                result = TsTimelineRecParser._get_ts_timeline_update_data(
                    db_path, time_ranges, self.mock_runtime_curs
                )

                # 验证调用
                mock_add_col.assert_called_once_with(
                    db_path,
                    DBNameConstant.TABLE_STEP_TRACE_DATA,
                    "ai_core_num",
                    "INT",
                    '0'
                )

                # 验证数据库查询
                expected_sql = (
                    "select count(*) from TimeLine "
                    "where tasktype=0 and taskState=3 and timestamp>? "
                    "and timestamp<?"
                )
                self.mock_runtime_curs.execute.assert_has_calls([
                    call(expected_sql, (1000, 2000)),
                    call(expected_sql, (3000, 4000))
                ])

                # 验证结果
                self.assertEqual(len(result), 2)

    def test_get_ts_timeline_update_data_empty_result(self):
        """测试_get_ts_timeline_update_data方法 - 空结果"""
        db_path = "/tmp/test.db"
        time_ranges = [(1, 100, 1000, 2000)]

        with patch.object(DBManager, 'add_new_column'):
            with patch.object(Utils, 'generator_to_list') as mock_gen_list:
                mock_gen_list.side_effect = [
                    [(1000, 2000)],  # step_ranges
                    []  # update_data
                ]

                # 模拟查询返回None
                mock_fetch = Mock(fetchone=Mock(return_value=None))
                self.mock_runtime_curs.execute.return_value = mock_fetch

                result = TsTimelineRecParser._get_ts_timeline_update_data(
                    db_path, time_ranges, self.mock_runtime_curs
                )

                self.assertEqual(result, [])

    def test_check_step_trace_db_success(self):
        """测试_check_step_trace_db方法成功情况"""
        # 设置实例属性
        self.parser.trace_conn = self.mock_trace_conn
        self.parser.trace_curs = self.mock_trace_curs

        with patch.object(DBManager, 'judge_table_exist', return_value=True):
            result = self.parser._check_step_trace_db()
            self.assertTrue(result)

    def test_check_step_trace_db_failure_missing_conn(self):
        """测试_check_step_trace_db方法 - 缺少连接"""
        # 连接为None
        self.parser.trace_conn = None
        self.parser.trace_curs = self.mock_trace_curs

        result = self.parser._check_step_trace_db()
        self.assertFalse(result)

        # 游标为None
        self.parser.trace_conn = self.mock_trace_conn
        self.parser.trace_curs = None

        result = self.parser._check_step_trace_db()
        self.assertFalse(result)

    def test_check_step_trace_db_failure_table_not_exist(self):
        """测试_check_step_trace_db方法 - 表不存在"""
        self.parser.trace_conn = self.mock_trace_conn
        self.parser.trace_curs = self.mock_trace_curs

        with patch.object(DBManager, 'judge_table_exist', return_value=False):
            result = self.parser._check_step_trace_db()
            self.assertFalse(result)

    def test_parse_successful_flow(self):
        """测试parse方法成功流程"""
        # 设置实例属性
        self.parser.trace_conn = self.mock_trace_conn
        self.parser.trace_curs = self.mock_trace_curs

        # 模拟数据库查询结果
        mock_time_ranges = [(1, 100, 1000, 2000), (2, 100, 3000, 4000)]
        mock_update_data = [(5, 1, 100), (8, 2, 100)]

        # 模拟各种依赖
        with patch.object(PathManager, 'get_db_path', return_value="/tmp/test.db") as mock_get_db_path:
            with patch.object(DBManager, 'check_connect_db_path',
                              return_value=(self.mock_trace_conn, self.mock_trace_curs)) as mock_connect_db_path:
                with patch.object(self.parser, '_check_step_trace_db', return_value=True) as mock_check_step:
                    with patch.object(DBManager, 'fetch_all_data',
                                      return_value=mock_time_ranges) as mock_fetch:
                        with patch.object(DBManager, 'check_connect_db',
                                          return_value=(self.mock_runtime_conn,
                                                        self.mock_runtime_curs)) as mock_connect_db:
                            with patch.object(TsTimelineRecParser, '_check_runtime_db',
                                              return_value=True) as mock_check_runtime:
                                with patch.object(TsTimelineRecParser, '_get_ts_timeline_update_data',
                                                  return_value=mock_update_data) as mock_get_data:
                                    with patch.object(DBManager, 'destroy_db_connect') as mock_destroy:
                                        with patch.object(self.parser, 'save') as mock_save:
                                            # 执行parse
                                            self.parser.parse()

                                            # 验证调用链
                                            mock_get_db_path.assert_called_once_with("/tmp/test_project",
                                                                                     DBNameConstant.DB_STEP_TRACE)
                                            mock_connect_db_path.assert_called_once_with("/tmp/test.db")
                                            mock_check_step.assert_called_once()

                                            # 验证SQL查询
                                            expected_sql = (
                                                "select index_id, model_id, "
                                                "step_start, step_end from step_trace_data"
                                            )
                                            mock_fetch.assert_called_once_with(
                                                self.mock_trace_curs,
                                                expected_sql
                                            )

                                            mock_connect_db.assert_called_once_with(
                                                "/tmp/test_project",
                                                DBNameConstant.DB_RUNTIME
                                            )
                                            mock_check_runtime.assert_called_once_with(
                                                self.mock_runtime_conn,
                                                self.mock_runtime_curs
                                            )

                                            mock_get_data.assert_called_once_with("/tmp/test.db", mock_time_ranges,
                                                                                  self.mock_runtime_curs)

                                            # 验证实例属性设置
                                            self.assertEqual(self.parser.ts_timeline_update_data, mock_update_data)

                                            # 验证保存被调用
                                            mock_save.assert_called_once()

                                            # 验证连接被销毁
                                            mock_destroy.assert_has_calls([
                                                call(self.mock_trace_conn, self.mock_trace_curs),
                                                call(self.mock_runtime_conn, self.mock_runtime_curs)
                                            ])

    def test_parse_runtime_db_check_fails(self):
        """测试parse方法 - runtime数据库检查失败"""
        with patch.object(PathManager, 'get_db_path', return_value="/tmp/test.db"):
            with patch.object(DBManager, 'check_connect_db_path',
                              return_value=(self.mock_trace_conn, self.mock_trace_curs)):
                with patch.object(self.parser, '_check_step_trace_db', return_value=True):
                    with patch.object(DBManager, 'fetch_all_data', return_value=[(1, 100, 1000, 2000)]):
                        with patch.object(DBManager, 'check_connect_db',
                                          return_value=(self.mock_runtime_conn, self.mock_runtime_curs)):
                            with patch.object(TsTimelineRecParser, '_check_runtime_db', return_value=True):
                                with patch.object(DBManager, 'destroy_db_connect') as mock_destroy:
                                    # 执行parse
                                    self.parser.parse()

                                    # 验证连接被销毁
                                    mock_destroy.assert_has_calls([
                                        call(self.mock_trace_conn, self.mock_trace_curs),
                                        call(self.mock_runtime_conn, self.mock_runtime_curs)
                                    ])

    def test_parse_sqlite_error_handling(self):
        """测试parse方法 - SQLite错误处理"""
        with patch.object(PathManager, 'get_db_path', return_value="/tmp/test.db"):
            with patch.object(DBManager, 'check_connect_db_path',
                              return_value=(self.mock_trace_conn, self.mock_trace_curs)):
                with patch.object(self.parser, '_check_step_trace_db', return_value=True):
                    with patch.object(DBManager, 'fetch_all_data', return_value=[(1, 100, 1000, 2000)]):
                        with patch.object(DBManager, 'check_connect_db',
                                          return_value=(self.mock_runtime_conn, self.mock_runtime_curs)):
                            with patch.object(TsTimelineRecParser, '_check_runtime_db', return_value=True):
                                with patch.object(TsTimelineRecParser, '_get_ts_timeline_update_data',
                                                  side_effect=sqlite3.Error("Test SQL error")):
                                    with patch.object(DBManager, 'destroy_db_connect') as mock_destroy:
                                        import logging
                                        with patch.object(logging, 'error') as mock_log_error:
                                            # 执行parse
                                            self.parser.parse()

                                            # 验证错误被记录
                                            mock_log_error.assert_called_once()

                                            # 验证连接被销毁
                                            mock_destroy.assert_has_calls([
                                                call(self.mock_trace_conn, self.mock_trace_curs),
                                                call(self.mock_runtime_conn, self.mock_runtime_curs)
                                            ])

    def test_save_successful(self):
        """测试save方法成功情况"""
        # 设置测试数据
        test_update_data = [(5, 1, 100), (8, 2, 100), (3, 3, 200)]
        self.parser.ts_timeline_update_data = test_update_data
        self.parser.trace_conn = self.mock_trace_conn

        with patch.object(DBManager, 'executemany_sql') as mock_exec_many:
            # 执行save
            self.parser.save()

            # 验证SQL
            expected_sql = (
                "update step_trace_data set ai_core_num=? "
                "where iter_id=? and model_id=?"
            )
            mock_exec_many.assert_called_once_with(
                self.mock_trace_conn,
                expected_sql,
                test_update_data
            )

            # 验证提交
            self.mock_trace_conn.commit.assert_called_once()

    def test_save_empty_data(self):
        """测试save方法 - 空数据"""
        self.parser.ts_timeline_update_data = []
        self.parser.trace_conn = self.mock_trace_conn

        with patch.object(DBManager, 'executemany_sql') as mock_exec_many:
            # 执行save
            self.parser.save()

            # 验证没有执行SQL
            mock_exec_many.assert_not_called()
            self.mock_trace_conn.commit.assert_not_called()

    def test_ms_run_calls_parse(self):
        """测试ms_run方法调用parse"""
        with patch.object(self.parser, 'parse') as mock_parse:
            # 执行ms_run
            self.parser.ms_run()

            # 验证parse被调用
            mock_parse.assert_called_once()

    def test_class_inheritance(self):
        """测试类继承关系"""
        # 验证继承关系
        self.assertIsInstance(self.parser, IParser)
        self.assertIsInstance(self.parser, MsMultiProcess)

        # 验证方法存在
        self.assertTrue(hasattr(self.parser, 'parse'))
        self.assertTrue(hasattr(self.parser, 'save'))
        self.assertTrue(hasattr(self.parser, 'ms_run'))
        self.assertTrue(hasattr(self.parser, '_check_step_trace_db'))


if __name__ == '__main__':
    unittest.main()
