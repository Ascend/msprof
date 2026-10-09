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
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils


def _get_llc_result_data(llc_data: list, sample_config: dict, llc_data_list: int) -> dict:
    result_data = OrderedDict()
    llc_profiling_mode = sample_config.get("llc_profiling")
    result_data['mode'] = llc_profiling_mode
    result_data['rate'] = "MB/s"
    result_data['table'] = []
    llc_data_one = Utils.generator_to_list(i[1] * 100 for i in llc_data)
    llc_data_two = Utils.generator_to_list(i[2] for i in llc_data)
    result_data['table'].append(
        OrderedDict(
            [
                ("Mode", result_data.get("mode")),
                ("Task", "Average"),
                ("Hit Rate(%)", round(sum(llc_data_one) / llc_data_list, NumberConstant.ROUND_THREE_DECIMAL)),
                ("Throughput(MB/s)", round(sum(llc_data_two) / llc_data_list, NumberConstant.ROUND_THREE_DECIMAL)),
            ]
        )
    )
    for llc_slice in llc_data:
        result_data['table'].append(
            OrderedDict(
                [
                    ("Mode", result_data.get("mode")),
                    ("Task", llc_slice[0]),
                    ("Hit Rate(%)", round(llc_slice[1] * 100, NumberConstant.ROUND_THREE_DECIMAL)),
                    ("Throughput(MB/s)", round(llc_slice[2], NumberConstant.ROUND_THREE_DECIMAL)),
                ]
            )
        )
    return result_data


def _check_llc_db(conn: any, curs: any) -> str:
    if not (conn and curs):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The db doesn't exist."})
    if not DBManager.judge_table_exist(curs, StrConstant.LLC_METRICS_TABLE):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The LLC Metric Data doesn't exist."})
    return ""


def _get_llc_sql() -> str:
    sql = (
        'SELECT l3tId, round(AVG(hitRate), {accuracy}), '
        'round(AVG(throughput), {accuracy}) FROM {0} '
        'WHERE device_id = ? GROUP BY l3tId'.format(
            StrConstant.LLC_METRICS_TABLE, accuracy=NumberConstant.DECIMAL_ACCURACY
        )
    )
    return sql


def get_llc_train_summary(result_dir: str, sample_config: dict, device_id: str) -> str:
    """
    report llc data summary
    """
    db_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_LLC)
    conn, curs = DBManager.check_connect_db_path(db_path)
    res = _check_llc_db(conn, curs)
    if res:
        if conn or curs:
            DBManager.destroy_db_connect(conn, curs)
        return res
    try:
        l3_list = curs.execute(
            "SELECT count(DISTINCT(l3tId)) FROM {0}".format(StrConstant.LLC_METRICS_TABLE)
        ).fetchone()[0]
    except sqlite3.Error:
        DBManager.destroy_db_connect(conn, curs)
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to get data of llc bandwidth.", "data": {}})

    if not l3_list:
        DBManager.destroy_db_connect(conn, curs)
        return json.dumps({'status': NumberConstant.ERROR, "info": "Cannot load LLC ID from LLC Metric Data."})
    llc_data = DBManager.fetch_all_data(curs, _get_llc_sql(), (device_id,))
    if not llc_data:
        DBManager.destroy_db_connect(conn, curs)
        return json.dumps({"status": NumberConstant.ERROR, "info": "The LLC Data doesn't exist."})

    try:
        result_data = _get_llc_result_data(llc_data, sample_config, l3_list)
        return json.dumps({"status": NumberConstant.SUCCESS, "info": "", "data": result_data})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to get data of llc bandwidth.", "data": {}})
    finally:
        DBManager.destroy_db_connect(conn, curs)
