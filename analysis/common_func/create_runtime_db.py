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

import logging
import os
import sqlite3
import struct

from common_func.common import get_data_dir_sorted_files
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_name_manager import get_ai_core_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_ts_track_aiv_compiles
from common_func.file_name_manager import get_ts_track_compiles
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from mscalculate.step_trace.create_step_table import StepTableBuilder
from msconfig.config_manager import ConfigManager
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.event_counter import AiCoreTaskInfo
from profiling_bean.struct_info.step_trace import StepTrace
from profiling_bean.struct_info.ts_memcpy import TsMemcpy
from profiling_bean.struct_info.ts_time_line import TimeLineData
from viewer.calculate_rts_data import create_ai_event_tables
from viewer.calculate_rts_data import insert_event_value


class ParsingRuntimeData(MsMultiProcess):
    """
    parsing runtime data
    """

    FILE_NAME = os.path.basename(__file__)
    TABLE_PATH = ConfigManager.TABLES

    # task state
    # tag
    API_CALL = 0
    TIME_LINE = 3
    EVENT_COUNT = 4
    TS_USAGE_TAG = 7
    AI_CORE_STATUS_TAG = 8
    AIV_CORE_STATUS_TAG = 9
    STEP_TRACE_TAG = 10
    TS_MEMCPY_TAG = 11
    RUNTIME_TS_TAG = 6
    HEADER = 4
    DETAIL = 64
    AI_CORE_TYPE_BY_HWTS = "0x6bd3"  # ai core data by hwts, third and fouth byte is 6bd3

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.file_list = file_list
        self.curs = None
        self.conn = None
        self.ai_core_by_hwts = False  # hwts report ai core data
        self.delta_dev = 0
        self.rts_data = {"time_line": [], "event_count": [], "step_trace": [], "ts_memcpy": []}
        self.parse_info = {"device_id_list": [], "replayid": '0', "device_id": '0'}

    @staticmethod
    def update(api_data: list) -> list:
        """
        api update data
        :param api_data:api data
        :return:
        """
        api_down_task = []
        for api in api_data:
            # API data is in the format of api,rowid，stream_id，task_id
            task_id = api[3].split(',')
            for task in task_id:
                api_down_task.append(api[0:3] + (task,))
        return api_down_task

    @staticmethod
    def _running_ts_tag(*args: any) -> None:
        """
        Ignore running_ts_tag struct
        :param args: running ts
        :return: None
        """

    @staticmethod
    def _ts_usage_tag(*args: any) -> None:
        """
        TS CPU usage is irrelevant in this scripts, then we ignore it
        :param args: ts cpu usage
        :return: None
        """

    @staticmethod
    def _ai_core_status_tag(*args: any) -> None:
        """
        Ignore ai core status struct
        :param args: ai core status
        :return: None
        """

    @staticmethod
    def _ai_vector_status_tag(*args: any) -> None:
        """
        Ignore ai vector core status struct
        :param args: ai vector core status
        :return: None
        """

    @staticmethod
    def _read_binary_align_reserved(file_: any, fmt: str) -> None:
        """
        reserved bytes for binary alignment
        :param file_: file reader
        :param fmt: binary data format
        :return: None
        """
        default_max_byte = "Q"
        union_fmt = fmt + default_max_byte
        reversed_bytes = struct.calcsize(union_fmt) - struct.calcsize(StructFmt.BYTE_ORDER_CHAR + union_fmt)
        if reversed_bytes:
            file_.read(reversed_bytes)

    def create_runtime_db(self: any) -> None:
        """
        create runtime database
        :return: None
        """
        status = self._start_parsing_data_file()
        if status == NumberConstant.ERROR or not self.conn:
            return
        if DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_EVENT_COUNTER):
            create_ai_event_tables(self.sample_config, self.curs, self.parse_info.get("device_id"))
            insert_event_value(self.curs, self.conn, self.parse_info.get("device_id"))

        if ChipManager().is_chip_v1():
            StepTableBuilder.run(self.sample_config)
        logging.info("Create Runtime DB finished!")
        DBManager.destroy_db_connect(self.conn, self.curs)

    def create_timeline_table(self: any, table_map_name: str) -> None:
        """
        create timeline table
        :param table_map_name: timeline mapping name
        :return: None
        """
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_RUNTIME_TIMELINE):
            sql = DBManager.sql_create_general_table(
                table_map_name, DBNameConstant.TABLE_RUNTIME_TIMELINE, self.TABLE_PATH
            )
            DBManager.execute_sql(self.conn, sql)
        timeline_sql = 'insert into {table_name} ({column}) values (?,?,?,?,?,?,?,?,?)'.format(
            table_name=DBNameConstant.TABLE_RUNTIME_TIMELINE,
            column='replayId,taskType,task_id,stream_id,taskState,timeStamp,thread,device_id,mode',
        )
        DBManager.executemany_sql(self.conn, timeline_sql, self.rts_data.get("time_line"))

    def create_event_counter_table(self: any, table_map_name: str) -> None:
        """
        create event count table
        :param table_map_name: event count mapping name
        :return: None
        """

        column = (
            'replayId,taskType,task_id,stream_id,overflow,'
            'overflowCycle,timeStamp,{0},task_cyc,block,thread,device_id,mode'.format(
                ','.join('event{}'.format(i) for i in range(1, len(self.rts_data.get("event_count")[0]) - 11))
            )
        )
        event_sql = 'insert into {table_name} ({column}) values ({value})'.format(
            table_name=DBNameConstant.TABLE_EVENT_COUNTER,
            column=column,
            value=('?,' * (len(self.rts_data.get("event_count")[0]) - 1) + '?'),
        )

        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_EVENT_COUNTER):
            sql = DBManager.sql_create_general_table(
                table_map_name, DBNameConstant.TABLE_EVENT_COUNTER, self.TABLE_PATH
            )
            DBManager.execute_sql(self.conn, sql)
        DBManager.executemany_sql(self.conn, event_sql, self.rts_data.get("event_count"))

    def create_step_trace_table(self: any, table_map_name: str) -> None:
        """
        create step trace table
        :param table_map_name: Step trace Map
        :return: NA
        """
        db_path = PathManager.get_db_path(self.sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE)
        step_conn, step_curs = DBManager.create_connect_db(db_path)
        if not step_conn or not step_curs:
            return
        if not DBManager.judge_table_exist(step_curs, DBNameConstant.TABLE_STEP_TRACE):
            sql = DBManager.sql_create_general_table(table_map_name, DBNameConstant.TABLE_STEP_TRACE, self.TABLE_PATH)
            # The range of iteration id start with 1.
            DBManager.execute_sql(step_conn, sql)
        DBManager.insert_data_into_table(step_conn, DBNameConstant.TABLE_STEP_TRACE, self.rts_data.get("step_trace"))
        DBManager.destroy_db_connect(step_conn, step_curs)

    def create_ts_memcpy_table(self: any, table_map_name: str) -> None:
        """
        create ts memcpy table
        :param table_map_name: Ts Memcpy Map
        :return: NA
        """
        db_path = PathManager.get_db_path(self.sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE)
        memcpy_conn, memcpy_curs = DBManager.create_connect_db(db_path)
        if not memcpy_conn or not memcpy_curs:
            return
        if not DBManager.judge_table_exist(memcpy_curs, DBNameConstant.TABLE_TS_MEMCPY):
            sql = DBManager.sql_create_general_table(table_map_name, DBNameConstant.TABLE_TS_MEMCPY, self.TABLE_PATH)
            DBManager.execute_sql(memcpy_conn, sql)
        DBManager.insert_data_into_table(memcpy_conn, DBNameConstant.TABLE_TS_MEMCPY, self.rts_data.get("ts_memcpy"))
        DBManager.destroy_db_connect(memcpy_conn, memcpy_curs)

    def insert_data(self: any) -> None:
        """
        insert data into created tables
        :return: None
        """
        if self.rts_data.get("time_line"):
            self.create_timeline_table('TimeLineMap')
        if self.rts_data.get("event_count"):
            self.create_event_counter_table('EventCounterMap')
        if self.rts_data.get("step_trace"):
            self.create_step_trace_table('StepTraceMap')
        if self.rts_data.get("ts_memcpy"):
            self.create_ts_memcpy_table('TsMemcpyMap')
        del self.rts_data.get("time_line")[:]
        del self.rts_data.get("event_count")[:]

    def read_binary_data(self: any, binary_data_path: str, legacy_bytes: bytes) -> bytes:
        """
        parsing binary data
        :param binary_data_path: binary data file path
        :param legacy_bytes: bytes remind last file
        :return:
        """
        project_path = self.sample_config.get("result_dir")
        file_name = PathManager.get_data_file_path(project_path, binary_data_path)
        legacy_bytes = self._do_read_binary_data(file_name, binary_data_path, legacy_bytes)
        try:
            self.insert_data()
        except sqlite3.Error as err:
            logging.error("%s: %s", binary_data_path, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return legacy_bytes
        logging.info("End parsing rts data file: %s", os.path.basename(file_name))
        return legacy_bytes

    def ms_run(self: any) -> None:
        """
        entrance for data parser
        :return: None
        """
        try:
            self.create_runtime_db()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _time_line(self: any, *args: any) -> None:
        mode = args[0]
        try:
            time_line_data = TimeLineData.decode(args[2])
        except struct.error:
            logging.error('Get time line original data error. ', exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.rts_data.get("time_line").append(
            (
                self.parse_info.get("replayid"),
                time_line_data.task_type,
                time_line_data.task_id,
                time_line_data.stream_id,
                time_line_data.task_state,
                time_line_data.time_stamp,
                time_line_data.thread,
                self.parse_info.get("device_id"),
                mode,
            )
        )

    def _start_parsing_data_file(self: any) -> int:
        """
        start parsing the data
        :return: result or data parse
        """
        try:
            return self._do_parse_data_file()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def _step_trace_status_tag(self: any, *args: any) -> None:
        """
        analysis step trace data
        :param args: bytes for step trace
        :return: None
        """
        try:
            step_trace = StepTrace.decode(args[2])
        except struct.error:
            logging.error('Get step trace original data error. ', exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.rts_data.get("step_trace").append(
            (
                step_trace.index_id,
                step_trace.model_id,
                step_trace.timestamp,
                step_trace.stream_id,
                step_trace.task_id,
                step_trace.tag_id,
            )
        )

    def _event_count(self: any, *args: any) -> None:
        try:
            ai_core_pmu = AiCoreTaskInfo.decode(args[2])
        except struct.error:
            logging.error('Get event count original data error. ', exc_info=Constant.TRACE_BACK_SWITCH)
            return
        # when overflow=0, overflowCycle is a number full of F.
        # The number is bigger than sqlite's scope
        if ai_core_pmu.counter_info.overflow == 0:
            ai_core_pmu.counter_info.overflow_cycle = 0  # set overflowCycle to 0
        self.rts_data.get("event_count").append(
            tuple(
                [
                    self.parse_info.get("replayid"),
                    ai_core_pmu.task_type,
                    ai_core_pmu.task_id,
                    ai_core_pmu.stream_id,
                    ai_core_pmu.counter_info.overflow,
                    ai_core_pmu.counter_info.overflow_cycle,
                    ai_core_pmu.counter_info.time_stamp,
                ]
                + Utils.generator_to_list(i for i in ai_core_pmu.counter_info.event_counter)
                + [
                    ai_core_pmu.counter_info.task_cyc,
                    ai_core_pmu.counter_info.block,
                    0,
                    self.parse_info.get("device_id"),
                    args[0],
                ]
            )
        )

    def _ts_memcpy_tag(self: any, *args: any) -> None:
        """
        analysis ts memcpy data
        :param args: bytes for ts memcpy
        :return: None
        """
        try:
            ts_memcpy = TsMemcpy.decode(args[2])
        except struct.error:
            logging.error('Get memory copy data error. ', exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.rts_data.get("ts_memcpy").append(
            (ts_memcpy.timestamp, ts_memcpy.stream_id, ts_memcpy.task_id, ts_memcpy.task_state)
        )

    def _do_read_binary_data(self: any, file_name: str, binary_data_path: str, legacy_bytes: any) -> any:
        parse_dct = {
            self.TIME_LINE: self._time_line,
            self.EVENT_COUNT: self._event_count,
            self.RUNTIME_TS_TAG: self._running_ts_tag,
            self.TS_USAGE_TAG: self._ts_usage_tag,
            self.AI_CORE_STATUS_TAG: self._ai_core_status_tag,
            self.AIV_CORE_STATUS_TAG: self._ai_vector_status_tag,
            self.STEP_TRACE_TAG: self._step_trace_status_tag,
            self.TS_MEMCPY_TAG: self._ts_memcpy_tag,
        }

        with FileOpen(file_name, 'rb') as file_reader:
            # File size security is guaranteed by external calls
            if legacy_bytes is None:
                legacy_bytes = bytes()
            binary_file_data = legacy_bytes + file_reader.file_reader.read(os.path.getsize(file_name))
            # this offset record offset to func, calculate by bufsize
            binary_data_size = len(binary_file_data)
            offset = 0
            while offset < binary_data_size:
                # if remain_size less then header size (4bytes) break
                if binary_data_size - offset < struct.calcsize(StructFmt.RUNTIME_HEADER_FMT):
                    legacy_bytes = binary_file_data[offset:]
                    break
                mode, tag, bufsize = struct.unpack_from(StructFmt.RUNTIME_HEADER_FMT, binary_file_data, offset)
                # if last data len is less then bufsize, save it
                if binary_data_size - offset < bufsize:
                    legacy_bytes = binary_file_data[offset:]
                    break
                func_ = parse_dct.get(tag)
                if not func_:
                    logging.error("invalid tag: %s, %s", binary_data_path, tag)
                    break
                func_(mode, tag, binary_file_data[offset : offset + bufsize], binary_data_path)
                offset += bufsize
                # complete file, set empty legacy bytes
                if offset == binary_data_size:
                    legacy_bytes = bytes()
        return legacy_bytes

    def _check_file_with_task_based(self: any, core_patterns: tuple, core_profiling_mode: str, file_name: str) -> any:
        """
        check file with task based
        :return: match result
        """
        if self.sample_config.get(core_profiling_mode) == StrConstant.AIC_TASK_BASED_MODE:
            return True, get_file_name_pattern_match(file_name, *core_patterns)
        return False, {}

    def _check_file_match(self: any, file_name: str, project_path: str) -> bool:
        ts_track_compiles = get_ts_track_compiles()
        ts_track_aiv_compiles = get_ts_track_aiv_compiles()
        ts_result = get_file_name_pattern_match(file_name, *ts_track_compiles)
        ts_aiv_result = get_file_name_pattern_match(file_name, *ts_track_aiv_compiles)
        _, aicore_result = self._check_file_with_task_based(
            get_ai_core_compiles(), StrConstant.AICORE_PROFILING_MODE, file_name
        )

        if aicore_result and is_valid_original_data(file_name, project_path) and ChipManager().is_chip_v1():
            self.parse_info["device_id"] = aicore_result.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
        elif ts_result and is_valid_original_data(file_name, project_path) and ChipManager().is_chip_v1():
            self.parse_info["device_id"] = ts_result.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
        elif ts_aiv_result and is_valid_original_data(file_name, project_path):
            self.parse_info["device_id"] = ts_aiv_result.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
        else:
            return False
        return True

    def _do_parse_data_file(self: any) -> int:
        project_path = self.sample_config.get("result_dir")
        data_dir = PathManager.get_data_dir(project_path)
        file_all = get_data_dir_sorted_files(data_dir)
        legacy_bytes = bytes()
        last_data_type = " "
        for file_name in file_all:
            if not self._check_file_match(file_name, project_path):
                continue
            self.delta_dev = InfoConfReader().get_delta_time()
            FileManager.add_complete_file(project_path, file_name)
            self.parse_info.get("device_id_list").append(self.parse_info.get("device_id"))
            logging.info("start parsing rts data file: %s", file_name)
            self.conn, self.curs = DBManager.create_connect_db(
                PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME)
            )
            DBManager.execute_sql(self.conn, "PRAGMA page_size=8192")
            data_type = os.path.basename(file_name).split(".")[0]
            # change analysis data type, need clear legacy_bytes
            if last_data_type != data_type and len(legacy_bytes) > 0:
                logging.error("%s data must be lost, please check sum file size!", last_data_type)
                legacy_bytes = bytes()
            last_data_type = data_type
            legacy_bytes = self.read_binary_data(file_name, legacy_bytes)
        return NumberConstant.SUCCESS
