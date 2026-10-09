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
import os
import sqlite3
from collections import OrderedDict

from common_func.common import byte_per_us2_mb_pers
from common_func.common import ns2_us
from common_func.common import warn
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.json_manager import JsonManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from viewer.get_msvp_summary import get_aicore_utilization
from viewer.get_msvp_timeline_training import get_dvpp_engine_id
from viewer.get_msvp_timeline_training import get_dvpp_ids
from viewer.get_msvp_timeline_training import get_dvpp_legend
from viewer.get_msvp_timeline_training import get_dvpp_total_data


class TraceViewer:
    """
    Trace viewer object
    """

    def __init__(self: any, scope: str) -> None:
        self.scope = scope

    @staticmethod
    def multiple_name_dump(
        trace_data: dict,
        legend: dict,
        delta_dev: any,
        pid: int = TraceViewHeaderConstant.DEFAULT_PID_VALUE,
        tid: int = TraceViewHeaderConstant.DEFAULT_TID_VALUE,
    ) -> list:
        """
        Multiple task id
        :param trace_data: trace data
        :param legend: legend
        :param delta_dev: delta time for device
        :param pid: pid
        :param tid: tid
        :return: result
        """
        _result = []
        if not isinstance(trace_data, dict) or not trace_data:
            return _result
        for key in list(trace_data.keys()):
            column_trace_data = Utils.generator_to_list(
                [
                    key,
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=TraceViewer._cal_sys_time_us(delta_dev, item), use_us=True
                    ),
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

    @staticmethod
    def format_trace_data(data_item: list, trace_name: str, timestamp: any, pid: int, tid: int) -> tuple:
        """
        PCIe data format
        :param data_item: data
        :param trace_name: tarce name
        :param timestamp: time stamp
        :param pid: pid
        :param tid: tid
        :return: result
        """
        tmp_args_data = {"Tx": 0, "Rx": 0}
        if trace_name == "PCIe_post":
            tmp_args_data["Tx"] = byte_per_us2_mb_pers(data_item[4])
            tmp_args_data["Rx"] = byte_per_us2_mb_pers(data_item[-7])
        elif trace_name == "PCIe_nonpost":
            tmp_args_data["Tx"] = byte_per_us2_mb_pers(data_item[7])
            tmp_args_data["Rx"] = byte_per_us2_mb_pers(data_item[-4])
        elif trace_name == "PCIe_cpl":
            tmp_args_data["Tx"] = byte_per_us2_mb_pers(data_item[10])
            tmp_args_data["Rx"] = byte_per_us2_mb_pers(data_item[-1])
        else:
            tmp_args_data["Tx"] = ns2_us(data_item[13])
            tmp_args_data["Rx"] = 0
        tmp_trace_data = (trace_name, timestamp, pid, tid, tmp_args_data)
        return tmp_trace_data

    @staticmethod
    def _cal_sys_time_us(delta_dev: any, item: list) -> int:
        return int(float(item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS


def _get_hccs_result(hccs_data: list, trace_parser: any) -> list:
    delta_dev = InfoConfReader().get_delta_time()
    _trace = []
    json_data = [InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data()]
    _result = TraceViewManager.metadata_event([["process_name", json_data[0], json_data[1], trace_parser.scope]])
    for _data_item in hccs_data:
        _trace.extend(
            [
                [
                    'Tx',
                    InfoConfReader().trans_into_local_time(
                        int(float(_data_item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.NS_TO_US,
                        use_us=True,
                    ),
                    json_data[0],
                    json_data[1],
                    OrderedDict([('Tx(MB/s)', _data_item[1])]),
                ],
                [
                    'Rx',
                    InfoConfReader().trans_into_local_time(
                        int(float(_data_item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.NS_TO_US,
                        use_us=True,
                    ),
                    json_data[0],
                    json_data[1],
                    OrderedDict([('Rx(MB/s)', _data_item[2])]),
                ],
            ]
        )
    _result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, _trace))
    return _result


def _get_hcc_events_data(start: int, end: int, devid: str, curs: any) -> list:
    _data = []
    if abs(end - 0) <= NumberConstant.FLT_EPSILON:
        sql = "SELECT timestamp, txThroughput, rxThroughput FROM {0} WHERE device_id IS ?".format(
            DBNameConstant.TABLE_HCCS_EVENTS
        )
        _data = DBManager.fetch_all_data(curs, sql, (devid,))
    else:
        sql = (
            "SELECT timestamp, txThroughput, rxThroughput FROM {0} WHERE device_id "
            "IS ? AND timestamp between ? and ?".format(DBNameConstant.TABLE_HCCS_EVENTS)
        )
        _data = DBManager.fetch_all_data(curs, sql, (devid, start, end))
    return _data


def get_hccs_timeline(result_dir: str, devid: str, start: int, end: int) -> any:
    """
    Report hccs timeline
    """
    conn, curs = DBManager.check_connect_db(result_dir, DBNameConstant.DB_HCCS)
    if not conn or not curs:
        logging.error("Failed to connect hccs.db.")
        return []
    _data = _get_hcc_events_data(start, end, devid, curs)
    trace_parser = TraceViewer('HCCS')
    _result = _get_hccs_result(_data, trace_parser)
    DBManager.destroy_db_connect(conn, curs)
    return _result


def _get_pcie_data(pcie_data: list, trace_parser: any) -> list:
    delta_dev = InfoConfReader().get_delta_time()
    pid = InfoConfReader().get_json_pid_data()
    tid = InfoConfReader().get_json_tid_data()
    trace_data = []
    meta_data = [["process_name", pid, tid, trace_parser.scope]]
    _trace = TraceViewManager.metadata_event(meta_data)
    for data_item in pcie_data:
        if len(data_item) >= 23:
            # check whether the tmp_data has at least 23 domains.
            trace_data.append(
                trace_parser.format_trace_data(
                    data_item,
                    "PCIe_post",
                    InfoConfReader().trans_into_local_time(
                        int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS, use_us=True
                    ),
                    pid,
                    tid,
                )
            )
            trace_data.append(
                trace_parser.format_trace_data(
                    data_item,
                    "PCIe_nonpost",
                    InfoConfReader().trans_into_local_time(
                        int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS, use_us=True
                    ),
                    pid,
                    tid,
                )
            )
            trace_data.append(
                trace_parser.format_trace_data(
                    data_item,
                    "PCIe_cpl",
                    InfoConfReader().trans_into_local_time(
                        int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS, use_us=True
                    ),
                    pid,
                    tid,
                )
            )
            trace_data.append(
                trace_parser.format_trace_data(
                    data_item,
                    "PCIe_nonpost_latency",
                    InfoConfReader().trans_into_local_time(
                        int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS, use_us=True
                    ),
                    pid,
                    tid,
                )
            )

    _trace += TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)
    return _trace


def get_pcie_timeline(param: dict) -> any:
    """
    Report PCIe timeline
    """
    conn, curs = DBManager.check_connect_db(param.get("project_path"), DBNameConstant.DB_PCIE)
    if not (conn and curs):
        logging.error("Failed to connect pcie database. ")
        return []
    sql = (
        "select * from {} where device_id=? and timestamp between ? "
        "and ? and tx_p_bandwidth_max >= tx_p_bandwidth_min;".format(DBNameConstant.TABLE_PCIE)
    )
    data = DBManager.fetch_all_data(curs, sql, (param['device_id'], param['start_time'], param['end_time']))
    trace_parser = TraceViewer('PCIe')
    _trace = _get_pcie_data(data, trace_parser)
    DBManager.destroy_db_connect(conn, curs)
    return _trace


def get_dvpp_timeline(param: dict) -> any:
    """
    function that provides dvpp data searched from peripheral.db
    :param param: get dvpp timeline params
    :return: json format data
    """
    conn, curs = DBManager.check_connect_db(param['project_path'], 'peripheral.db')
    _result = []
    if not (conn and curs):
        logging.error("The db doesn't exist!")
        return []
    try:
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_DVPP_ORIGIN):
            logging.error("The table doesn't exist.")
            return []
    except sqlite3.Error:
        logging.error("No data is collected.")
        return []
    else:
        dict_engine_id = get_dvpp_engine_id(Constant.DVPP_TYPE_NAME, conn)
        dvpp_list = get_dvpp_ids(conn) if "all" in param['dvppid'] else param['dvppid']
        _process_dvpp_timeline_data(_result, conn, dvpp_list, dict_engine_id, param)
        return _result
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _process_dvpp_timeline_data(result: list, conn: any, dvpp_list: list, engine_ids: dict, params: dict) -> None:
    delta_dev = InfoConfReader().get_delta_time()
    sort_index = 0
    for dvpp_id in dvpp_list:
        for _type in list(engine_ids.keys()):
            trace_parser = TraceViewer('DVPP/{}/{}'.format(dvpp_id, _type))
            result += TraceViewManager.metadata_event(
                [
                    ["process_name", sort_index, InfoConfReader().get_json_tid_data(), trace_parser.scope],
                    ["process_sort_index", sort_index, InfoConfReader().get_json_tid_data(), sort_index],
                ]
            )
            for _id in engine_ids[_type]:
                slice_ = {}
                _dump_data = {}
                _param = {
                    'project_path': params['project_path'],
                    'start_time': params['start_time'],
                    'end_time': params['end_time'],
                    'device_id': params['device_id'],
                    'dvppid': dvpp_id,
                    'engine_type': Constant.DVPP_TYPE_NAME.index(_type),
                    'engine_id': _id,
                }
                slice_['_slice_time'], slice_['_slice_util'] = get_dvpp_total_data(_param, conn)
                _dump_data["slice_data"] = {
                    "Engine_{}/frame".format(_id): slice_.get('_slice_time', 0),
                    "Engine_{}/utilization".format(_id): slice_.get('_slice_util', 0),
                }
                _dump_data["legend"] = {
                    "Engine_{}/frame".format(_id): get_dvpp_legend()[0],
                    "Engine_{}/utilization".format(_id): get_dvpp_legend()[1],
                }
                result.extend(
                    trace_parser.multiple_name_dump(
                        _dump_data.get("slice_data", {}),
                        _dump_data.get("legend", ""),
                        delta_dev,
                        sort_index,
                        InfoConfReader().get_json_tid_data(),
                    )
                )
            sort_index += 1


def _get_network_data(table: dict, sql_: dict, curs: any, *param: any) -> dict:
    dev_id, start, end = param
    dump_data = OrderedDict([("results", {}), ("legends", {})])
    for _func_id in table.get('_func_list', []):
        for _direct in ['tx', 'rx']:
            sql_["_events"] = '{0}_bandwidth_efficiency,{0}_packets,{0}_error_rate,{0}_dropped_rate'.format(_direct)
            sql_['sentence'] = (
                "SELECT timestamp, {1} FROM {0} WHERE device_id "
                "IS ? AND func_id IS ? AND timestamp "
                "between ? and ?".format(table.get('_table', ''), sql_.get("_events", ''))
            )
            dump_data.get("results", {})['Port {}/{}'.format(_func_id[0], _direct.capitalize())] = (
                DBManager.fetch_all_data(curs, sql_.get('sentence', ''), (dev_id, _func_id[0], start, end))
            )
            dump_data.get("legends", {})['Port {}/{}'.format(_func_id[0], _direct.capitalize())] = (
                Utils.generator_to_list(item.replace("_", " ").title() for item in sql_.get("_events", "").split(','))
            )
    return dump_data


def get_network_timeline(result_dir: str, devid: str, start: int, end: int, collect_type: str) -> any:
    """
    Return trace-viewer json format RoCE/NIC timeline
    """
    table = {}
    if collect_type == 'roce':
        table['_db'] = DBNameConstant.DB_ROCE_RECEIVE
        table['_table'] = DBNameConstant.TABLE_ROCE_RECEIVE
        table['_header'] = 'RoCE'
    elif collect_type == 'nic':
        table['_db'] = DBNameConstant.DB_NIC_RECEIVE
        table['_table'] = DBNameConstant.TABLE_NIC_RECEIVE
        table['_header'] = 'NIC'
    else:
        logging.error("Failed to get collection type.")
        return []
    conn, curs = DBManager.check_connect_db(result_dir, table.get('_db', ""))
    if not conn or not curs:
        logging.error("Failed to connect {%s}.", table.get('_db'))
        return []

    table['delta_dev'] = InfoConfReader().get_delta_time()
    sql_ = {'sentence': "SELECT DISTINCT func_id FROM {} WHERE device_id IS ?".format(table.get('_table', ''))}
    table['_func_list'] = DBManager.fetch_all_data(curs, sql_.get('sentence', ''), (devid,))
    trace_parser = TraceViewer(table.get('_header', ''))
    _result = []
    _result += TraceViewManager.metadata_event(
        [
            [
                "process_name",
                InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(),
                trace_parser.scope,
            ]
        ]
    )
    dump_data = _get_network_data(table, sql_, curs, devid, start, end)
    _result += trace_parser.multiple_name_dump(
        dump_data.get("results", []),
        dump_data.get("legends", ""),
        table.get('delta_dev', 0),
        InfoConfReader().get_json_pid_data(),
        InfoConfReader().get_json_tid_data(),
    )
    DBManager.destroy_db_connect(conn, curs)
    return _result


def _get_aicore_utilization_data(aicore_result: dict, pid: int, tid: int) -> list:
    trace_data = []
    for aicore_name in aicore_result.get('data', {}).get('usage', {}):
        for result_value in aicore_result.get('data', {}).get('usage', {}).get(aicore_name, ''):
            if aicore_name == 'average':
                trace_name = 'Average'
            else:
                trace_name = 'Core {}'.format(aicore_name)
            trace_data.append(
                (
                    trace_name,
                    InfoConfReader().trans_into_local_time(
                        float(result_value[0]) * NumberConstant.MS_TIME_RATE, use_us=True
                    ),
                    pid,
                    tid,
                    OrderedDict([('Utilization(%)', result_value[1])]),
                )
            )
    return trace_data


def get_aicore_utilization_timeline(project_path: str) -> any:
    """
    get ai core utilization trace view data
    """
    try:
        aicore_result = get_aicore_utilization(
            project_path,
            NumberConstant.DEFAULT_NUMBER,
            NumberConstant.DEFAULT_START_TIME,
            NumberConstant.DEFAULT_END_TIME,
        )
    except sqlite3.Error as error:
        logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        return []

    if len(aicore_result) >= NumberConstant.MAX_STR_LENGTH:
        warn(
            os.path.basename(__file__),
            "The size of AI Core utilization data exceeds 8 MB. Parsing the data may take several minutes..",
        )

    aicore_result = JsonManager.loads(aicore_result)
    if not aicore_result:
        logging.error("Failed to get ai core utilization data.")
        return []

    pid = InfoConfReader().get_json_pid_data()
    tid = InfoConfReader().get_json_tid_data()

    meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_AI_CORE_UTILIZATION]]
    result_data = TraceViewManager.metadata_event(meta_data)
    try:
        trace_data = _get_aicore_utilization_data(aicore_result, pid, tid)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
        logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        return []
    result_data += TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)
    return result_data
