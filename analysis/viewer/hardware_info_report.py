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

from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from msmodel.hardware.mini_llc_model import cal_core2cpu


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


def cal_llc_band_res(llc_data: list, max_time: float) -> tuple:
    """
    calculate llc bandwidth result
    :param llc_data: llc original data
    :param max_time: time range
    :return: ['Metric', 'l3c_rd', 'l3c_wr'], result_data, 3# 3 is the count of summary items
    """
    read_hit = 0.0
    if not NumberConstant.is_zero(llc_data[0] + llc_data[1]):
        read_hit = round(llc_data[2] / (llc_data[0] + llc_data[1]), NumberConstant.DECIMAL_ACCURACY)

    write_hit = 0.0
    if not NumberConstant.is_zero(llc_data[3] + llc_data[4]):
        write_hit = round(llc_data[5] / (llc_data[3] + llc_data[4]), NumberConstant.DECIMAL_ACCURACY)

    hit_rate = [
        [
            "Hit_Rate(%)",
            round(read_hit * NumberConstant.PERCENTAGE, NumberConstant.ROUND_THREE_DECIMAL),
            round(write_hit * NumberConstant.PERCENTAGE, NumberConstant.ROUND_THREE_DECIMAL),
        ]
    ]
    if NumberConstant.is_zero(max_time):
        bandwidth = [["BandWidth(MB/s)", 0.0, 0.0]]
        hit_bandwidth = [["Hit_BandWidth(MB/s)", 0.0, 0.0]]
    else:
        bandwidth = [
            [
                "BandWidth(MB/s)",
                round(
                    (llc_data[0] + llc_data[1])
                    * NumberConstant.LLC_CAPACITY
                    / max_time
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE),
                    NumberConstant.ROUND_THREE_DECIMAL,
                ),
                round(
                    (llc_data[3] + llc_data[4])
                    * NumberConstant.LLC_CAPACITY
                    / max_time
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE),
                    NumberConstant.ROUND_THREE_DECIMAL,
                ),
            ]
        ]
        hit_bandwidth = [
            [
                "Hit_BandWidth(MB/s)",
                round(
                    llc_data[2]
                    * NumberConstant.LLC_CAPACITY
                    / max_time
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE),
                    NumberConstant.ROUND_THREE_DECIMAL,
                ),
                round(
                    llc_data[5]
                    * NumberConstant.LLC_CAPACITY
                    / max_time
                    / (NumberConstant.KILOBYTE * NumberConstant.KILOBYTE),
                    NumberConstant.ROUND_THREE_DECIMAL,
                ),
            ]
        ]
    result_data = bandwidth + hit_rate + hit_bandwidth
    headers = ['Metric', 'l3c_rd', 'l3c_wr']
    return headers, result_data, len(headers)


def _get_bandwidth_res(curs: any, device_id: str) -> tuple:
    try:
        max_time = curs.execute(
            "select max(timestamp) - min(timestamp) from LLCMetricData where device_id = ?;", (device_id,)
        ).fetchone()[0]
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    if max_time is None:
        return MsvpConstant.MSVP_EMPTY_DATA
    sql = (
        'select sum(read_allocate),sum(read_noallocate),sum(read_hit),'
        'sum(write_allocate),sum(write_noallocate),sum(write_hit)'
        ' from LLCMetricData where device_id = ?;'
    )
    try:
        llc_data = curs.execute(sql, (device_id,)).fetchone()
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    if llc_data:
        return cal_llc_band_res(llc_data, max_time)
    return MsvpConstant.MSVP_EMPTY_DATA


def get_llc_bandwidth(project_path: str, device_id: str) -> tuple:
    """
    get llc bandwidth original data
    """
    conn, curs = DBManager.check_connect_db(project_path, DBNameConstant.DB_LLC)
    sample_config = ConfigMgr.read_sample_config(project_path)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        if (
            not DBManager.judge_table_exist(curs, "LLCMetricData")
            or sample_config.get(StrConstant.LLC_PROF, "") != StrConstant.LLC_BAND_ITEM
        ):
            return MsvpConstant.MSVP_EMPTY_DATA
        return _get_bandwidth_res(curs, device_id)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _get_llc_capacity_data(curs: any, project_path: str, device_id: str, types: str) -> tuple:
    core2cpu = cal_core2cpu(project_path, device_id)
    dsid_name = core2cpu[types]
    dsid_name = Utils.generator_to_list(
        f"sum({i})*{NumberConstant.LLC_CAPACITY}/({NumberConstant.BYTES_TO_KB})" for i in dsid_name
    )
    sql = "select {column} from LLCDsidData where device_id = ?".format(column=",".join(dsid_name))
    dsid_data = DBManager.fetch_all_data(curs, sql, (device_id,))
    dsid_data = Utils.generator_to_list(list(i) for i in dsid_data)
    for index, value in enumerate(dsid_data):
        dsid_data[index] = Utils.generator_to_list(round(i, NumberConstant.ROUND_THREE_DECIMAL) for i in value)
    dsid_data = Utils.generator_to_list(
        ['Used Capacity of LLC'] + i + [round(sum(i), NumberConstant.ROUND_THREE_DECIMAL)] for i in dsid_data
    )
    cpu_list = Utils.generator_to_list('CPU{}(KB)'.format(value) for value in range(len(core2cpu[types])))
    headers = ['Metric'] + cpu_list + ['Total(KB)']
    return headers, dsid_data, 1  # 1 refers to the count of data of llc capacity


def llc_capacity_data(project_path: str, device_id: str, types: str) -> tuple:
    """
    get llc capacity data
    """
    if types not in {'ctrlcpu', 'aicpu'}:
        return MsvpConstant.MSVP_EMPTY_DATA
    sample_config = ConfigMgr.read_sample_config(project_path)
    conn, curs = DBManager.check_connect_db(project_path, 'llc.db')
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    counter_exist = DBManager.judge_table_exist(curs, "LLCOriginalData")
    if not counter_exist or sample_config.get(StrConstant.LLC_PROF, "") != StrConstant.LLC_CAPACITY_ITEM:
        DBManager.destroy_db_connect(conn, curs)
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        return _get_llc_capacity_data(curs, project_path, device_id, types)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)
