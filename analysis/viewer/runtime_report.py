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
import os
import sqlite3

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import add_aicore_units
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


def get_task_scheduler_data(db_path: str, table_name: str, configs: dict, params: dict) -> tuple:
    """
    get task scheduler data
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs and DBManager.judge_table_exist(curs, table_name)):
        report_task_data = []
    else:
        res = get_output_tasktype(curs, params)
        report_task_data = add_ts_opname(res, params.get(StrConstant.PARAM_RESULT_DIR))

    report_task_data = add_memcpy_data(params.get('project'), report_task_data)
    DBManager.destroy_db_connect(conn, curs)
    return configs.get(StrConstant.CONFIG_HEADERS), report_task_data, len(report_task_data)


def add_memcpy_data(result_dir: str, data: list) -> list:
    """
    add ts memcpy data to task scheduler
    :param result_dir: result dir
    :param data: task scheduler data
    :return: data
    """
    memcpy_viewer = MemoryCopyViewer(result_dir)
    memcpy_data = memcpy_viewer.get_memory_copy_chip0_summary()

    if memcpy_data:
        data.extend(memcpy_data)
        data = _update_time_ratio(data)

    return data


def _update_time_ratio(data: list) -> list:
    """
    datum index: 0 total time ratio; 1 total time
    """
    update_data = []
    sum_time = sum(datum[1] for datum in data)
    for datum in data:
        avg = 0
        if not NumberConstant.is_zero(sum_time):
            avg = datum[1] / sum_time * 100
        update_datum = [avg]
        update_datum.extend(datum[1:])
        update_data.append(tuple(update_datum))

    return update_data


def _get_new_task(report_task_data: list, result_dir: str, curs_ge: any) -> list:
    res = []
    for task in report_task_data:
        if len(task) > 13:  # search data length should be longer than 13
            # task[11]:task id, task[12]:stream id, task[13]: batch id occupy position
            op_name = get_opname(task, result_dir, curs_ge)
            op_name = op_name if op_name else Constant.NA
            task = list(task)
            task.insert(12, op_name)
            # Len of task is original 14, but 15 after insert opname. Index 14 is batch id.
            task.pop(14)
            res.append(tuple(task))
    return res


def add_ts_opname(report_task_data: list, result_dir: str) -> list:
    """
    add op name for task scheduler
    """
    conn_ge, curs_ge = DBManager.check_connect_db(result_dir, DBNameConstant.DB_GE_INFO)
    if not conn_ge or not curs_ge or not DBManager.judge_table_exist(curs_ge, DBNameConstant.TABLE_GE_TASK):
        DBManager.destroy_db_connect(conn_ge, curs_ge)
        conn_ge, curs_ge = DBManager.check_connect_db(result_dir, DBNameConstant.DB_RTS_TRACK)
    if not conn_ge or not curs_ge:
        logging.warning('Can not get op_name, maybe framework data or task_track data not collected.')
    new_task_list = _get_new_task(report_task_data, result_dir, curs_ge)
    DBManager.destroy_db_connect(conn_ge, curs_ge)
    return new_task_list


def _get_task_based_core_data(params: dict, curs: any, result_dir: str) -> tuple:
    data = []
    table_name = ""
    if params.get(StrConstant.DATA_TYPE) == StrConstant.AI_CORE_PMU_EVENTS:
        table_name = DBNameConstant.TABLE_METRIC_SUMMARY
        data = _get_output_event_counter(curs, result_dir, DBNameConstant.TABLE_METRIC_SUMMARY)
    elif params.get(StrConstant.DATA_TYPE) == StrConstant.AI_VECTOR_CORE_PMU_EVENTS:
        table_name = DBNameConstant.TABLE_AIV_METRIC_SUMMARY
        data = _get_output_event_counter(curs, result_dir, DBNameConstant.TABLE_AIV_METRIC_SUMMARY)
    return data, table_name


def get_task_based_core_data(result_dir: str, db_name: str, params: dict) -> tuple:
    """
    get aic and aiv data
    """
    conn, curs = DBManager.check_connect_db(result_dir, db_name)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    data, table_name = _get_task_based_core_data(params, curs, result_dir)
    if not data:
        return MsvpConstant.MSVP_EMPTY_DATA
    count = DBManager.fetch_all_data(curs, "select count(*) from {} ".format(table_name))
    DBManager.destroy_db_connect(conn, curs)
    return data[0], data[1:], count


def get_output_tasktype(cursor: any, param: dict) -> list:
    """
    get task scheduler data
    """
    exist = DBManager.judge_table_exist(cursor, "ReportTask")
    if not exist:
        return []
    sql = (
        "select ROUND(TimeRatio, {0}),ROUND(Time, {0}),Count,ROUND(Avg, {0}),ROUND(Min, {0}),ROUND(Max, {0}),"
        "ROUND(Waiting, {0}),ROUND(Running, {0}),ROUND(Pending, {0}), Type,API,task_id,stream_id,"
        "batch_id from ReportTask where device_id=?".format(NumberConstant.ROUND_THREE_DECIMAL)
    )
    report_task_data = DBManager.fetch_all_data(cursor, sql, (param.get(StrConstant.PARAM_DEVICE_ID),))
    return report_task_data


def get_metric_header(cursor: any, table_name: str) -> list:
    """
    calculate metric name
    """
    sql = "select * from {};".format(table_name)
    cursor.execute(sql)
    # get MetricSummary column name
    # if name equal total_time, add ms suffix
    headers = ['Task ID', "Stream ID", "Op Name"]
    metrics = Utils.generator_to_list(item[0] for item in cursor.description)
    add_aicore_units(metrics)
    if len(metrics) > 2:
        headers = headers + metrics[:-3]  # task id pos 0， stream id pos 1

    return headers


def get_output_event_counter(cursor: any, result_dir: str) -> list:
    """
    get ai core event count data
    """
    return _get_output_event_counter(cursor, result_dir, DBNameConstant.TABLE_METRIC_SUMMARY)


def _get_event_counter_metric_res(cursor: any, result_dir: str, table_name: str) -> list:
    metric_result = []
    headers = get_metric_header(cursor, table_name)
    sql = "select * from {0} order by task_id;".format(table_name)
    result = cursor.execute(sql).fetchall()
    res = []
    for items in result:
        tuples = [round(i, NumberConstant.DECIMAL_ACCURACY) if is_number(str(i)) else i for i in items]
        res.append(tuples)
    res = add_op_total(res, result_dir)
    metric_res = cal_metrics(res, metric_result, headers)
    return metric_res


def _get_output_event_counter(cursor: any, result_dir: str, table_name: str) -> list:
    """
    get ai core event count data by table name
    """
    sample_config = ConfigMgr.pre_check_sample(result_dir, 'ai_core_profiling_events')
    if not sample_config:
        return []
    if not DBManager.judge_table_exist(cursor, table_name):
        return []
    try:
        return _get_event_counter_metric_res(cursor, result_dir, table_name)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return []


def _get_opname(task: list, result_dir: str, curs_ge: any) -> str:
    op_name = Constant.NA
    db_name = DBNameConstant.DB_GE_INFO
    sql_to_get_opname = "select op_name from {} where task_id=? and stream_id=? and batch_id=?".format(
        DBNameConstant.TABLE_GE_TASK
    )
    if not os.path.exists(PathManager.get_db_path(result_dir, db_name)):
        db_name = DBNameConstant.DB_RTS_TRACK
        sql_to_get_opname = "select kernelname from {} where task_id=? and stream_id=? and batch_id=?".format(
            DBNameConstant.TABLE_TASK_TRACK
        )
        if not os.path.exists(PathManager.get_db_path(result_dir, db_name)):
            return op_name

    if len(task) >= 4:
        # task[-3]:task_id task[-2]:stream_id task[-1]:batch_id
        op_name = curs_ge.execute(sql_to_get_opname, (task[-3], task[-2], task[-1])).fetchone()
        op_name = op_name[0] if op_name else Constant.NA
    return op_name


def get_opname(task: list, result_dir: str, curs_ge: any) -> str:
    """
    query and obtain op names.
    :return: op names
    """
    op_name = Constant.NA
    if not curs_ge:
        return op_name
    try:
        op_name = _get_opname(task, result_dir, curs_ge)
        return op_name
    except sqlite3.Error:
        logging.error("get op name error.")
        return op_name


def add_op_total(result: list, result_dir: str) -> list:
    """
    add opname and total time to ai core result
    """
    res = []
    conn_ge, curs_ge = DBManager.check_connect_db(result_dir, DBNameConstant.DB_GE_INFO)
    if not conn_ge or not curs_ge:
        conn_ge, curs_ge = DBManager.check_connect_db(result_dir, DBNameConstant.DB_RTS_TRACK)
    for task in result:
        if len(task) > 1:
            op_name = get_opname(task, result_dir, curs_ge)
            res.append(tuple([op_name] + list(task)))
    DBManager.destroy_db_connect(conn_ge, curs_ge)
    return res


def cube_usage(config_dict: dict, value: list) -> list:
    """
    add cube usage column
    Numeric unit: aic_frequency: MHz, task_duration: ns
    """
    ratio_index = config_dict.get('mac_ratio_index', None)
    if not ratio_index or value[ratio_index] == Constant.NA:
        value.append(Constant.NA)
    elif not NumberConstant.is_zero(
        min(
            value[ratio_index],
            value[config_dict.get('total_cycles_index')],
            value[config_dict.get('task_duration_index')],
        )
    ):
        # Calculation formula: aic_total_cycles/ (aic_frequency * ai_core_num * task_duration)
        usage = (
            value[config_dict.get('total_cycles_index')]
            / (
                config_dict.get('aic_frequency')
                * config_dict.get('ai_core_num')
                * value[config_dict.get('task_duration_index')]
            )
            * NumberConstant.NS_TO_US
        )
        usage = round(usage * 100, NumberConstant.ROUND_THREE_DECIMAL)
        value.append(usage)
    else:
        value.append(0)
    return value


def add_mem_bound(value: list, vec_index: int, mac_index: int, mte2_index: int) -> list:
    """
    add memory bound column
    :param value: ready to appended value
    :param vec_index: vector ratio index
    :param mac_index: mac ratio index
    :param mte2_index: mte2 ratio index
    :return: appended value
    """
    if value[vec_index] == Constant.NA:
        value.append(Constant.NA)
    elif not NumberConstant.is_zero(max(value[vec_index], value[mac_index])):
        value.append(
            round(
                float(value[mte2_index] / max(value[vec_index], value[mac_index])), NumberConstant.ROUND_THREE_DECIMAL
            )
        )
    else:
        value.append(0)
    return value


def cal_metrics(result: list, metric_result: list, headers: list) -> list:
    """
    calculate results for aic/aiv metrics
    :param result: DB results
    :param metric_result: metrics value
    :param headers: report header
    :return:metric_result
    """
    if not result:
        return metric_result
    bound_flag = False
    vec_index = 0
    mac_index = 0
    mte2_index = 0
    if StrConstant.MAC_RATIO in headers and StrConstant.VEC_RATIO in headers and StrConstant.MTE2_RATIO in headers:
        mte2_index = headers.index(StrConstant.MTE2_RATIO)
        vec_index = headers.index(StrConstant.VEC_RATIO)
        mac_index = headers.index(StrConstant.MAC_RATIO)
        headers.append("memory_bound")
        bound_flag = True
    if "device_id" in headers:
        headers.remove("device_id")
    for value in result:
        value = list(value)
        if len(value) < 8:  # 8 is minimum length of value
            continue
        # insert task_id into first place of the result list
        value.insert(0, value[NumberConstant.METRICS_TASK_INDEX])
        # insert stream_id into second place of the result
        value.insert(1, value[NumberConstant.METRICS_STREAM_INDEX])
        # remove redundant task_id, stream_id, iterid data
        value.pop(NumberConstant.METRICS_TASK_INDEX)
        value.pop(NumberConstant.METRICS_STREAM_INDEX)
        value.pop(NumberConstant.METRICS_ITER_INDEX)
        if value[0] == -1:
            value[0] = 'total'  # task id
            value[1] = 'N/A'  # stream id
        if bound_flag:
            value = add_mem_bound(value, vec_index, mac_index, mte2_index)
        ai_core_time_transform(headers, value)
        metric_result.append(value)
    # move total to the start
    for i, _ in enumerate(metric_result):
        if metric_result[i] and metric_result[i][0] == 'total':
            metric_result.insert(0, metric_result.pop(i))
            break
    metric_result = [tuple(headers)] + metric_result
    return metric_result


def ai_core_time_transform(headers: list, value: list) -> None:
    """
    transform the ai core metrics.
    """
    for index, header in enumerate(headers):
        if header.find("(us)") != -1:
            value[index] = round(float(value[index]) * NumberConstant.NS_TO_US, NumberConstant.DECIMAL_ACCURACY)
