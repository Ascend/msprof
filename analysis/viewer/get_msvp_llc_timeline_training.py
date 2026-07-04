# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import sqlite3
import logging
from collections import OrderedDict

from common_func.common import CommonConstant
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from msmodel.hardware.mini_llc_model import MiniLlcModel
from msmodel.hardware.mini_llc_model import cal_core2cpu
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


def get_llc_mini_data(param: dict, sample_config: dict, curs: any) -> list:
    """
    get llc time series data
    """
    if sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM:
        return get_llc_bandwidth(curs)
    if sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM:
        return get_llc_capacity(param, curs)
    logging.error("Failed to get data of llc.")
    return []


def _format_llc_trace_data(bandwidth_data: list, delta_dev: float) -> list:
    trace_data = []
    interval_time = [0] * len(bandwidth_data)
    for index, _bandwidth_data in enumerate(bandwidth_data):
        if (index + 1) < len(bandwidth_data):
            interval_time[index] = round(
                (float(bandwidth_data[index + 1][0]) - float(bandwidth_data[index][0])), CommonConstant.ROUND_SIX
            )
        else:
            interval_time[index] = interval_time[0]
        bandwidth_data_value = get_bandwidth_value(_bandwidth_data, interval_time[index])
        if not bandwidth_data_value:
            return []

        trace_data.extend(
            [
                [
                    "LLC/Read",
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=(delta_dev + _bandwidth_data[0]) * NumberConstant.MICRO_SECOND, use_us=True
                    ),
                    InfoConfReader().get_json_pid_data(),
                    InfoConfReader().get_json_tid_data(),
                    OrderedDict(
                        [
                            ("BandWidth(MB/s)", bandwidth_data_value[0]),
                            ("Hit Rate(%)", bandwidth_data_value[1]),
                            ("Hit BandWidth(MB/s)", bandwidth_data_value[2]),
                        ]
                    ),
                ],
                [
                    "LLC/Write",
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=(delta_dev + _bandwidth_data[0]) * NumberConstant.MICRO_SECOND, use_us=True
                    ),
                    InfoConfReader().get_json_pid_data(),
                    InfoConfReader().get_json_tid_data(),
                    OrderedDict(
                        [
                            ("BandWidth(MB/s)", bandwidth_data_value[3]),
                            ("Hit Rate(%)", bandwidth_data_value[4]),
                            ("Hit BandWidth(MB/s)", bandwidth_data_value[5]),
                        ]
                    ),
                ],
            ]
        )
    return trace_data


def get_llc_bandwidth(curs: any) -> list:
    """
    get trace view data of llc bandwith
    """
    delta_dev = InfoConfReader().get_delta_time()
    meta_data = [
        ["process_name", InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data(), "LLC Bandwidth"]
    ]
    result_data = TraceViewManager.metadata_event(meta_data)
    cols = (
        "timestamp, sum(read_allocate), sum(read_noallocate), sum(read_hit), "
        "sum(write_allocate), sum(write_noallocate), sum(write_hit)"
    )
    sql = "SELECT {cols} FROM {table_name} group by timestamp".format(  # nosec B608
        cols=cols, table_name=DBNameConstant.TABLE_LLC_METRIC_DATA
    )
    bandwidth_data = DBManager.fetch_all_data(curs, sql)
    bandwidth_data = Utils.generator_to_list(list(i) for i in bandwidth_data)
    if not bandwidth_data:
        logging.error("Failed to get read and write of llc_bandwidth.")
        return []

    trace_data = _format_llc_trace_data(bandwidth_data, delta_dev)
    if not trace_data:
        logging.error("Failed to get read and write of llc_bandwidth.")
        return []
    result_data.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data))
    return result_data


def _format_llc_capacity(dsid_sql_data: list) -> list:
    trace_data = []
    delta_dev = InfoConfReader().get_delta_time()
    for disid_data in dsid_sql_data:
        trace_data.extend(
            Utils.generator_to_list(
                [
                    "Core {}".format(i),
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=(delta_dev + disid_data[4]) * NumberConstant.MICRO_SECOND, use_us=True
                    ),
                    InfoConfReader().get_json_pid_data(),
                    InfoConfReader().get_json_tid_data(),
                    OrderedDict([("Capacity(MB)", round(disid_data[i], CommonConstant.ROUND_SIX))]),
                ]
                for i in range(0, 4)
            )
        )
    return trace_data


