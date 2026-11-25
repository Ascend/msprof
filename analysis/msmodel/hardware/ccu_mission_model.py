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
from msmodel.ccu_model import insert_ccu_data_to_db
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginMissionDto
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginSetCKBV62Dto
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginLoopGroupV62Dto


class CCUMissionModel(ParserModel):
    """
    ccu mission model class
    """

    def flush(self: any, data_list: list, table_name: str) -> bool:
        """
        flush ccu mission data to db
        :param data_list: ccu mission origin data list
        :return: whether the insert transaction was committed
        """
        return insert_ccu_data_to_db(self.conn, table_name, data_list)

    def flush_v62(self: any, setckb_rows: list, loopgroup_rows: list, source_rows: list) -> bool:
        if not self.conn:
            return False
        try:
            with self.conn:
                self.conn.execute("BEGIN IMMEDIATE")
                for table, rows, columns in (
                    (DBNameConstant.TABLE_CCU_SETCKB_V62, setckb_rows, 11),
                    (DBNameConstant.TABLE_CCU_LOOPGROUP_V62, loopgroup_rows, 5),
                    (DBNameConstant.TABLE_CCU_MISSION_SOURCE_V62, source_rows, 3),
                ):
                    if self.conn.execute("SELECT 1 FROM {} LIMIT 1".format(table)).fetchone():
                        raise ValueError("CCU table {} already contains data".format(table))
                    placeholders = ", ".join(["?"] * columns)
                    self.conn.executemany("INSERT INTO {} VALUES ({})".format(table, placeholders), rows)
            return True
        except (sqlite3.Error, ValueError, OverflowError) as error:
            logging.error("Failed to persist CCU v6.2 mission data: %s", error, exc_info=Constant.TRACE_BACK_SWITCH)
            return False


class CCUViewerMissionModel(ViewModel):
    """
    ccu viewer mission model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_MISSION])

    def get_timeline_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = (
            "SELECT stream_id, task_id, lp_instr_id, setckebit_instr_id, rel_id, "
            "CASE WHEN setckebit_start_time <> 0 THEN setckebit_start_time ELSE lp_start_time END AS start_time, "
            "CASE WHEN setckebit_start_time <> 0 THEN rel_end_time ELSE lp_end_time END AS end_time, "
            "CASE WHEN setckebit_start_time <> 0 THEN 'Wait' ELSE 'LoopGroup' END AS time_type FROM {} "
            "WHERE setckebit_start_time <> 0 OR lp_start_time <> 0;".format(DBNameConstant.TABLE_CCU_MISSION)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginMissionDto)

    def get_summary_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = (
            "select stream_id, task_id, lp_instr_id, lp_start_time, lp_end_time, "
            "setckebit_instr_id, setckebit_start_time, rel_id, rel_end_time from {};".format(
                DBNameConstant.TABLE_CCU_MISSION
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginMissionDto)


class CCUViewerMissionV62Model(ViewModel):
    """
    ccu viewer mission model class for chip v6_2
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(
            result_dir,
            DBNameConstant.DB_CCU,
            [DBNameConstant.TABLE_CCU_SETCKB_V62, DBNameConstant.TABLE_CCU_LOOPGROUP_V62],
        )

    def get_setckb_summary_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_CCU_SETCKB_V62, dto_class=OriginSetCKBV62Dto)

    def get_loopgroup_summary_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_CCU_LOOPGROUP_V62, dto_class=OriginLoopGroupV62Dto)
