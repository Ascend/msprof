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

import json
import sqlite3
from collections import OrderedDict

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import float_calculate
from common_func.utils import Utils


def _insert_hbm_data(data: list, hbm_id_count: int) -> None:
    hbm_lst1 = Utils.generator_to_list(x[1] for x in data)
    calculate_val1 = float_calculate([float_calculate(hbm_lst1), hbm_id_count], '/')
    hbm_lst2 = Utils.generator_to_list(x[2] for x in data)
    calculate_val2 = float_calculate([float_calculate(hbm_lst2), hbm_id_count], '/')
    data.insert(0, ("Average", _reformat_hbm_data(calculate_val1), _reformat_hbm_data(calculate_val2)))


def _get_hbm_result_ata(curs: any, device_id: str, hbm_id_count: int) -> list:
    sql = (
        'select hbmId,'
        '(select AVG(bandwidth) from HBMbwData where event_type="read"),'
        '(select AVG(bandwidth) from HBMbwData where event_type="write") from HBMbwData '
        'where device_id=? group by hbmId'
    )
    data = DBManager.fetch_all_data(curs, sql, (device_id,))
    _insert_hbm_data(data, hbm_id_count)
    result_data = {'rate': 'MB/s', 'table': []}
    for tmp in data:
        result_data.get('table', []).append(
            OrderedDict([('Task', tmp[0]), ('Read(MB/s)', tmp[1]), ('Write(MB/s)', tmp[2])])
        )
    return result_data


def _check_hbm_db(conn: any, curs: any) -> str:
    if not (conn and curs):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The db doesn't exist."})
    if not DBManager.judge_table_exist(curs, "HBMOriginalData"):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The HBM Original Data doesn't exist."})
    return ""


def get_hbm_summary(project_path: str, device_id: str) -> str:
    """
    get HBM data summary
    """
    conn, curs = DBManager.check_connect_db(project_path, DBNameConstant.DB_HBM)
    res = _check_hbm_db(conn, curs)
    if res:
        return res
    try:
        hbm_id_count = curs.execute(
            'select count(distinct(hbmId)) from HBMbwData where device_id=?', (device_id,)
        ).fetchone()[0]
    except sqlite3.Error:
        return json.dumps({'status': NumberConstant.ERROR, 'info': 'Failed to get HBM data. '})
    else:
        if not hbm_id_count:
            return json.dumps({'status': NumberConstant.ERROR, "info": "Failed to get hbm data."})
        result_data = _get_hbm_result_ata(curs, device_id, hbm_id_count)
        return json.dumps({"status": NumberConstant.SUCCESS, "info": "", 'data': result_data})
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_hbm_summary_data(project_path: str, device_id: str) -> any:
    """
    get HBM data summary
    """
    conn, curs = DBManager.check_connect_db(project_path, DBNameConstant.DB_HBM)
    res = _check_hbm_db(conn, curs)
    if res:
        return res
    try:
        hbm_id_count = curs.execute(
            'select count(distinct(hbmId)) from HBMbwData where device_id=?', (device_id,)
        ).fetchone()[0]
    except sqlite3.Error:
        return []
    else:
        if not hbm_id_count:
            return json.dumps({'status': NumberConstant.ERROR, "info": "Failed to get hbm data."})
        sql = (
            'select hbmId,'
            'sum(case when event_type="read" then bandwidth else 0 end) / '
            'sum(case when event_type="read" then 1 else 0 end) as read,'
            'sum(case when event_type="write" then bandwidth else 0 end) / '
            'sum(case when event_type="write" then 1 else 0 end) as write'
            ' FROM "HBMbwData"  WHERE device_id = ? GROUP BY hbmid'
        )
        data = DBManager.fetch_all_data(curs, sql, (device_id,))
        _insert_hbm_data(data, hbm_id_count)
        data = _format_hbm_data(data)
        return data
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _reformat_hbm_data(calculate_value: any) -> any:
    """
    format the calculated number.
    """
    format_result = calculate_value
    if format_result is not None:
        # replace E with e in Scientific counting.
        format_result = format_result.replace("E", "e")
    return format_result


def _format_hbm_data(data: list) -> list:
    hbm_data = [[] for _ in range(len(data))]
    for index, item in enumerate(data):
        if not index:
            hbm_data[index] = list(item)
            hbm_data[index][1] = item[1]
            hbm_data[index][2] = item[2]
        else:
            hbm_data[index] = list(item)
            hbm_data[index][1] = round(item[1], NumberConstant.ROUND_THREE_DECIMAL) if item[1] else item[2]
            hbm_data[index][2] = round(item[2], NumberConstant.ROUND_THREE_DECIMAL) if item[2] else item[2]
    return hbm_data
