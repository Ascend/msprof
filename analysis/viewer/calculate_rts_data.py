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
import multiprocessing
import os
import sqlite3
from functools import reduce
from operator import add

from common_func.common import call_sys_exit
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import config_file_obj
from common_func.msvp_common import MsvpCommonConst
from common_func.msvp_common import error
from common_func.path_manager import PathManager
from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.utils import Utils
from framework.load_info_manager import LoadInfoManager
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from profiling_bean.db_dto.step_trace_dto import IterationRange


class CalculateRtsDataConst:
    """
    for calculate rts const
    """

    FILE_NAME = os.path.basename(__file__)
    TYPE = "ai_core"
    MAX_LENGTH = 20000
    RETENTION_PRECISION = 6
    PERCENT = 100
    # task state
    WAITING = 1
    RUNNING = 2
    COMPLETE = 3
    PENDING = 4
    AI_CORE_START = 7
    AI_CORE_DONE = 8
    AI_CPU_PHASES_TWO_START = 9

    # timeline data index
    TASK_ID_INDEX = 5
    STREAM_ID_INDEX = 6

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "CalculateRtsDataConst"

    def file_name(self: any) -> str:
        """
        file name
        """
        return self.FILE_NAME


def _get_task_schedule_sql() -> str:
    sql = (
        "select ifnull(taskType, '')||'|'||ifnull(api, '')||'|'||task_id||'|'||stream_id||'|'||batch_id, "
        "sum(case when waittime=0 or complete=0.0 then 0 "
        "when running<waittime then complete-waittime "
        "else running-waittime end) as waiting,"
        " sum(case when running=0 or complete=0.0 then 0 "
        "else complete-running end) as running, "
        "sum(case when pendingtime=0 or complete=0.0 then 0 "
        "else complete-pendingtime end) as pending,"
        "min(case when complete=0.0 then 0 "
        "when running=0 then complete-pendingtime "
        "else complete-running end) as min_time,"
        "max(case when complete=0.0 then 0 "
        "when running=0 then complete-pendingtime "
        "else complete-running end) as max_time, "
        "sum(case when complete=0.0 then 0 "
        "when running=0 then complete-pendingtime "
        "else complete-running end)/count(rowid) as avg, "
        "count(rowid) from TaskTime where device_id = ?"
        "group by taskType, api, task_id, stream_id, batch_id;"
    )
    return sql


