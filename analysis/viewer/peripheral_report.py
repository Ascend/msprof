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
from common_func.get_table_data import GetTableData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.msvp_constant import MsvpConstant
from common_func.info_conf_reader import InfoConfReader


def get_peripheral_dvpp_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get peripheral dvpp data
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs and DBManager.judge_table_exist(curs, table_name)):
        return MsvpConstant.MSVP_EMPTY_DATA
    data = get_dvpp_data(curs, device_id)
    try:
        count = GetTableData.get_data_count_for_device(curs, table_name, device_id)
        return configs.get(StrConstant.CONFIG_HEADERS), data, count
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_dvpp_data(curs: any, devid: str) -> list:
    """
    get dvpp data
    """
    report_data = DBManager.fetch_all_data(curs, "select * from DvppReportData where device_id = ?;", (devid,))
    new_report_data = []
    for rd in report_data:
        item = [rd[0], StrConstant.DVPP_ENGINE_TYPE.get(rd[2])]
        item.extend(rd[3:])
        new_report_data.append(item)
    return new_report_data


def get_peripheral_nic_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get peripheral nic data
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs and DBManager.judge_table_exist(curs, table_name)):
        return MsvpConstant.MSVP_EMPTY_DATA
    data = GetTableData.get_table_data_for_device(curs, table_name, device_id)
    try:
        count = GetTableData.get_data_count_for_device(curs, table_name, device_id)
        data = format_nic_summary(data, configs.get(StrConstant.CONFIG_HEADERS))
        return configs.get(StrConstant.CONFIG_HEADERS), data, count
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)


def format_nic_summary(data: list, headers: list) -> list:
    round_index_start = headers.index('Bandwidth(MB/s)')
    round_index_end = headers.index('txDropped rate(%)')
    timestamp_index = headers.index('Timestamp(us)')
    format_data = [[] for _ in range(len(data))]
    for index, line in enumerate(data):
        line = list(line)
        line[timestamp_index] = format_high_precision_for_csv(InfoConfReader().trans_into_local_time(line[0]))
        for i in range(round_index_start, round_index_end + 1):
            line[i] = round(float(line[i]), NumberConstant.ROUND_THREE_DECIMAL)
        format_data[index] = line
    return format_data
