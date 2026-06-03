#!/usr/bin/env python3
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
import io
import unittest
from unittest import mock

from common_func.multi_process_cb import MultiProcessCbConstant
from common_func.multi_process_cb import _exec_query_sql
from common_func.multi_process_cb import _multiprocess_callback_helper
from common_func.multi_process_cb import _update_hot_func
from common_func.multi_process_cb import insert_data
from common_func.multi_process_cb import line_match
from common_func.multi_process_cb import manipulation_data
from common_func.multi_process_cb import multiprocess_callback
from common_func.multi_process_cb import process_stack

NAMESPACE = 'common_func.multi_process_cb'


class TestMultiProcessCb(unittest.TestCase):

    def test_update_hot_func_compacts_long_symbol(self):
        hot_func = []

        _update_hot_func(hot_func, ['90.0', 'very', 'hot', 'func+0x1', '(lib.so)'])

        self.assertEqual(['90.0', 'very hot func', '0x1', 'lib.so'], hot_func)

    def test_process_stack_returns_hot_func_and_stack_path(self):
        perf_out = io.StringIO('90.0 hot_func+0x1 (lib.so)\nfunc_a\nfunc_b\n\n')

        result = process_stack(perf_out)

        self.assertEqual(['90.0', 'hot_func', '0x1', 'lib.so', 'func_a<-func_b'], result)

    def test_process_stack_returns_unknown_or_empty_on_edge_cases(self):
        unknown_result = process_stack(io.StringIO('90.0 unknown_func (lib.so)\n\n'))
        self.assertEqual(['90.0', 'unknown', 'unknown', 'lib.so', 'unknown'], unknown_result)

        perf_out = mock.Mock()
        perf_out.readline.side_effect = OSError('boom')
        self.assertEqual([], process_stack(perf_out))

    def test_line_match_matches_normal_and_raw_events(self):
        info = {'cpu_id': '1'}

        matched, pmu_mode = line_match('proc 12/34 [001] 56.78: 90 event:', info)
        self.assertIsNotNone(matched)
        self.assertEqual('event', matched.groups()[-1])
        self.assertEqual(MultiProcessCbConstant.PMU_MODE_CORE, pmu_mode)

        matched, pmu_mode = line_match('proc 12/34 [001] 56.78: 90 raw 0x2:', info)
        self.assertIsNotNone(matched)
        self.assertEqual('0x2', matched.groups()[-1])
        self.assertEqual(MultiProcessCbConstant.PMU_MODE_CORE, pmu_mode)

    def test_insert_data_handles_core_and_no_core_modes(self):
        matched = mock.Mock()
        matched.groups.return_value = ['proc', '12', '34', '001', '56.78', '90', '0x20']
        with mock.patch(NAMESPACE + '.process_stack', return_value=['hot', 'stack']):
            no_core_info = {
                'pmu_mode': MultiProcessCbConstant.PMU_MODE_NO_CORE,
                'matched': matched,
                'sample': [],
                'samples': [],
                'replayid': 7,
                'sample_count': 11,
                'query': 'sql',
            }
            samples, sample = insert_data(no_core_info, mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock())
            self.assertEqual([[ 'proc', '12', '34', -1, '001', '56.78', '90', 'r20', 'hot', 'stack', 7 ]], samples)
            self.assertEqual(['proc', '12', '34', -1, '001', '56.78', '90', 'r20', 'hot', 'stack', 7], sample)

            core_info = {
                'pmu_mode': MultiProcessCbConstant.PMU_MODE_CORE,
                'matched': matched,
                'sample': [],
                'samples': [],
                'replayid': 8,
                'sample_count': 10,
                'query': 'sql',
            }
            samples, sample = insert_data(core_info, mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock())
            self.assertEqual([['proc', '12', '34', '001', '56.78', '90', 'r2', 'hot', 'stack', 8]], samples)
            self.assertEqual(['proc', '12', '34', '001', '56.78', '90', 'r2', 'hot', 'stack', 8], sample)

    def test_exec_query_sql_executes_each_sample_under_lock(self):
        lock = mock.Mock()
        conn = mock.Mock()
        curs = mock.Mock()

        _exec_query_sql(lock, {'query': 'insert into t values (?)'}, [(1,), (2,)], conn, curs)

        lock.acquire.assert_called_once_with()
        self.assertEqual(2, curs.execute.call_count)
        conn.commit.assert_called_once_with()
        lock.release.assert_called_once_with()

    def test_manipulation_data_skips_invalid_lines_and_collects_samples(self):
        file_obj = mock.Mock()
        file_obj.readline.side_effect = [':bad\n', 'skip\n', 'match\n']
        file_obj.tell.side_effect = [1, 3]
        info = {'end_pos': 3}
        with mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.line_match', side_effect=[(None, None), ('matched',
                                                                                  MultiProcessCbConstant.PMU_MODE_CORE)]), \
                mock.patch(NAMESPACE + '.insert_data', return_value=([['row']], [])):
            result = manipulation_data(file_obj, mock.Mock(), mock.Mock(), info, mock.Mock())

        self.assertEqual([['row']], result)
        self.assertEqual('matched', info['matched'])
        self.assertEqual(MultiProcessCbConstant.PMU_MODE_CORE, info['pmu_mode'])

    def test_multiprocess_callback_helper_executes_insert_for_remaining_samples(self):
        args = {'start_pos': 1, 'pro_no': 1}
        info = {'end_pos': 3, 'query': 'sql'}
        file_obj = io.StringIO('header\n\npayload\n\n')
        with mock.patch(NAMESPACE + '.manipulation_data', return_value=[['row']]) as manipulation_data_mock, \
                mock.patch(NAMESPACE + '._exec_query_sql') as exec_query_sql:
            _multiprocess_callback_helper(args, file_obj, info, mock.Mock(), mock.Mock(), mock.Mock())

        manipulation_data_mock.assert_called_once()
        exec_query_sql.assert_called_once()

    def test_multiprocess_callback_logs_exception_and_cleans_up(self):
        args = {
            'end_pos': 10,
            'replayid': 1,
            'id': '1',
            'lock': mock.Mock(),
            'dbname': 'runtime.db',
            'filename': 'perf.data',
            'start_pos': 0,
            'pro_no': 0,
        }
        conn = mock.Mock()
        curs = mock.Mock()
        with mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(conn, curs)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect') as destroy_db_connect, \
                mock.patch(NAMESPACE + '.FileOpen') as file_open, \
                mock.patch(NAMESPACE + '._multiprocess_callback_helper', side_effect=RuntimeError('boom')) as helper, \
                mock.patch(NAMESPACE + '.logging.exception') as logging_exception, \
                mock.patch(NAMESPACE + '.logging.info'):
            file_open.return_value.__enter__.return_value.file_reader = mock.Mock()
            multiprocess_callback(args)

        helper.assert_called_once()
        logging_exception.assert_called_once()
        destroy_db_connect.assert_called_once_with(conn, curs)


if __name__ == '__main__':
    unittest.main()
