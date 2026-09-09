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

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager


def insert_ccu_data_to_db(conn: any, table_name: str, data_list: list) -> bool:
    if not conn:
        logging.warning("Database connection is unavailable when inserting CCU table %s.", table_name)
        return False
    try:
        if conn.execute("SELECT COUNT(*) FROM {}".format(table_name)).fetchone()[0] != 0:
            logging.error("CCU table %s is not empty; clear parsed output and reimport.", table_name)
            return False
    except sqlite3.Error:
        logging.exception("Cannot check existing CCU table %s.", table_name)
        return False
    if not data_list:
        return True
    placeholders = "?," * (len(data_list[0]) - 1) + "?"
    sql = "insert into {} values ({})".format(table_name, placeholders)
    if DBManager.executemany_sql(conn, sql, data_list):
        return True
    logging.warning("Insert data into CCU table %s failed.", table_name, exc_info=Constant.TRACE_BACK_SWITCH)
    return False
