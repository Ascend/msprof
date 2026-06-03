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

import logging
import os
import re

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.file_manager import FileOpen


class MultiProcessCbConstant:
    """
    multi process cb constant class
    """

    PMU_MODE_CORE = 1
    PMU_MODE_NO_CORE = 2
    PMU_MODE_INVALID = 3
    SAMPLE_LEN = 100000
    PROCESS_NUMBER_END = 9
    PROCESS_NUMBER_START = 0
    SAMPLE_COUNT = 13
    HOT_FUNC_LEN = 3

    def get_multi_process_constant_class_name(self: any) -> any:
        """
        get multi process constant class name
        """
        return self.__class__.__name__

    def get_multi_process_constant_class_member(self: any) -> any:
        """
        get multi process constant class member num
        """
        return self.__dict__


def _update_hot_func(hot_func: list, hot_func_temp: list) -> None:
    if len(hot_func_temp) > MultiProcessCbConstant.HOT_FUNC_LEN:
        _middle = ' '.join(hot_func_temp[1:-1])
        hot_func_temp = [hot_func_temp[0], _middle, hot_func_temp[-1]]
    hot_func.append(hot_func_temp[0])
    if 'unknown' in hot_func_temp[1]:
        hot_func.extend(['unknown', 'unknown'])
    else:
        hot_func = manipulation_hot_func_data(hot_func_temp, hot_func)
    hot_func.append(hot_func_temp[2].strip("()"))


def _do_process_stack(perf_out: any) -> tuple:
    hot = True
    hot_func = []
    stack_funcs = []
    while True:
        line = perf_out.readline(Constant.MAX_READ_LINE_BYTES)
        if not line or line == '\n':
            break
        if hot:
            hot_func_temp = line.strip().split()
            if len(hot_func_temp) < MultiProcessCbConstant.HOT_FUNC_LEN:
                continue
            _update_hot_func(hot_func, hot_func_temp)
            hot = False
        else:
            stack_funcs.append(line.strip())
    return hot_func, stack_funcs


def process_stack(perf_out: any) -> list:
    """
    process stack
    """
    try:
        hot_func, stack_funcs = _do_process_stack(perf_out)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(err)
        return []
    if stack_funcs:
        hot_func.append('<-'.join(stack_funcs))
    else:
        hot_func.append('unknown')

    return hot_func


def manipulation_hot_func_data(hot_func_temp: list, hot_func: list) -> list:
    """
    manipulation hot func data
    """
    _opt = hot_func_temp[1]
    if _opt.count('+') > 1:
        _tmp_add = _opt.split('+')
        _tmp_add_result = '+'.join([_tmp_add[0], _tmp_add[1]])

        hot_func.extend([_tmp_add_result, _tmp_add[-1]])
    else:
        native_tmp = hot_func_temp[1].split('+')
        hot_func.extend([native_tmp[0], native_tmp[-1]])
    return hot_func


def _exec_query_sql(lock: any, info: dict, samples: list, *conn_items: any) -> None:
    conn, curs = conn_items
    lock.acquire()
    for sample in samples:
        curs.execute(info.get("query"), sample)
    conn.commit()
    lock.release()


def _multiprocess_callback_helper(args: any, file_obj: any, info: dict, *conn_element: any) -> None:
    conn, curs, lock = conn_element
    if args.get("pro_no") != MultiProcessCbConstant.PROCESS_NUMBER_END:
        file_obj.seek(info.get("end_pos"))
        file_obj.readline(Constant.MAX_READ_LINE_BYTES)
        while True:
            _next_line = file_obj.readline(Constant.MAX_READ_LINE_BYTES)
            if _next_line == '\n' or not _next_line:
                info["end_pos"] = file_obj.tell()
                break

    file_obj.seek(args["start_pos"])
    if args.get("pro_no") != MultiProcessCbConstant.PROCESS_NUMBER_START:
        file_obj.readline(Constant.MAX_READ_LINE_BYTES)
        while True:
            _next_line = file_obj.readline(Constant.MAX_READ_LINE_BYTES)
            if _next_line == '\n' or not _next_line:
                break

    samples = manipulation_data(file_obj, curs, conn, info, lock)
    if samples:
        _exec_query_sql(lock, info, samples, conn, curs)


