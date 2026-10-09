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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import read_cpu_cfg
from common_func.msvp_constant import MsvpConstant


def _reformat_aictrl_pmu_data(res: list, headers: list) -> tuple:
    aicpu_events_map = read_cpu_cfg("ai_cpu", "events")
    if aicpu_events_map is None:
        return MsvpConstant.MSVP_EMPTY_DATA
    data = []
    for pmu_data in res:
        new_pmu = pmu_data[0].replace("\x00", '').replace("r", "0x")
        new_pmu1 = pmu_data[0].replace("\x00", '').replace("r", "")
        item = [new_pmu, aicpu_events_map[int(new_pmu1, 16)].capitalize(), pmu_data[1]]
        data.append(item)
    return headers, data, len(data)


def get_aictrl_pmu_events(project_path: str, db_name: str, table_name: str, headers: list) -> tuple:
    """
    get msvp aicpu or ctrlcpu pmu event
    """
    conn, curs = DBManager.check_connect_db(project_path, db_name)
    try:
        if not (conn and curs) or not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        res = DBManager.fetch_all_data(
            curs,
            "SELECT pmuevent AS event, SUM(pmucount) AS count FROM {} GROUP BY pmuevent ORDER BY count DESC;".format(
                table_name
            ),
        )
        return _reformat_aictrl_pmu_data(res, headers)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _get_ts_event_count(sample_config: dict, curs: any, table_name: str) -> dict:
    event_count = {}
    ts_cpu_events = sample_config.get("ts_cpu_profiling_events", "").split(",")
    ts_cpu_events = (hex(int(i, NumberConstant.HEX_NUMBER)) for i in ts_cpu_events)
    event_sum_sql = 'select sum(count) from {} where event=?;'.format(table_name)
    for event_id in ts_cpu_events:
        count = DBManager.fetch_all_data(curs, event_sum_sql, (event_id,))
        event_count[event_id] = count[0][0]
    return event_count


def _get_ts_result_data(event_count: dict) -> list:
    total_data = []
    ts_cpu_events_map = read_cpu_cfg("ts_cpu", "events")
    for key, value in list(event_count.items()):
        if value is not None:
            if ts_cpu_events_map:
                total_data.append(
                    (str(key), ts_cpu_events_map.get(int(key, NumberConstant.HEX_NUMBER)).capitalize(), str(value))
                )
            else:
                total_data.append((str(key), " ", str(value)))
    return total_data


def get_ts_pmu_events(project_path: str, db_name: str, table_name: str, headers: list) -> tuple:
    """
    get msvp ts pmu event
    """
    conn, curs = DBManager.check_connect_db(project_path, db_name)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    sample_config = ConfigMgr.pre_check_sample(project_path, 'ts_cpu_profiling_events')
    try:
        if not sample_config or not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        event_count = _get_ts_event_count(sample_config, curs, table_name)
        total_data = _get_ts_result_data(event_count)
        res = sorted(total_data, key=lambda x: x[2], reverse=True)
        return headers, res, len(res)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_cpu_hot_function(project_path: str, db_name: str, table_name: str, headers: list) -> tuple:
    """
    get ai/ctrl cpu hot function data
    """
    conn, curs = DBManager.check_connect_db(project_path, db_name)
    try:
        if not (conn and curs) or not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
        total_cycles = curs.execute("SELECT SUM(r11) FROM {};".format(table_name)).fetchone()[0]
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        if not total_cycles:
            return MsvpConstant.MSVP_EMPTY_DATA
        cpu_hot_func_sql = (
            "SELECT func,module,SUM(r11) AS cycles,CAST(1.0*SUM(r11)*100/? AS decimal(8,{})) "
            "FROM {} where r11 != 0 GROUP BY func,module "
            "ORDER BY cycles DESC;".format(NumberConstant.DECIMAL_ACCURACY, table_name)
        )
        cpu_data = DBManager.fetch_all_data(curs, cpu_hot_func_sql, (total_cycles,))
        data = [rec[:3] + (str(round(rec[3], NumberConstant.ROUND_THREE_DECIMAL)),) for rec in cpu_data]
        return headers, data, len(data)
    finally:
        DBManager.destroy_db_connect(conn, curs)
