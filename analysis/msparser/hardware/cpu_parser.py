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
import multiprocessing
import os
import sqlite3
import traceback
from functools import reduce
from operator import add

from common_func import multi_process_cb
from common_func.common import CommonConstant
from common_func.common import call_sys_exit
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.file_manager import FileManager
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_ctrl_cpu_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import error
from common_func.msvp_common import get_cpu_event_chunk
from common_func.msvp_common import is_valid_original_data
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager


def create_originaldatatable(curs: any, table_name: str) -> int:
    """
    create OriginalData table
    :param curs: sqlite curs
    :param table_name: table name
    :return: result of creating table
    """
    sql = DBManager.sql_create_general_table(table_name, 'OriginalData', ConfigManager.TABLES)
    if not sql:
        return NumberConstant.ERROR
    try:
        curs.execute(sql)
    except sqlite3.Error as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        error(os.path.basename(__file__), str(err))
        return NumberConstant.ERROR
    return NumberConstant.SUCCESS


def create_eventcounttable(curs: any, pmu_events: list, tablename: str = 'EventCount') -> None:
    """
    create EventCount table
    :param curs: sqlite curs
    :param pmu_events: pmu events
    :param tablename: table name
    :return: None
    """
    sql = (
        "CREATE TABLE IF NOT EXISTS " + tablename + "(func text,module text,callstack text,"
        "common text,pid INT,tid INT,core INT," + ",".join(pmuevent + " INT" for pmuevent in pmu_events) + ")"
    )
    curs.execute(sql)


def sql_insert_eventcounttable(pmu_events: list) -> str:
    """
    generate sql statement
    :param pmu_events: pmu event
    :return: sql
    """
    insert_statement = "INSERT INTO EventCount SELECT function,module,callstack,common,pid,tid,core,"
    group_statement = " FROM OriginalData GROUP BY function,module,callstack,common,pid,tid,core"
    pmu_events_sql = list(
        "SUM(CASE WHEN pmuevent='" + pmuevent + "' THEN pmucount ELSE 0 END)" for pmuevent in pmu_events
    )
    sql = insert_statement + ",".join(pmu_events_sql) + group_statement
    return sql


def insert_eventcounttable(conn: any, curs: any, pmu_events: list) -> None:
    """
    insert data into EventCount able
    :param conn: sqlite connect
    :param curs: sqlite cursor
    :param pmu_events: pmu event
    :return: None
    """
    sql = sql_insert_eventcounttable(pmu_events)
    logging.info('start insert into EventCount')
    try:
        curs.execute(sql)
    except sqlite3.Error as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
    else:
        conn.commit()
        logging.info('end insert into EventCount')


def create_hotinstable(curs: any, pmu_events: list) -> None:
    """
    create HotIns table
    """

    field_list = []
    for pmuevent in pmu_events:
        field_list.append((pmuevent, "INT"))

    field_list.insert(Constant.DEFAULT_START, ("ip", "TEXT"))
    field_list.extend([("pid", "INT"), ("tid", "INT"), ("core", "INT"), ("function", "TEXT"), ("module", "TEXT")])
    sql = "CREATE TABLE IF NOT EXISTS HotIns ({0})".format(",".join(field[0] + " " + field[1] for field in field_list))
    try:
        curs.execute(sql)
    except sqlite3.Error as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)


def sql_insert_hotinstable(pmu_events: list) -> str:
    """
    generate sql statement
    """
    pmu_sql = list("SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)" for _ in range(len(pmu_events)))
    sql = (
        "INSERT INTO HotIns SELECT ip," + ",".join(pmu_sql) + ",pid,tid,core,function,module FROM "
        "OriginalData GROUP BY pid,tid,core,ip,function,module"
    )
    return sql


def insert_hotinstable(conn: any, curs: any, pmu_events: list) -> None:
    """
    insert data into HotIns Table
    """
    logging.info("start insert into HotIns")
    sql = sql_insert_hotinstable(pmu_events)
    try:
        curs.execute(sql, pmu_events)
    except sqlite3.Error as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
    else:
        conn.commit()
        logging.info("end insert into HotIns")


