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

import sqlite3

from common_func.db_manager import DBManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant


def _get_ddr_data_from_db(curs: any, device_id: str) -> list:
    sql = 'select flux_read, flux_write from DDRMetricData where device_id = ?;'
    ddr_data = curs.execute(sql, (device_id,)).fetchone()
    if ddr_data is None:
        return MsvpConstant.MSVP_EMPTY_DATA
    ddr_data = curs.execute(sql, (device_id,)).fetchall()
    read_sum, write_sum = 0, 0
    for ddr in ddr_data:
        read_sum += ddr[0]
        write_sum += ddr[1]
    read_avg = 0.0
    write_avg = 0.0
    if not NumberConstant.is_zero(len(ddr_data)):
        read_avg = read_sum * 1.0 / len(ddr_data)
        write_avg = write_sum * 1.0 / len(ddr_data)
    data = [
        [
            'Average',
            round(read_avg, NumberConstant.ROUND_THREE_DECIMAL),
            round(write_avg, NumberConstant.ROUND_THREE_DECIMAL),
        ]
    ]
    return data


def get_ddr_data(db_path: str, device_id: str, configs: dict) -> tuple:
    """
    get ddr data from database
    :param db_path: database path
    :param device_id: device id for search
    :param configs: configs for search
    :return data headers, data body, data count
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not conn or not curs:
        return MsvpConstant.MSVP_EMPTY_DATA
    if not DBManager.judge_table_exist(curs, "DDRMetricData"):
        return MsvpConstant.MSVP_EMPTY_DATA

    try:
        data = _get_ddr_data_from_db(curs, device_id)
        return configs.get(StrConstant.CONFIG_HEADERS), data, 1  # ddr data only contains one result
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)
