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
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msparser.interface.iparser import IParser


class TsTimelineRecParser(IParser, MsMultiProcess):
    """
    class used to parse ts timeline rec
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.trace_conn = None
        self.trace_curs = None
        self._file_list = file_list
        self.ts_timeline_update_data = []

    @staticmethod
    def _check_runtime_db(runtime_conn: any, runtime_curs: any) -> bool:
        if not (runtime_conn and runtime_curs) or not DBManager.judge_table_exist(
            runtime_curs, DBNameConstant.TABLE_RUNTIME_TIMELINE
        ):
            return False
        return True

    @staticmethod
    def _get_ts_timeline_update_data(db_path: str, time_ranges: list, runtime_curs: any) -> list:
        DBManager.add_new_column(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA, "ai_core_num", "INT", '0')
        sql = "select count(*) from {0} where tasktype=0 and taskState=3 and timestamp>? and timestamp<?".format(
            DBNameConstant.TABLE_RUNTIME_TIMELINE
        )
        step_ranges = Utils.generator_to_list(time_range[2:] for time_range in time_ranges)
        ai_core_nums = []
        for step_range in step_ranges:
            ai_core_num = runtime_curs.execute(sql, step_range).fetchone()
            if ai_core_num:
                ai_core_nums.append(ai_core_num)

        update_data = Utils.generator_to_list(
            (iter_data[1][0], iter_data[0][0], iter_data[0][1]) for iter_data in zip(time_ranges, ai_core_nums)
        )
        return update_data

    def parse(self: any) -> None:
        """
        parse
        """
        db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_STEP_TRACE)
        self.trace_conn, self.trace_curs = DBManager.check_connect_db_path(db_path)
        if not self._check_step_trace_db():
            return
        sql = "select index_id, model_id, step_start, step_end from {0}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        time_ranges = DBManager.fetch_all_data(self.trace_curs, sql)

        runtime_conn, runtime_curs = DBManager.check_connect_db(self._project_path, DBNameConstant.DB_RUNTIME)
        if not TsTimelineRecParser._check_runtime_db(runtime_conn, runtime_curs):
            return
        try:
            self.ts_timeline_update_data = TsTimelineRecParser._get_ts_timeline_update_data(
                db_path, time_ranges, runtime_curs
            )
        except sqlite3.Error as ts_err:
            logging.error(ts_err, exc_info=Constant.TRACE_BACK_SWITCH)
            DBManager.destroy_db_connect(self.trace_conn, self.trace_curs)
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)
            return
        self.save()
        DBManager.destroy_db_connect(self.trace_conn, self.trace_curs)
        DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def save(self: any) -> None:
        """
        save data
        """
        if not self.ts_timeline_update_data:
            return
        sql = "update {0} set ai_core_num=? where iter_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        DBManager.executemany_sql(self.trace_conn, sql, self.ts_timeline_update_data)
        self.trace_conn.commit()

    def ms_run(self: any) -> None:
        """
        run function
        """
        self.parse()

    def _check_step_trace_db(self: any) -> bool:
        if not (self.trace_conn and self.trace_curs) or not DBManager.judge_table_exist(
            self.trace_curs, DBNameConstant.TABLE_STEP_TRACE_DATA
        ):
            return False
        return True
