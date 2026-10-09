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
import os
import sqlite3

from common_func.common import CommonConstant
from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import path_check


def _pre_check_pmu_events(project_path: str) -> tuple:
    sample_path = path_check(os.path.join(project_path, CommonConstant.SAMPLE_JSON))
    if not sample_path:
        return NumberConstant.ERROR, "Configuration file doesn't exist.", {}
    sample_config = ConfigMgr.read_sample_config(project_path)
    if not sample_config:
        return NumberConstant.ERROR, "Failed to generate sample configuration table.", {}
    if sample_config.get('ai_core_profiling_mode', '') not in [
        StrConstant.AIC_TASK_BASED_MODE,
        StrConstant.AIC_SAMPLE_BASED_MODE,
        '',
    ]:
        return NumberConstant.ERROR, "Failed to verify configuration file parameters.", {}

    return NumberConstant.SUCCESS, 'success', sample_config


def pre_check_pmu_events_interface(project_path: str) -> tuple:
    """
    pre check pmu events interface
    """
    try:
        ret_code, ret_msg, sample_config = _pre_check_pmu_events(project_path)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return NumberConstant.ERROR, 'Failed to check pmu events.', {}
    if ret_code == NumberConstant.ERROR:
        return ret_code, ret_msg, sample_config
    return (
        NumberConstant.SUCCESS,
        '',
        {'sample_config': sample_config, 'type_db_match': {"ai_core_profiling": DBNameConstant.DB_NAME_AICORE}},
    )


def calculate_utilization(tmp_data: list, result_data: dict) -> None:
    """
    calculate utilization block data
    """
    time_list = [1]
    for i in range(len(tmp_data) - 1):
        time_list.append(tmp_data[i + 1][1] - tmp_data[i][1])
    freq = InfoConfReader().get_freq(StrConstant.AIC)
    for tmp in tmp_data:  # [timestamp, utilization, coreid]
        interval = freq * float(time_list[tmp_data.index(tmp)])
        if not NumberConstant.is_zero(interval) and len(tmp) >= 3:  # length tmp should be longer than 3
            result_data['usage'].setdefault(str(tmp[2]), []).append(
                [
                    StrConstant.ACCURACY % float(tmp[1]),
                    StrConstant.ACCURACY % float(float(tmp[0]) * NumberConstant.PERCENTAGE / interval),
                ]
            )


def get_utilization_data(*param: list) -> dict:
    """
    get utilization data
    """
    curs, result_data, min_time, _, core, start, end = param
    sql = 'select task_cyc, timestamp/{} - ?, coreid from AICoreOriginalData where coreid=? order by timestamp;'.format(
        NumberConstant.NS_TIME_RATE
    )
    param = (min_time, core)
    if (start and end) is not None:
        sql = (
            'select task_cyc, timestamp/{NS_TIME_RATE} - ?, coreid from AICoreOriginalData '
            'where coreid=? and timestamp/{NS_TIME_RATE} - ? >= ? '
            'and timestamp/{NS_TIME_RATE} - ? <= ? order by timestamp'.format(NS_TIME_RATE=NumberConstant.NS_TIME_RATE)
        )
        param = (min_time, core, min_time, start, min_time, end)
    tmp_data = curs.execute(sql, param).fetchall()
    calculate_utilization(tmp_data, result_data)
    return result_data


def cal_ave(result_data: dict, pos_cores: dict) -> dict:
    """
    calculate average of ai core utilization data
    """
    if not result_data.get("usage"):
        return result_data
    data_keys = list(result_data['usage'].keys())
    if not data_keys:
        return result_data
    key_len = len(data_keys)
    item = data_keys[0]
    # data length alignment
    pos_max = pos_cores[item][0]
    pos_min = pos_cores[item][1]
    # find first max position
    for key in data_keys:
        pos_max = max(pos_cores[key][0], pos_max)
    for key in data_keys:
        length = pos_max - pos_cores[key][0]
        result_data['usage'][key] = result_data['usage'][key][length:]
    # find last min position
    for key in data_keys:
        pos_min = min(pos_cores[key][1], pos_min)
    for key in data_keys:
        length = pos_min - pos_cores[key][1]
        if length < 0:
            result_data['usage'][key] = result_data['usage'][key][:length]
    data_len = len(result_data['usage'][item])

    result_data['usage']['average'] = []
    for i in range(data_len):
        sum_count = 0
        for _, item in enumerate(data_keys):
            if len(result_data['usage'][item]) > i and len(result_data['usage'][item][i]) == 2:
                sum_count += float(result_data['usage'][item][i][1])
        average_value = sum_count / key_len if not NumberConstant.is_zero(key_len) else 0
        result_data['usage']['average'].append([result_data['usage'][item][i][0], StrConstant.ACCURACY % average_value])
    return result_data