def multiprocess_callback(args: any) -> None:
    """
    insert data into
    """
    info = {
        'end_pos': args["end_pos"],
        'replayid': args["replayid"],
        "pmu_mode": MultiProcessCbConstant.PMU_MODE_INVALID,
        "query": 'INSERT INTO OriginalData VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)',
        "sample_count": MultiProcessCbConstant.SAMPLE_COUNT,
        'cpu_id': args['id'],
    }
    lock = args['lock']
    conn, curs = DBManager.create_connect_db(args["dbname"])
    try:
        with FileOpen(args["filename"], 'r') as file_obj:
            _multiprocess_callback_helper(args, file_obj.file_reader, info, conn, curs, lock)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError, StopIteration) as reason:
        logging.exception("Exception occurred: %s", reason)
    finally:
        DBManager.destroy_db_connect(conn, curs)
        logging.info("process %d end insert OriginalData", os.getpid())


def manipulation_data(file_obj: any, curs: any, conn: any, info: dict, lock: any) -> list:
    """
    manipulation data
    """
    sample = []
    samples = []
    end_pos = info["end_pos"]

    while True:
        line = file_obj.readline(Constant.MAX_READ_LINE_BYTES)
        line = line.strip()

        if line.startswith(':'):
            logging.warning("Invalid control cpu data, check ai_ctrl_cpu.data.x.slice_x file around '%s'", line)
            continue
        matched, pmu_mode = line_match(line, info)
        if not matched:
            if file_obj.tell() >= end_pos:
                break
            continue
        info["matched"] = matched
        info["pmu_mode"] = pmu_mode
        info["sample"] = sample
        info["samples"] = samples
        samples, sample = insert_data(info, curs, conn, file_obj, lock)
        sample = []
        if file_obj.tell() >= end_pos:
            break
    return samples


def line_match(line: str, info: dict) -> tuple:
    """
    line match branch
    """
    pmu_pat = re.compile(
        r'^(.{{1,50}}) +(\d{{1,20}})/(\d{{1,20}}) +\[(00[{}])\]'
        r' +(\d{{1,20}}\.\d{{1,20}}): +(\d{{1,20}}) +(\S{{1,50}}?):'.format(info['cpu_id'])
    )

    pmu_pat_raw = re.compile(
        r'^(.{{1,50}}) +(\d{{1,20}})/(\d{{1,20}}) +\[(00[{}])\] +(\d{{1,20}}\.\d{{1,20}}): +(\d{{1,20}})'
        r' +raw +(\S{{1,50}}):'.format(info['cpu_id'])
    )

    matched = pmu_pat.search(line)
    pmu_mode = MultiProcessCbConstant.PMU_MODE_CORE
    if not matched:
        matched = pmu_pat_raw.search(line)
    return matched, pmu_mode


def insert_data(info: dict, curs: any, conn: any, file_obj: any, lock: any) -> tuple:
    """
    insert data
    """
    if info["pmu_mode"] == MultiProcessCbConstant.PMU_MODE_NO_CORE:
        pmu = list(info["matched"].groups())
        pmu[-1] = pmu[-1].replace("0x", "r")
        pmu.insert(3, -1)
        info["sample"].extend(pmu)
    else:
        pmu = list(info["matched"].groups())
        pmu[-1] = pmu[-1].replace("0x", "r").replace('0', '')
        info["sample"].extend(pmu)
    info["sample"].extend(process_stack(file_obj))
    info["sample"].append(info["replayid"])
    if len(info["sample"]) == info["sample_count"]:
        info["samples"].append(info["sample"])
    else:
        logging.info(info["sample"])
    if len(info["samples"]) == MultiProcessCbConstant.SAMPLE_LEN:
        lock.acquire()
        logging.info("process %d begin insert OriginalData 100000", os.getpid())
        for i in info["samples"]:
            curs.execute(info["query"], i)
        conn.commit()
        logging.info("process %d end insert OriginalData 100000", os.getpid())
        lock.release()
        info["samples"] = []

    return info["samples"], info["sample"]
