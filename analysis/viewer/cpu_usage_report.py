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

# pylint: disable=duplicate-code

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant


def get_sys_cpu_usage_data(db_path: str, table_name: str, configs: dict) -> tuple:
    """
    get system cpu usage
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        if not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        data = _sys_usage_data(curs, table_name)
        return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _do_divide(val: int, total_size) -> float:
    """
    do divide
    """
    ratio = 0.0
    try:
        ratio = round(val / total_size * NumberConstant.PERCENTAGE, NumberConstant.ROUND_THREE_DECIMAL)
    except ZeroDivisionError as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
    return ratio


def _sys_usage_data(curs: any, table_name: str) -> list:
    """
    get sys cpu data
    """
    sql = (
        "select cputype, sum(user), sum(sys), sum(iowait), sum(irq), sum(soft), sum(idle), "
        "sum(user+nice+sys+idle+iowait+irq+soft+steal+guest+gnice) "
        "from {} where cputype != '' group by cputype".format(table_name)
    )
    result_data = DBManager.fetch_all_data(curs, sql)
    data_list = []
    for data in result_data:
        total_size = round(data[7], NumberConstant.ROUND_THREE_DECIMAL)
        cpu_type = data[0]
        user_ratio = _do_divide(data[1], total_size)
        sys_ratio = _do_divide(data[2], total_size)
        io_wait_ratio = _do_divide(data[3], total_size)
        irq_ratio = _do_divide(data[4], total_size)
        soft_ratio = _do_divide(data[5], total_size)
        idle_ratio = _do_divide(data[6], total_size)
        data_list.append([cpu_type, user_ratio, sys_ratio, io_wait_ratio, irq_ratio, soft_ratio, idle_ratio])
    return data_list


def get_process_cpu_usage(db_path: str, table_name: str, configs: dict) -> tuple:
    """
    get process cpu usage
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        if not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        data = _proc_usage_data(curs, table_name)
        return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _proc_usage_data(curs: any, table_name: str) -> list:
    """
    get process cpu usage data
    """
    proc_sql = (
        "select pid, process_name, 100*sum((utime+stime+cutime+cstime)/sys_usage)/count(*) "
        "from {} group by pid order by 100*sum((utime+stime+cutime+cstime)/sys_usage)/count(*)"
        " desc".format(table_name)
    )
    proc_data = DBManager.fetch_all_data(curs, proc_sql)
    return _format_cpu_usage_data(proc_data)


def _format_cpu_usage_data(result_data: list) -> list:
    """
    format for 6 decimal places.
    :param result_data: result_data
    :return: tuple for cpu usage data.
    """
    for i, data in enumerate(result_data):
        result_list_data = list(data)
        if is_number(str(result_list_data[-1])):
            result_list_data[-1] = round(result_list_data[-1], NumberConstant.ROUND_THREE_DECIMAL)
        result_data[i] = tuple(result_list_data)
    return result_data