def _get_dsid_sql_data(params: dict, types: str, curs: any) -> list:
    project_path = params.get(StrConstant.PARAM_RESULT_DIR)

    core2cpu = cal_core2cpu(project_path, params.get(StrConstant.PARAM_DEVICE_ID))
    dsid_name = core2cpu[types]
    dsid_name = Utils.generator_to_list(
        "sum({dsid})*{LLC_CAPACITY}/({KILOBYTE}*{KILOBYTE})".format(
            LLC_CAPACITY=MiniLlcModel.LLC_CAPACITY, KILOBYTE=Constant.KILOBYTE, dsid=i
        )
        for i in dsid_name
    )
    sql = "select {column}, timestamp from {table_name} where device_id = ? group by timestamp".format(  # nosec B608
        column=",".join(dsid_name), table_name=DBNameConstant.TABLE_LLC_DSID
    )
    dsid_sql_data = curs.execute(sql, (params.get(StrConstant.PARAM_DEVICE_ID),)).fetchall()
    return dsid_sql_data


def get_llc_capacity(params: dict, curs: any) -> list:
    """
    get trace view data of llc capacity
    """
    types = StrConstant.AICPU if params.get(StrConstant.DATA_TYPE) == StrConstant.LLC_AICPU else StrConstant.CTRL_CPU
    types_map = {StrConstant.AICPU: "AI CPU", StrConstant.CTRL_CPU: "Ctrl CPU"}
    try:
        dsid_sql_data = _get_dsid_sql_data(params, types, curs)
    except sqlite3.Error:
        logging.error("Failed to get data of llc_capacity.")
        return []
    else:
        if not dsid_sql_data:
            logging.error("Failed to get read and write of llc_capacity.")
            return []
        dsid_sql_data = Utils.generator_to_list(list(i) for i in dsid_sql_data)
        trace_data = _format_llc_capacity(dsid_sql_data)
        meta_data = [
            [
                "process_name",
                InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(),
                "LLC of {}".format(types_map.get(types, "")),
            ]
        ]
        result_data = TraceViewManager.metadata_event(meta_data)
        result_data.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)
        )
        return result_data
    finally:
        pass


def get_bandwidth_value(bandwidth_data: list, interval_time: float) -> list:
    """
    get bandwidth value, hit rate hit bandwidth of llc bandwidth
    """

    # *_bandwidth equal sum of *_allocate and *_noallocate
    # read_hit_rate equal read_hit divided by read_bandwidth
    # hit_bandwidth equal read_hit or write_hit
    sum_re_allocate = bandwidth_data[1] + bandwidth_data[2]
    sum_wr_allocate = bandwidth_data[4] + bandwidth_data[5]

    read_bandwidth_value = 0.0
    read_hit_bandwidth_value = 0.0
    write_hit_bandwidth_value = 0.0
    write_bandwidth_value = 0.0
    if not NumberConstant.is_zero(interval_time):
        read_bandwidth_value = round(
            sum_re_allocate / interval_time * NumberConstant.LLC_CAPACITY_CONVERT_MB, CommonConstant.ROUND_SIX
        )
        read_hit_bandwidth_value = round(
            bandwidth_data[3] * NumberConstant.LLC_CAPACITY_CONVERT_MB / interval_time, CommonConstant.ROUND_SIX
        )
        write_bandwidth_value = round(
            sum_wr_allocate / interval_time * NumberConstant.LLC_CAPACITY_CONVERT_MB, CommonConstant.ROUND_SIX
        )
        write_hit_bandwidth_value = round(
            bandwidth_data[6] * NumberConstant.LLC_CAPACITY_CONVERT_MB / interval_time, CommonConstant.ROUND_SIX
        )

    read_hit_rate_value = 0.0
    if not NumberConstant.is_zero(sum_re_allocate):
        read_hit_rate_value = round(
            bandwidth_data[3] / sum_re_allocate * NumberConstant.PERCENTAGE, CommonConstant.ROUND_SIX
        )

    write_hit_rate_value = 0.0
    if not NumberConstant.is_zero(sum_wr_allocate):
        write_hit_rate_value = round(
            bandwidth_data[6] / sum_wr_allocate * NumberConstant.PERCENTAGE, CommonConstant.ROUND_SIX
        )

    bandwidth_value = [
        read_bandwidth_value,
        read_hit_rate_value,
        read_hit_bandwidth_value,
        write_bandwidth_value,
        write_hit_rate_value,
        write_hit_bandwidth_value,
    ]
    return bandwidth_value if bandwidth_value else []


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
        if ChipManager().is_chip_v1():
            return get_llc_mini_data(param, sample_config, curs)
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
