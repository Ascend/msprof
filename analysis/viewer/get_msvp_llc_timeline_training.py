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

import sqlite3
import logging
from collections import OrderedDict

from common_func.common import CommonConstant
from common_func.config_mgr import ConfigMgr
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from viewer.get_trace_timeline import TraceViewer


def _multiple_name_dump_with_absolute_ns(trace_parser: any, trace_data: dict, legend: dict, pid: int, tid: int) -> list:
    _result = []
    if not isinstance(trace_data, dict) or not trace_data:
        return _result
    for key in list(trace_data.keys()):
        column_trace_data = Utils.generator_to_list(
            [
                key,
                InfoConfReader().format_absolute_ns_to_us(item[0]),
                pid,
                tid,
                OrderedDict(list(zip(legend[key], item[1:]))),
            ]
            for item in trace_data[key]
        )
        _result += TraceViewManager.column_graph_trace(
            TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, column_trace_data
        )
    return _result


def _fill_llc_nomini(param: dict, results: dict, legends: dict, sample_config: dict, curs: any) -> None:
    for llc_id in range(0, 4):
        param['llc_id'] = llc_id
        result_data = OrderedDict()
        result_data["llc_id"] = llc_id
        result_data["start_time"] = NumberConstant.DEFAULT_START_TIME
        result_data["stop_time"] = NumberConstant.DEFAULT_END_TIME
        result_data["rate"] = 'MB/s'
        result_data['mode'] = sample_config.get('llc_profiling', "")
        data_total = get_llc_metric_data(param, curs)
        llc_event = {"Throughput": ["Throughput(MB/s)"], "Hit Rate": ["Hit Rate(%)"]}
        if data_total:
            for _direct in ["Throughput", "Hit Rate"]:
                results['LLC {} {}/{}'.format(llc_id, result_data['mode'].capitalize(), _direct)] = data_total.get(
                    _direct, 0
                )
                legends['LLC {} {}/{}'.format(llc_id, result_data['mode'].capitalize(), _direct)] = llc_event.get(
                    _direct, 0
                )


def get_llc_nomini_data(param: dict, sample_config: dict, curs: any) -> list:
    """
    get llc time series data
    """
    results = OrderedDict()
    legends = OrderedDict()
    try:
        _fill_llc_nomini(param, results, legends, sample_config, curs)
    except sqlite3.Error:
        logging.error("Failed to get data of llc.")
        return []
    trace_parser = TraceViewer("LLC")
    _trace_results = TraceViewManager.metadata_event(
        [["process_name", InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data(), "LLC"]]
    )
    if legends and results:
        _trace_results += _multiple_name_dump_with_absolute_ns(
            trace_parser, results, legends, InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data()
        )
        return _trace_results
    logging.error("No data is collected.")
    return []


def pre_check_llc(conn: any, curs: any, sample_config: dict, table_name: str) -> list:
    """
    Check LLC events
    """
    if not (conn and curs):
        logging.error("The db doesn't exist.")
        return []
    if not sample_config or not sample_config.get('llc_profiling'):
        logging.error("Failed to load llc profiling events from %s", CommonConstant.SAMPLE_JSON)
        return []
    if sample_config.get('llc_profiling') not in ['read', 'write', 'bandwidth', 'capacity']:
        logging.error("Invalid llc profiling events.")
        return []
    if not DBManager.judge_table_exist(curs, table_name):
        logging.error("The table doesn't exist.")
        return []
    return []


def get_llc_db_table(sample_config: dict) -> str:
    """
    get db, table and cols for llc data
    """
    if sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM:
        table_name = DBNameConstant.TABLE_LLC_METRIC_DATA
    elif sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM:
        table_name = DBNameConstant.TABLE_LLC_DSID
    else:
        table_name = StrConstant.LLC_METRICS_TABLE
    return table_name


def get_llc_timeline(param: dict) -> list:
    """
    get llc time series data
    """
    conn, curs = DBManager.check_connect_db(param[StrConstant.PARAM_RESULT_DIR], DBNameConstant.DB_LLC)
    sample_config = ConfigMgr.read_sample_config(param[StrConstant.PARAM_RESULT_DIR])
    table_name = get_llc_db_table(sample_config)

    llc_check_result = pre_check_llc(conn, curs, sample_config, table_name)
    if llc_check_result:
        DBManager.destroy_db_connect(curs, conn)
        return llc_check_result

    try:
        return get_llc_nomini_data(param, sample_config, curs)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        logging.error("Failed to get data of llc.")
        return []
    finally:
        DBManager.destroy_db_connect(curs, conn)


def get_llc_metric_data(param: dict, curs: any) -> dict:
    """
    get llc read/write time series data
    """
    metric_data = {}
    # If the end time is equal to 0, output data for all time
    sql = (
        'SELECT timestamp, throughput FROM {0} WHERE device_id = ? AND l3tId = ? AND timestamp between ? and ?;'.format(  # nosec B608
            StrConstant.LLC_METRICS_TABLE
        )
    )
    sql_ = 'SELECT timestamp, hitRate FROM {0} WHERE device_id = ? AND l3tId = ? AND timestamp between ? and ?;'.format(  # nosec B608
        StrConstant.LLC_METRICS_TABLE
    )
    metric_data['Throughput'] = DBManager.fetch_all_data(
        curs,
        sql,
        (
            param[StrConstant.PARAM_DEVICE_ID],
            param['llc_id'],
            NumberConstant.DEFAULT_START_TIME,
            NumberConstant.DEFAULT_END_TIME,
        ),
    )
    metric_data['Hit Rate'] = DBManager.fetch_all_data(
        curs,
        sql_,
        (
            param[StrConstant.PARAM_DEVICE_ID],
            param['llc_id'],
            NumberConstant.DEFAULT_START_TIME,
            NumberConstant.DEFAULT_END_TIME,
        ),
    )
    return metric_data