def calculate_task_schedule_data(curs: any, device: str) -> list:
    """
    calculate task schedule data
    """
    task_data = {}
    timeline_exist = DBManager.judge_table_exist(curs, "TaskTime")
    if not timeline_exist:
        logging.info("TaskTime table doesn't exist")
        return []

    total_data = []
    sql = _get_task_schedule_sql()
    task_data["tasktime_data"] = DBManager.fetch_all_data(curs, sql, (device,))
    task_data = calculate_task_time(task_data)
    state_time_data = task_data.get("type_state_time")
    try:
        for i in state_time_data:
            tasktype, api, task_id, stream_id, batch_id = i.split('|')
            _per_state_time = state_time_data.get(i)
            total_data.append(
                (
                    str(
                        round(
                            float(
                                _per_state_time.get('running')
                                / task_data.get("total_time")
                                * CalculateRtsDataConst.PERCENT
                            ),
                            CalculateRtsDataConst.RETENTION_PRECISION,
                        )
                    ),
                    _per_state_time.get('running') / NumberConstant.NS_TO_US,
                    _per_state_time.get('count'),
                    _per_state_time.get('avg') / NumberConstant.NS_TO_US,
                    _per_state_time.get('min') / NumberConstant.NS_TO_US,
                    _per_state_time.get('max') / NumberConstant.NS_TO_US,
                    _per_state_time.get('waiting') / NumberConstant.NS_TO_US,
                    _per_state_time.get('running') / NumberConstant.NS_TO_US,
                    _per_state_time.get('pending') / NumberConstant.NS_TO_US,
                    StrConstant.TASK_TYPE_MAPPING.get(str(tasktype), "unknown {}".format(str(tasktype))),
                    api,
                    task_id,
                    stream_id,
                    device,
                    batch_id,
                )
            )
        return sorted(total_data, key=lambda x: float(x[0].replace('%', '')), reverse=True)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError, ZeroDivisionError) as err:
        logging.error(repr(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return []


def calculate_task_time(task_data: dict) -> dict:
    """
    calculate task time
    """
    type_state_time = calculate_type_state_time(task_data)
    task_data["type_state_time"] = type_state_time
    task_data["total_time"] = float(sum(v['running'] for v in list(type_state_time.values())))
    return task_data


def calculate_type_state_time(task_data: dict) -> dict:
    """
    calculate task status time by multiprocessing
    """
    type_state_time = {}
    for task in task_data["tasktime_data"]:
        type_state_time[task[0]] = {
            'waiting': task[1],
            'running': task[2],
            'pending': task[3],
            'count': task[-1],
            'min': task[4],
            'max': task[5],
            'avg': task[6],
        }
    return type_state_time


def _compute_multi_process(timeline_data: list, project_path: str, task_time: dict) -> list:
    cpu_count = multiprocessing.cpu_count() / 3 * 3
    processes = []
    step = len(timeline_data)
    if len(timeline_data) > CalculateRtsDataConst.MAX_LENGTH and not NumberConstant.is_zero(cpu_count):
        step = int(len(timeline_data) / cpu_count) - 1
    count = 0
    for i in range(0, len(timeline_data), step):
        start_tag = 0
        stop_tag = 0
        if i != 0:
            while timeline_data[i - start_tag][-1] != CalculateRtsDataConst.COMPLETE:
                start_tag += 1
            _start = i - start_tag + 1
        else:
            _start = 0
        if i + step < len(timeline_data):
            while timeline_data[i + step - stop_tag][-1] != CalculateRtsDataConst.COMPLETE:
                stop_tag += 1
            _stop = i + step - stop_tag + 1
            process = multiprocessing.Process(
                target=calculate_timeline_task_time, args=(timeline_data[_start:_stop], task_time, count, project_path)
            )
        else:
            _stop = -1
            process = multiprocessing.Process(
                target=calculate_timeline_task_time, args=(timeline_data[_start:], task_time, count, project_path)
            )
        process.start()
        processes.append(process)
        count += 1
    return processes


def multi_calculate_task_cost_time(timeline_data: list, project_path: str) -> list:
    """
    calculate timeline task time data by multiprocessing
    """
    manager = multiprocessing.Manager()
    task_time = manager.dict()
    processes = _compute_multi_process(timeline_data, project_path, task_time)

    for pro in processes:
        pro.join()
    task_time = reduce(add, Utils.generator_to_list(data for data in list(task_time.values())))
    return task_time


def calculate_timeline_task_time(timeline_data: list, task_time: dict, pid: int, project_path: str) -> None:
    """
    calculate timeline task time data
    :param timeline_data: replayId,device_id,'','',taskType, task_id,stream_id,timeStamp,taskState
    :param task_time: result
    :param pid: pid
    """
    LoadInfoManager().load_info(project_path)
    current_task_id = 0
    current_stream_id = 0
    waiting_status_index, pending_status_index, running_status_index = [-1, -1, -1]
    insert_data = []
    index = 0
    data_len = len(timeline_data)
    try:
        while index < data_len:
            if (
                timeline_data[index][CalculateRtsDataConst.TASK_ID_INDEX] != current_task_id
                or timeline_data[index][CalculateRtsDataConst.STREAM_ID_INDEX] != current_stream_id
            ):
                current_task_id = timeline_data[index][CalculateRtsDataConst.TASK_ID_INDEX]
                current_stream_id = timeline_data[index][CalculateRtsDataConst.STREAM_ID_INDEX]
                waiting_status_index, pending_status_index, running_status_index = [-1, -1, -1]
            pending_status_index, running_status_index, waiting_status_index = handle_task_time(
                index, insert_data, timeline_data, [waiting_status_index, pending_status_index, running_status_index]
            )
            index = index + 1
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(err)
    task_time[pid] = insert_data


def handle_task_time(index: int, insert_data: list, timeline_data: list, status_index_list: list) -> tuple:
    """
    handle task time with task state
    :param index: current index
    :param insert_data: result
    :param timeline_data: replayId,device_id,'','',taskType, task_id,stream_id,timeStamp,taskState
    :param status_index_list: index list
    :return: pending_status_index, running_status_index, waiting_status_index
    """
    waiting_status_index, pending_status_index, running_status_index = status_index_list
    if timeline_data[index][-1] == CalculateRtsDataConst.COMPLETE:
        tmp = {
            'waiting': calculate_time(timeline_data, waiting_status_index),
            "pending": calculate_time(timeline_data, pending_status_index),
            'running': calculate_time(timeline_data, running_status_index),
            'complete': InfoConfReader().time_from_syscnt(timeline_data[index][-2]),
        }
        insert_data.append(
            timeline_data[index][0:7]
            + (tmp.get('waiting', 0), tmp.get('pending', 0), tmp.get('running', 0), tmp.get('complete', 0))
        )
        waiting_status_index, pending_status_index, running_status_index = [-1, -1, -1]
    elif timeline_data[index][-1] == CalculateRtsDataConst.WAITING:
        waiting_status_index = index
    elif timeline_data[index][-1] == CalculateRtsDataConst.PENDING:
        pending_status_index = index
    elif timeline_data[index][-1] == CalculateRtsDataConst.RUNNING:
        running_status_index = index
    elif timeline_data[index][-1] not in [
        CalculateRtsDataConst.AI_CORE_START,
        CalculateRtsDataConst.AI_CORE_DONE,
        CalculateRtsDataConst.AI_CPU_PHASES_TWO_START,
    ]:
        logging.error("Unrecognized tag %s", timeline_data[index][-1])
    return pending_status_index, running_status_index, waiting_status_index


def calculate_time(timeline_data: list, index: int) -> int:
    """
    calculate time with record index
    """
    return InfoConfReader().time_from_syscnt(timeline_data[index][-2]) if index >= 0 else 0


def check_aicore_events(events: list, is_custom: bool = False) -> None:
    """
    check aicore events
    """
    if not events:
        error(CalculateRtsDataConst.FILE_NAME, 'Insert data error, aicore event list is empty. ')
        call_sys_exit(NumberConstant.ERROR)
    ai_core_config = config_file_obj(file_name=MsvpCommonConst.AI_CORE)
    formula_key = Utils.generator_to_list(item[0] for item in ai_core_config.items('events'))
    if is_custom:
        formula_key = Utils.generator_to_list(item[0] for item in ai_core_config.items('custom'))
    for event in events:
        if event not in formula_key:
            error(CalculateRtsDataConst.FILE_NAME, 'Invalid event {} . '.format(event))


def create_ai_event_tables(sample_config: dict, curs: any, device: str) -> None:
    """
    create ai event tables
    """
    logging.info('start create ai core events tables')
    ai_core_events = sample_config.get("ai_core_profiling_events", "").split(",")
    check_aicore_events(ai_core_events, is_custom=judge_custom_pmu_scene(sample_config))
    pmu_event_lst = Utils.generator_to_list(pmu_event.replace('0x', 'r') + " numeric" for pmu_event in ai_core_events)
    sql = (
        "CREATE TABLE IF NOT EXISTS EventCount ("
        + ",".join(pmu_event_lst)
        + ",task_cyc numeric, task_id INT, stream_id INT, block_num INT, "
        "core_num INT, device_id INT)"
    )
    curs.execute(sql)
    ai_core_events = Utils.generator_to_list(ai_core_events[i : i + 8] for i in range(0, len(ai_core_events), 8))
    for _, event in enumerate(ai_core_events):
        for index, value in enumerate(event):
            curs.execute(
                "create table if not exists {tablename} (timestamp INT, pmucount INT, "
                " replayid INT, task_id INT, stream_id INT, device_id INT)".format(tablename=value.replace('0x', 'r'))
            )
            sql = (
                "insert into {tablename} select timestamp, event{index},"
                "replayid, task_id, stream_id, device_id from EventCounter "
                "where device_id = ?;".format(tablename=value.replace('0x', 'r'), index=index + 1)
            )
            curs.execute(sql, (device,))
    curs.execute(
        "create table IF NOT EXISTS task_cyc(timestamp INTEGER, "
        "pmucount INTEGER,replayid INT, task_id INT, stream_id INT, device_id INT)"
    )
    sql = (
        "insert into task_cyc select timestamp, task_cyc, replayid, task_id, stream_id, "
        "device_id from EventCounter where device_id = ?"
    )
    curs.execute(sql, (device,))
    logging.info('create event tables finished')


def judge_custom_pmu_scene(sample_config: dict, metrics_type: str = "ai_core_metrics") -> bool:
    """
    Check whether the current PMU setting is customized.
    """
    metrics = sample_config.get(metrics_type)
    return metrics.startswith("Custom") if metrics else False


def _get_pmu_data(res: list, col_name_list: list, device: str, curs: any) -> None:
    # get pmu data
    for table in col_name_list:
        sql = 'select pmucount from {} where device_id=? order by rowid;'.format(table)
        result = DBManager.fetch_all_data(curs, sql, (device,))
        pmu_data = Utils.generator_to_list(i[0] for i in result)
        res.append(pmu_data)


def _get_stream_and_task_id(res: list, col_name_list: list, device: str, curs: any) -> None:
    # get stream_id and task_id
    sql = 'select task_id, stream_id from {} where device_id=? order by rowid;'.format(col_name_list[0])
    result = DBManager.fetch_all_data(curs, sql, (device,))
    res.append(Utils.generator_to_list(i[0] for i in result))
    res.append(Utils.generator_to_list(i[1] for i in result))


def _get_block_core_device_data(res: list, device: str, curs: any) -> None:
    # get block data
    sql = 'select block from EventCounter where tasktype=0 and replayid=0 and device_id=?;'
    block_data = DBManager.fetch_all_data(curs, sql, (device,))
    res.append(Utils.generator_to_list(i[0] for i in block_data))
    # get core num
    core_num = InfoConfReader().get_data_under_device("ai_core_num")
    res.append([core_num] * len(block_data))

    # get device_id
    res.append([device] * len(block_data))


def insert_event_value(curs: any, conn: any, device: str) -> None:
    """
    insert event value into EventCount
    """
    headers = DBManager.get_table_headers(curs, "EventCount")
    col_name_filter = ('device_id', 'block_num', 'core_num', 'task_id', 'stream_id')
    col_name_list = []
    for i in headers:
        if i not in col_name_filter:
            col_name_list.append(i)

    if not col_name_list:
        return
    tmp = []

    _get_pmu_data(tmp, col_name_list, device, curs)
    _get_stream_and_task_id(tmp, col_name_list, device, curs)
    _get_block_core_device_data(tmp, device, curs)

    event_result = list(zip(*tmp))

    if not event_result:
        return

    # insert into db
    val_len = "?," * (len(event_result[0]) - 1) + '?'
    sql = "insert into EventCount values ({value})".format(value=val_len)
    DBManager.executemany_sql(conn, sql, event_result)


def create_metric_table(conn: any, metrics: list, table_name: str) -> bool:
    """
    insert event value into metric summary
    """
    sql = 'CREATE TABLE IF NOT EXISTS {name}({column})'.format(
        column=','.join(metric.replace('(ms)', '').replace('(GB/s)', '') + ' numeric' for metric in metrics)
        + ', task_id INT, '
        'stream_id INT, core_type INT, batch_id INT',
        name=table_name,
    )
    return DBManager.execute_sql(conn, sql)


def _query_limit_and_offset(iter_range: IterationRange, curs: any) -> list:
    result = []
    sql = (
        f"select min(iter_id), max(iter_id) from {DBNameConstant.TABLE_STEP_TRACE_DATA} "
        f"where index_id>=? and index_id<=? and model_id=?"
    )
    data = DBManager.fetchone(curs, sql, (*iter_range.get_iteration_range(), iter_range.model_id))
    if data:
        start_iter_id, end_iter_id = data
        sql = f"select sum(ai_core_num) from {DBNameConstant.TABLE_STEP_TRACE_DATA} where iter_id < ?"
        offset_result = DBManager.fetchone(curs, sql, (start_iter_id,))
        offset = 0 if offset_result[0] is None else offset_result[0]

        sql = f"select sum(ai_core_num) from {DBNameConstant.TABLE_STEP_TRACE_DATA} where iter_id>=? and iter_id<=?"
        limit_result = DBManager.fetchone(curs, sql, (start_iter_id, end_iter_id))
        limit = limit_result[0] if limit_result else 0
        result = [limit, offset]
    return result


def get_limit_and_offset(result_dir: str, iter_range: IterationRange) -> list:
    """
    get limit and offset for ai core within the index id.
    :param result_dir: project path
    :param iter_range: iteration range
    :return: limit and offset
    """
    result = []
    db_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_STEP_TRACE)
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
        return result
    if "ai_core_num" not in DBManager.get_table_headers(curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
        return result
    try:
        result = _query_limit_and_offset(iter_range, curs)
        return result
    except sqlite3.Error as err:
        logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        return result
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_metrics_from_sample_config(
    project_path: str, metrics_type: str = StrConstant.AI_CORE_PROFILING_METRICS
) -> list:
    """
    get ai core metric from sample json.
    """
    sample_config = ConfigMgr.read_sample_config(project_path)
    metrics = ['total_time(ms)', 'total_cycles']
    if judge_custom_pmu_scene(sample_config, metrics_type=metrics_type):
        metrics.extend(sample_config.get('ai_core_profiling_events').replace('0x', 'r').split(','))
        return metrics
    metrics_list = AiCoreMetricsManager.AICORE_METRICS_LIST

    if sample_config.get(metrics_type) not in metrics_list:
        return []
    sample_metrics = metrics_list.get(sample_config.get(metrics_type)).split(",")
    for tmp in sample_metrics:
        if tmp.lower() not in Utils.generator_to_list(
            item[0] for item in config_file_obj(file_name=MsvpCommonConst.AI_CORE).items('metrics')
        ):
            logging.error('Invalid metric %s .', tmp)
    new_metrics = []
    if sample_config.get(metrics_type) in {
        AiCoreMetricsManager.PMU_PIPE,
        AiCoreMetricsManager.PMU_PIPE_EXCT,
        AiCoreMetricsManager.PMU_PIPE_EXECUT,
        AiCoreMetricsManager.PMU_SCALAR_RATIO,
        AiCoreMetricsManager.PMU_PIPE_STALL_CYCLE,
    }:
        for metric in sample_metrics[:-1]:
            if metric.endswith(StrConstant.RATIO_EXTRA_NAME):
                new_metrics.append(metric[: -NumberConstant.EXTRA_RATIO_NAME_LEN] + "time")
            elif metric.endswith(StrConstant.RATIO_NAME):
                new_metrics.append(metric[: -NumberConstant.RATIO_NAME_LEN] + "time")
            new_metrics.append(metric)
        if sample_config.get(metrics_type) == AiCoreMetricsManager.PMU_PIPE_EXECUT:
            new_metrics.append(sample_metrics[-1][: -NumberConstant.RATIO_NAME_LEN] + "time")
        new_metrics.append(sample_metrics[-1])
    sample_metrics = new_metrics if new_metrics else sample_metrics
    metrics.extend(sample_metrics)
    cal = CalculateAiCoreData(project_path)
    cal.add_fops_header(metrics_type, metrics)
    return metrics