def _get_aicore_util(curs: any, number: float, start: float, end: float) -> str:
    """
    branch to collect aicore util data
    """
    result_data = {'maxTime': 0, 'minTime': 0, 'usage': {}}
    max_time = curs.execute(
        'select max(timestamp)/{} from AICoreOriginalData where replayid=0'.format(NumberConstant.NS_TIME_RATE)
    ).fetchone()[0]
    min_time = curs.execute(
        'select min(timestamp)/{} from AICoreOriginalData where replayid=0'.format(NumberConstant.NS_TIME_RATE)
    ).fetchone()[0]
    if max_time is None or min_time is None:
        return json.dumps({'status': NumberConstant.ERROR, 'data': "Unable to get aicore utilization."})
    result_data['maxTime'], result_data['minTime'] = '%.2f' % float(max_time - min_time), 0
    cores = curs.execute('select distinct(coreid) from AICoreOriginalData;').fetchall()
    pos_cores = {}
    for core in cores:
        pos_cores = get_aicore_position(curs, pos_cores, core[0], 0, start, end)
        result_data = get_utilization_data(curs, result_data, 0, number, core[0], start, end)
    result_data = cal_ave(result_data, pos_cores)
    return json.dumps({'status': NumberConstant.SUCCESS, 'data': result_data})


def get_aicore_utilization(project_path: str, number: float, start: float, end: float) -> str:
    """
    get ai core utilization data
    """
    result = {StrConstant.STATUS: 0, StrConstant.MSG: ''}
    result[StrConstant.STATUS], result[StrConstant.MSG], func_map = pre_check_pmu_events_interface(project_path)
    if result.get(StrConstant.STATUS) == NumberConstant.ERROR:
        return json.dumps({StrConstant.STATUS: NumberConstant.ERROR, StrConstant.INFO: result.get(StrConstant.MSG)})
    conn, curs = DBManager.check_connect_db(project_path, func_map.get('type_db_match', {}).get('ai_core_profiling'))
    if not (conn and curs):
        return json.dumps({StrConstant.STATUS: NumberConstant.ERROR, StrConstant.INFO: "The db doesn't exist."})
    try:
        if func_map.get('sample_config', {}).get("ai_core_profiling_mode") == StrConstant.AIC_SAMPLE_BASED_MODE:
            return _get_aicore_util(curs, number, start, end)
        return json.dumps(
            {StrConstant.STATUS: NumberConstant.ERROR, StrConstant.DATA: "Unable to get aicore utilization."}
        )
    except sqlite3.Error:
        return json.dumps(
            {StrConstant.STATUS: NumberConstant.ERROR, StrConstant.INFO: 'Can not get aicore utilization'}
        )
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_aicore_position(*param: list) -> dict:
    """
    get aicore data position
    """
    curs, pos_cores, core, min_time, start, end = param
    start_sql = 'select count(*) from AICoreOriginalData where coreid=? and timestamp/{NS_TIME_RATE} - ? < ?'.format(
        NS_TIME_RATE=NumberConstant.NS_TIME_RATE
    )
    start_param = (core, min_time, start)
    end_sql = 'select count(*) from AICoreOriginalData where coreid=? and timestamp/{NS_TIME_RATE} - ? <= ?'.format(
        NS_TIME_RATE=NumberConstant.NS_TIME_RATE
    )
    end_param = (core, min_time, end)
    pos_cores[str(core)] = [
        curs.execute(start_sql, start_param).fetchone()[0],
        curs.execute(end_sql, end_param).fetchone()[0],
    ]
    return pos_cores
