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
import struct
import unittest
from unittest import mock
from unittest.mock import Mock

from common_func.create_runtime_db import ParsingRuntimeData
from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager

NAMESPACE = 'common_func.create_runtime_db'


class TestParsingRuntimeData(unittest.TestCase):
    sample_config = {}

    def test_update(self):
        api_data = [(1, 2, 3, 'a,b,c', 4)]
        check = ParsingRuntimeData(self.sample_config, CONFIG)
        result = check.update(api_data)
        self.assertEqual(result, [(1, 2, 3, 'a'), (1, 2, 3, 'b'), (1, 2, 3, 'c')])

    def test_time_line(self):
        data = struct.pack("=BBHLHHHHQLL", 1, 1, 1, 1, 7, 1, 1, 0, 101010019214, 4294967295, 0)
        wrong_data = struct.pack("=BBHLHHHHQL", 1, 1, 1, 1, 7, 1, 1, 0, 101010019214, 4294967295)
        check = ParsingRuntimeData(self.sample_config, CONFIG)
        check._time_line(1, 3, wrong_data, 'ts_track.data.0.slice_0')
        check._time_line(1, 3, data, 'ts_track.data.0.slice_0')
        self.assertEqual(check.rts_data["time_line"],
                         [('0', 7, 0, 1, 1, 101010019214.0, 4294967295, '0', 1)])

    def test_event_count(self):
        data = struct.pack("=BBHLHHHHQ8QQQH3H", 1, 1, 1, 1, 0, 5, 3, 0, 18446744073709551615, 26050, 0, 526, 0, 14002,
                           26675, 224, 12,
                           58006, 101612566070, 2, 0, 0, 0)
        wrong_data = struct.pack("=BBHLHHHHQ8QQQH2H", 1, 1, 1, 1, 0, 5, 3, 0, 18446744073709551615, 26050, 0, 526, 0,
                                 14002, 26675, 224,
                                 12, 58006, 101612566070, 2, 0, 0)
        check = ParsingRuntimeData(self.sample_config, CONFIG)
        check._event_count(1, 4, wrong_data, 'aicore.data.0.slice_0')
        check._event_count(1, 4, data, 'aicore.data.0.slice_0')
        self.assertEqual(check.rts_data["event_count"],
                         [('0', 0, 3, 5, 0, 0, 101612566070, 26050, 0, 526, 0, 14002, 26675, 224, 12,
                           58006, 2, 0, '0', 1)])

    def test_step_trace_status_tag(self):
        data = struct.pack("=BBHLQQQHHHH", 1, 1, 1, 1, 101612167908, 1, 1, 3, 1, 0, 0)
        wrong_data = struct.pack("=BBHLQQQHH", 1, 1, 1, 1, 101612167908, 1, 1, 3, 1)
        check = ParsingRuntimeData(self.sample_config, CONFIG)
        check._step_trace_status_tag(1, 10, wrong_data, 'ts_track.data.0.slice_0')
        check._step_trace_status_tag(1, 10, data, 'ts_track.data.0.slice_0')
        self.assertEqual(check.rts_data["step_trace"],
                         [(1, 1, 101612167908.0, 3, 1, 0)])

    def test_ts_memcpy_tag(self):
        # timestamp, stream id, task id, task state, reserve 0, reserve 1, reserve 2, reserve 3
        data = struct.pack("=BBHLQHHBBHQQ", 0, 1, 2, 3, 101612167908, 10, 10, 0, 1, 0, 0, 0)
        wrong_data = struct.pack("=BBHLQQQHH", 0, 1, 2, 3, 101612167908, 1, 1, 3, 1)
        check = ParsingRuntimeData(self.sample_config, CONFIG)
        check._ts_memcpy_tag(1, 10, wrong_data, 'ts_track.data.0.slice_0')
        check._ts_memcpy_tag(1, 10, data, 'ts_track.data.0.slice_0')
        self.assertEqual(check.rts_data["ts_memcpy"],
                         [(101612167908.0, 10, 10, 0)])

    def test_create_runtime_db(self):
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        with mock.patch(NAMESPACE + '.ParsingRuntimeData._start_parsing_data_file',
                        return_value=(1, 'fail')), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check.create_runtime_db()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.ParsingRuntimeData._start_parsing_data_file',
                        return_value=(0, 'fail')):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check.create_runtime_db()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.ParsingRuntimeData._start_parsing_data_file',
                        return_value=(0, 'success')), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                           return_value=True), \
                mock.patch(NAMESPACE + '.create_ai_event_tables'), \
                mock.patch(NAMESPACE + '.insert_event_value'), \
                mock.patch(NAMESPACE + '.StepTableBuilder.run'), \
                mock.patch(NAMESPACE + '.logging.info'):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            check.conn = res[0]
            check.create_runtime_db()

    def test_create_timeline_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        sql = "CREATE TABLE IF NOT EXISTS TimeLine(replayid INTEGER,tasktype INTEGER," \
              "task_id INTEGER,stream_id INTEGER,taskstate INTEGER,timestamp INTEGER," \
              "thread INTEGER,device_id INTEGER,mode INTEGER)"
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=sql):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.rts_data['api_call'] = \
                [('0', 7, 0, 1, 1, 101010019214.0, 4294967295, '0', 1)]
            result = check.create_timeline_table('TimeLineMap')
            self.assertEqual(result, None)
        res[1].execute('drop table TimeLine')
        db_manager.destroy(res)

    def test_create_event_counter_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        sql = "CREATE TABLE IF NOT EXISTS EventCounter(replayid INTEGER,tasktype INTEGER," \
              "task_id INTEGER,stream_id INTEGER,overflow INTEGER,overflowcycle INTEGER," \
              "timestamp INTEGER,event1 TEXT,event2 TEXT,event3 TEXT,event4 TEXT,event5 TEXT," \
              "event6 TEXT,event7 TEXT,event8 TEXT,task_cyc TEXT,block INTEGER,thread INTEGER," \
              "device_id INTEGER,mode INTEGER)"
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=sql):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.rts_data['event_count'] = [
                ('0', 0, 3, 5, 0, 0, 101612566070, 26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 2, 0, '0', 1)
            ]
            result = check.create_event_counter_table('EventCounterMap')
            self.assertEqual(result, None)
        res[1].execute('drop table EventCounter')
        res[0].commit()
        db_manager.destroy(res)

    def test_create_step_trace_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('step_trace.db')
        sql = "CREATE TABLE IF NOT EXISTS StepTrace(index_id INTEGER,model_id INTEGER,timestamp REAL," \
              "stream_id INTEGER,task_id INTEGER,tag_id INTEGER)"
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\test'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=res), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                               return_value=sql):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                check.rts_data["step_trace"] = [(1, 1, 101612167908.0, 3, 1, 0)]
                check.create_step_trace_table('StepTraceMap')
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=(False, False)):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.create_step_trace_table('StepTraceMap')
                self.assertEqual(result, None)
        res[1].execute('drop table StepTrace')
        res[0].commit()
        db_manager.destroy(res)

    def test_create_ts_memcpy_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('step_trace.db')
        sql = "CREATE TABLE IF NOT EXISTS TsMemcpy(timestamp REAL, " \
              "stream_id INTEGER, task_id INTEGER, task_state INTEGER )"
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\test'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=res), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                               return_value=sql):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                check.rts_data["ts_memcpy"] = [(101612167908.0, 10, 10, 0)]
                check.create_step_trace_table('TsMemcpyMap')
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=(False, False)):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.create_step_trace_table('TsMemcpyMap')
                self.assertEqual(result, None)
        res[1].execute('drop table TsMemcpy')
        res[0].commit()
        db_manager.destroy(res)

    def test_insert_data(self):
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        with mock.patch(NAMESPACE + '.ParsingRuntimeData.create_timeline_table'), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData.create_event_counter_table'), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData.create_step_trace_table'):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            check.rts_data["time_line"] = [('0', 7, 0, 1, 1, 101010019214.0, 4294967295, '0', 1)]
            check.rts_data["event_count"] = [
                ('0', 0, 3, 5, 0, 0, 101612566070, 26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 2, 0, '0', 1)
            ]
            check.rts_data["step_trace"] = [(1, 1, 101612167908.0, 3, 1, 0)]
            check.conn, check.curs = res[0], res[1]
            check.insert_data()
        db_manager.destroy(res)

    def test_read_binary_data(self):
        data_1 = struct.pack("=BBH", 1, 11, 112)
        data_2 = struct.pack("=BBH", 1, 12, 0)
        data_3 = struct.pack("=BBHLHHHHQ8QQQH3HB", 1, 4, 112, 200, 0, 5, 3, 0, 18446744073709551615,
                             26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 101612566070, 2, 0, 0, 0, 0)
        data_4 = struct.pack("=BBHLHHHHQ8QQQH3H", 1, 4, 112, 200, 0, 5, 3, 0, 18446744073709551615,
                             26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 101612566070, 2, 0, 0, 0)
        target_is_root = 'common_func.file_manager.is_root_user'
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch(target_is_root, return_value=True), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData.insert_data'), \
                mock.patch(NAMESPACE + '.logging.info'):
            with mock.patch('builtins.open', mock.mock_open(read_data=data_1)), \
                    mock.patch('os.path.getsize', return_value=200), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=True), \
                    mock.patch('os.access', return_value=True), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.read_binary_data('runtime.api.0.slice_0', b'')
            self.assertEqual(result, b'\x01\x0bp\x00')
            with mock.patch('builtins.open', mock.mock_open(read_data=data_2)), \
                    mock.patch('os.path.getsize', return_value=200), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=True), \
                    mock.patch('os.access', return_value=True), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.read_binary_data('runtime.api.0.slice_0', b'')
            self.assertEqual(result, b'')
            with mock.patch('builtins.open', mock.mock_open(read_data=data_3)), \
                    mock.patch('os.path.getsize', return_value=200), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=True), \
                    mock.patch('os.access', return_value=True), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.read_binary_data('runtime.api.0.slice_0', None)
            self.assertEqual(result, b'\x00')
            with mock.patch('builtins.open', mock.mock_open(read_data=data_4)), \
                    mock.patch('os.path.getsize', return_value=200), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=True), \
                    mock.patch('os.access', return_value=True), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = ParsingRuntimeData(self.sample_config, CONFIG)
                result = check.read_binary_data('runtime.api.0.slice_0', None)
            self.assertEqual(result, b'')

    def test_check_file_match(self):
        CONFIG['ai_core_profiling_mode'] = "task-based"
        with mock.patch(NAMESPACE + '.is_valid_original_data',
                        return_value=True):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._check_file_match('ts_track.data.0.slice_0', 'test')
            self.assertTrue(result)
            result = check._check_file_match('ts_track.aiv_data.0.slice_0', 'test')
            self.assertTrue(result)
            result = check._check_file_match('aicore.data.0.slice_0', 'test')
            self.assertTrue(result)
            result = check._check_file_match('test', 'test')
            self.assertFalse(result)
        with mock.patch(NAMESPACE + '.is_valid_original_data',
                        return_value=True):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._check_file_match('aiVectorCore.data.0.slice_0', 'test')
            self.assertFalse(result)
            CONFIG['aiv_profiling_mode'] = "task-based"
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._check_file_match('aiVectorCore.data.0.slice_0', 'test')
            self.assertFalse(result)

    def test_start_parsing_data_file(self):
        wrong_file = ['test']
        file_all = ['aicore.data.0.slice_0', 'runtime.api.0.slice_0', 'ts_track.data.0.slice_0']
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        with mock.patch(NAMESPACE + '.ParsingRuntimeData._do_parse_data_file',
                        side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._start_parsing_data_file()
        self.assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.get_data_dir_sorted_files', return_value=wrong_file), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData._check_file_match',
                           return_value=False):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._start_parsing_data_file()
        self.assertEqual(result, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.get_data_dir_sorted_files', return_value=file_all), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData._check_file_match',
                           return_value=True), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value='test\\db'), \
                mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.ParsingRuntimeData.read_binary_data',
                           return_value=b'\x00'):
            check = ParsingRuntimeData(self.sample_config, CONFIG)
            result = check._start_parsing_data_file()
        db_manager.destroy(res)
        self.assertEqual(result, 0)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingRuntimeData.create_runtime_db'):
            ParsingRuntimeData(self.sample_config, CONFIG).ms_run()