def _reformat_ddr_data(trace_parser: any, data_total: dict) -> list:
    delta_dev = InfoConfReader().get_delta_time()
    results = OrderedDict()
    legends = OrderedDict()
    ddr_events = {"Read": ["Read(MB/s)"], "Write": ["Write(MB/s)"]}
    meta_data = [
        ["process_name", InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data(), trace_parser.scope]
    ]
    _result = TraceViewManager.metadata_event(meta_data)
    for _direct in ["Read", "Write"]:
        results['DDR/{}'.format(_direct)] = data_total.get(_direct, 0)
        legends['DDR/{}'.format(_direct)] = ddr_events.get(_direct, 0)
        _result += trace_parser.multiple_name_dump(
            results, legends, delta_dev, InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data()
        )
    return _result


def get_ddr_timeline(param: dict) -> list:
    """
    get ddr time series data
    """
    conn, curs = DBManager.check_connect_db(param['project_path'], 'ddr.db')
    if not (conn and curs):
        logging.error("The db doesn't exist.")
        return []
    if not DBManager.judge_table_exist(curs, "DDROriginalData"):
        logging.error("The table doesn't exist.")
        return []
    data_total = get_ddr_metric_data(param, curs)
    if not data_total:
        logging.error("No data is collected.")
        return []
    trace_parser = TraceViewer("DDR")
    DBManager.destroy_db_connect(conn, curs)
    return _reformat_ddr_data(trace_parser, data_total)


def get_ddr_metric_data(param: dict, curs: any) -> dict:
    """
    get ddr read/write time series data
    """
    result_data = {}
    ddr_events = {'''flux_read''': "Read", '''flux_write''': "Write"}
    try:
        for key, value in ddr_events.items():
            # If the end time is equal to 0, output data for all time
            if abs(param['end_time'] - 0) <= NumberConstant.FLT_EPSILON:
                sql = 'select timestamp,{} from DDRMetricData where device_id = ?;'.format(key)  # nosec B608
                ddr_training_data = DBManager.fetch_all_data(curs, sql, (param['device_id']))
            else:
                sql = (
                    'select timestamp,{} from DDRMetricData where device_id = ? and timestamp between ? and ?;'.format(  # nosec B608
                        key
                    )
                )
                ddr_training_data = DBManager.fetch_all_data(
                    curs, sql, (param['device_id'], param['start_time'], param['end_time'])
                )
            if ddr_training_data:
                result_data[value] = ddr_training_data

        return result_data
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return {}


def _fill_hbm_data(param: dict, results: dict, legends: dict, curs: any) -> None:
    hbm_events = {"Read": ["Read(MB/s)"], "Write": ["Write(MB/s)"]}
    for hbm_id in range(0, 4):
        param['hbm_id'] = hbm_id
        data_total = get_hbm_bw_data(param, curs)
        if data_total:
            for _direct in ["Read", "Write"]:
                results['HBM {}/{}'.format(param['hbm_id'], _direct)] = data_total.get(_direct, 0)
                legends['HBM {}/{}'.format(param['hbm_id'], _direct)] = hbm_events.get(_direct, 0)


def _reformat_hbm_data(trace_parser: any, param: dict, curs: any) -> list:
    results = OrderedDict()
    legends = OrderedDict()
    _fill_hbm_data(param, results, legends, curs)
    _result = []
    if legends and results:
        meta_data = [
            [
                "process_name",
                InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(),
                trace_parser.scope,
            ]
        ]
        _result = TraceViewManager.metadata_event(meta_data)
        _result += _multiple_name_dump_with_absolute_ns(
            trace_parser, results, legends, InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data()
        )
    return _result


def get_hbm_timeline(param: dict) -> list:
    """
    get hbm time series data
    """
    conn, curs = DBManager.check_connect_db(param['project_path'], 'hbm.db')
    if not (conn and curs):
        logging.error("The db doesn't exist.")
        return []
    if not DBManager.judge_table_exist(curs, "HBMOriginalData"):
        DBManager.destroy_db_connect(conn, curs)
        logging.error("The table doesn't exist.")
        return []
    trace_parser = TraceViewer("HBM")
    try:
        _result = _reformat_hbm_data(trace_parser, param, curs)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        logging.error("Failed to get data of hbm.")
        return []
    else:
        if _result:
            return _result
        logging.error("No data is collected.")
        return []
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_hbm_bw_data(param: dict, curs: any) -> dict:
    """
    get hbm bandwidth data
    """
    result_data = OrderedDict()
    hbm_events = OrderedDict([('read', 'Read'), ('write', 'Write')])
    try:
        for key, value in hbm_events.items():
            # If the end time is equal to 0, output data for all time
            if abs(param['end_time'] - 0) <= NumberConstant.FLT_EPSILON:
                sql = 'select timestamp, bandwidth from HBMbwData where device_id = ? and hbmId = ? and event_type=?;'
                result_data[value] = DBManager.fetch_all_data(curs, sql, (param['device_id'], param['hbm_id'], key))
            else:
                sql = (
                    'select timestamp, bandwidth from HBMbwData '
                    'where device_id = ? and hbmId = ? and event_type=? and '
                    'timestamp between ? and ?'
                )
                result_data[value] = DBManager.fetch_all_data(
                    curs, sql, (param['device_id'], param['hbm_id'], key, param['start_time'], param['end_time'])
                )
        return result_data
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        logging.error("Get HBM data failed.")
        return {}