def get_cpu_pmu_events(sample_info: dict, cpu_type: str) -> list:
    """
    get cpu pmu events
    """
    pmu_events = get_cpu_event_chunk(sample_info, cpu_type)
    reduced_pmu_events = reduce(add, pmu_events)
    pmu_events_list = Utils.generator_to_list(event.replace("0x", "r") for event in reduced_pmu_events)
    return pmu_events_list


class ParsingCPUData(MsMultiProcess):
    """
    parsing cpu data file(base class)
    """

    FILE_NAME = os.path.basename(__file__)
    FILE_SIZE = 10

    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.curs = None
        self.conn = None
        self.type = 'cpu'
        self.dbname = "cpu.db"
        self.patterns = get_ctrl_cpu_compiles()
        self._file_list = []

    @classmethod
    def get_cpu_id(cls: any, sample_config: dict, cpu_type: str) -> str:
        """
        get cpu id from info.json
        :param sample_config: sample config
        :param cpu_type: cpu type
        :return: cpu id
        """
        project_path = sample_config.get("result_dir")
        if not os.path.exists(project_path):
            logging.info("No project path found in %s", CommonConstant.SAMPLE_JSON)
            error(cls.FILE_NAME, "No project path found in {}".format(CommonConstant.SAMPLE_JSON))
            return ''
        cpu_id = InfoConfReader().get_data_under_device("{}_cpu".format(cpu_type))
        if not cpu_id:
            return ''
        try:
            return cpu_id if all(i.isdigit() for i in cpu_id.split(',')) else ''
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return ''
        finally:
            pass

    def init_cpu_db(self: any) -> None:
        """
        inti cpu db file
        """
        project_path = self.sample_config.get("result_dir")
        if not os.path.exists(project_path):
            logging.info("No project path found in %s", CommonConstant.SAMPLE_JSON)
            error(self.FILE_NAME, "No project path found in {}".format(CommonConstant.SAMPLE_JSON))
            call_sys_exit(NumberConstant.ERROR)
        db_path = os.path.join(project_path, "sqlite")
        check_path_valid(db_path, False)
        self.conn, self.curs = DBManager.create_connect_db(os.path.join(db_path, self.dbname))
        if not (self.conn and self.curs):
            logging.error("Failed to connect to the database: %s", self.dbname)
            return
        try:
            self.curs.execute("PRAGMA page_size=8192")
        except sqlite3.Error:
            logging.error(traceback.format_exc(), exc_info=Constant.TRACE_BACK_SWITCH)
            self.conn.close()
            call_sys_exit(NumberConstant.ERROR)
        else:
            try:
                self.curs.execute("PRAGMA journal_mode=WAL")
            except sqlite3.Error:
                logging.error(traceback.format_exc(), exc_info=Constant.TRACE_BACK_SWITCH)
                self.conn.close()
                call_sys_exit(NumberConstant.ERROR)
            else:
                self.conn.commit()

    def parsing_data_file(self: any, cpuid: str, data_path: str) -> int:
        """
        parsing cpu data file
        """
        project_path = self.sample_config.get("result_dir", "")
        if not os.path.exists(project_path):
            logging.info("No project path found in %s", CommonConstant.SAMPLE_JSON)
            error(self.FILE_NAME, "No project path found in {}".format(CommonConstant.SAMPLE_JSON))
            return NumberConstant.ERROR
        ret, start_time = self._get_start_time()
        if ret:
            return ret
        try:
            self._multiprocess(data_path, project_path, cpuid)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError):
            logging.error(traceback.format_exc())
            return NumberConstant.ERROR
        try:
            replay_start = self.curs.execute(
                "select timestamp from OriginalData where replayid=? order by timestamp limit 1", (0,)
            ).fetchone()
        except sqlite3.Error:
            logging.error(traceback.format_exc(), exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR
        else:
            try:
                if start_time and replay_start:
                    self.curs.execute(
                        "UPDATE OriginalData SET timestamp=timestamp-? WHERE replayid=?",
                        (replay_start[0] - start_time, 0),
                    )
            except sqlite3.Error:
                logging.error(traceback.format_exc(), exc_info=Constant.TRACE_BACK_SWITCH)
                return NumberConstant.ERROR
            else:
                self.conn.commit()
                FileManager.add_complete_file(project_path, data_path.rsplit('/', 1)[-1])
                return NumberConstant.SUCCESS

    def create_other_table(self: any) -> None:
        """
        create event count table and hot ins table
        """
        pmu_events = get_cpu_pmu_events(self.sample_config, 'ai_ctrl_cpu')

        try:
            if not DBManager.judge_table_exist(self.curs, 'EventCount'):
                create_eventcounttable(self.curs, pmu_events)
        except sqlite3.Error:
            logging.error(traceback.format_exc(), exc_info=Constant.TRACE_BACK_SWITCH)
        else:
            insert_eventcounttable(self.conn, self.curs, pmu_events)
            create_hotinstable(self.curs, pmu_events)
            insert_hotinstable(self.conn, self.curs, pmu_events)
        finally:
            self.conn.commit()
            self.conn.close()

    def start_create_cpu_db(self: any) -> None:
        """
        get the control cpu related data and create corresponding tables
        """
        try:
            self.init_and_parsing()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as reason:
            logging.exception("System failed to analysis data: %s", reason)
            error(self.FILE_NAME, "System failed to analysis {0} data: {1}".format(self.type, reason))

    def init_and_parsing(self: any) -> None:
        """
        init db file and parsing data into db
        """
        project_path = self.sample_config.get("result_dir")
        if not os.path.exists(project_path):
            logging.info("No project path found in %s", CommonConstant.SAMPLE_JSON)
            error(self.FILE_NAME, "No project path found in {}".format(CommonConstant.SAMPLE_JSON))
            return
        if not self._do_parse(project_path):
            logging.error(self.FILE_NAME, "cpu parse error, path is {}".format(project_path))

    def ms_run(self: any) -> None:
        """
        entrance for cpu parser
        """
        try:
            if self._file_list:
                self.start_create_cpu_db()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            if isinstance(self.conn, sqlite3.Connection):
                self.conn.close()

    def _multiprocess(self: any, data_path: str, project_path: str, cpuid: str) -> None:
        processes = []
        lock = multiprocessing.Lock()
        for i in range(self.FILE_SIZE):
            kwargs = {
                "replayid": 0,
                "filename": data_path,
                "id": cpuid,
                "start_pos": i * os.path.getsize(data_path) / self.FILE_SIZE,
                "end_pos": (i + 1) * os.path.getsize(data_path) / self.FILE_SIZE,
                "dbname": os.path.join(project_path, "sqlite", self.dbname),
                "pro_no": i,
                "lock": lock,
            }
            pro = multiprocessing.Process(target=multi_process_cb.multiprocess_callback, args=(kwargs,))
            pro.start()
            processes.append(pro)
        for pro_ in processes:
            pro_.join()

    def _do_parse(self: any, project_path: str) -> bool:
        for file in os.listdir(os.path.join(project_path, "data")):
            if not (get_file_name_pattern_match(file, *self.patterns) and is_valid_original_data(file, project_path)):
                continue
            cpu_id = self.get_cpu_id(self.sample_config, self.type)
            if cpu_id == '':
                logging.error('failed to get %s cpu id', self.type)
                return False
            # create database
            self.init_cpu_db()
            data_path = os.path.join(project_path, "data", file)
            if os.path.getsize(data_path) != 0:
                # replay id is 0
                status = self.parsing_data_file(cpu_id, data_path)
                if status == NumberConstant.ERROR:
                    return False
            # create EventCount table and insert data
            self.create_other_table()
        return True

    def _get_start_time(self: any) -> tuple:
        # test the existence of OriginalData table
        if self.curs.execute(
            "select count(*) from sqlite_master where type='table' and name='OriginalData'"
        ).fetchone()[0]:
            start_time = self.curs.execute("select timestamp from OriginalData order by timestamp limit 1")
            start_time = start_time.fetchone()
            if start_time:
                start_time = start_time[0]
            else:
                start_time = Constant.DEFAULT_START
        else:
            status = create_originaldatatable(self.curs, "OriginalDataMap")
            if status:
                return status, 0
            self.curs.execute("CREATE INDEX pmuevent_index ON OriginalData(pmuevent)")
            self.curs.execute("CREATE INDEX timestamp_index ON OriginalData(timestamp)")
            start_time = 0
        return NumberConstant.SUCCESS, start_time
