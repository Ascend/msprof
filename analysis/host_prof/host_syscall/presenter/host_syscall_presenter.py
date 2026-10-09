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
import re
import time

from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_host_syscall_compiles
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number
from common_func.singleton import singleton
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase
from host_prof.host_syscall.model.host_syscall import HostSyscall


class TimeInfo:
    """
    time info data class
    """

    def __init__(self: any, *args: any) -> None:
        self._raw_data = args[0]
        self._start_time_ms = args[1]
        self._duration_ms = args[2]

    @property
    def raw_data(self: any) -> list:
        """
        get raw data
        :return: raw data
        """
        return self._raw_data

    @property
    def start_time_ms(self: any) -> any:
        """
        get start_time_ms
        :return: start time
        """
        return self._start_time_ms

    @property
    def duration_ms(self: any) -> any:
        """
        get duration_ms
        :return:duration time
        """
        return self._duration_ms


@singleton
class PerfGapTime:
    """
    singleton class for record time gap.And gap time is used to convert perf time to raw time
    """

    def __init__(self: any) -> None:
        self._init_flag = False
        self._gap_time_us = 0

    def set_gap_time(self: any, gap_time: float) -> None:
        """
        set the perf gap time
        :param gap_time: hap time
        :return: None
        """
        self._gap_time_us = gap_time

    def set_init_flag(self: any, flag: bool) -> None:
        """
        set perf init flag
        :param flag: init flag
        :return: true or false
        """
        self._init_flag = flag

    def is_init(self: any) -> bool:
        """
        check perf init or not.
        :return: True or False
        """
        return self._init_flag

    def get_gap_time(self: any) -> float:
        """
        get perf gap time
        :return: gap time
        """
        return self._gap_time_us


@singleton
class PthreadGapTime:
    """
    singleton class for record time gap.And gap time is used to convert pthread time to raw time
    """

    def __init__(self: any) -> None:
        self._init_flag = False
        self._gap_time_us = 0

    def set_gap_time(self: any, gap_time: float) -> None:
        """
        set the pthread gap time
        :param gap_time: gap time
        :return: None
        """
        self._gap_time_us = gap_time

    def is_init(self: any) -> bool:
        """
        check pthread init or not.
        :return: True or False
        """
        return self._init_flag

    def set_init_flag(self: any, flag: bool) -> None:
        """
        set pthread init flag
        :param flag: true or false
        :return:
        """
        self._init_flag = flag

    def get_gap_time(self: any) -> float:
        """
        :return: gap pthread time
        """
        return self._gap_time_us


class HostSyscallPresenter(HostProfPresenterBase):
    """
    class for parsing host os runtime api data
    """

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.pid = InfoConfReader().get_json_pid_data()

    @staticmethod
    def check_api_name(api_name: str) -> bool:
        """
        filter special name
        :param api_name: api name
        :return: True or False
        """
        for one_filter in StrConstant.API_FUNC_NAME_FILTER:
            if one_filter is not None and one_filter.search(api_name) is not None:
                return True
        return False

    @staticmethod
    def get_command_api_info(raw_data_list: list, pid: int) -> tuple:
        """
        get command api info
        :param raw_data_list:list of raw data
        :param pid: pid
        :return: command result
        """
        command_name = ""
        command_tid = ""
        api_name = ""
        command_api_list = raw_data_list[1].strip().split()
        if "/" in raw_data_list[1].split('(')[0]:
            if len(command_api_list) == 2:
                command_name = command_api_list[0].split('/')[0]
                command_tid = command_api_list[0].split('/')[1]
                api_name = command_api_list[1].split('(')[0]
        else:
            command_name = '*'
            command_tid = pid
            api_name = command_api_list[0].split('(')[0] if len(command_api_list) != 0 else "NA"
        return command_name, command_tid, api_name

    @staticmethod
    def get_duration(raw_data_list: list) -> str:
        """
        get duration
        :param raw_data_list:list of raw data
        :return: duration time
        """
        duration_sec = ''
        if raw_data_list[-1].endswith(">"):
            duration_match = re.match(r'\d+\.\d+', raw_data_list[-1].strip("<").strip(">"))
            if duration_match is None:
                return duration_sec
            duration_sec = duration_match.group()
        return duration_sec

    @staticmethod
    def summary_reformat(summary_data: list) -> list:
        """
        timestamp: us
        """
        return [
            (
                data[0],
                data[1],
                data[2],
                data[3],
                data[4] / NumberConstant.CONVERSION_TIME,
                data[5],
                data[6] / NumberConstant.CONVERSION_TIME,
                data[7] / NumberConstant.CONVERSION_TIME,
                data[8] / NumberConstant.CONVERSION_TIME,
            )
            for data in summary_data
        ]

    @staticmethod
    def get_summary_api_info(result_dir: str) -> list:
        """
        get summary host os runtime api data
        :param result_dir: result dir
        :return: runtime api data
        """
        cur_model = HostSyscall(result_dir)
        cur_model.init()
        if not cur_model.check_db() or not cur_model.has_runtime_api_data():
            cur_model.finalize()
            return []
        res = cur_model.get_summary_runtime_api_data()
        res = HostSyscallPresenter.summary_reformat(res)
        cur_model.finalize()
        return res

    @staticmethod
    def _update_thread_gap_time(start_time: float) -> None:
        if not PthreadGapTime().is_init():
            start_time_raw, _ = InfoConfReader().get_collect_raw_time()
            if is_number(start_time_raw):
                # perfb - perfa + raw
                PthreadGapTime().set_init_flag(True)
                PthreadGapTime().set_gap_time(start_time - float(start_time_raw) / NumberConstant.CONVERSION_TIME)

    @classmethod
    def _get_time_info(cls: any, line: str) -> any:
        if len(line.strip()) == 0:
            return EmptyClass()
        raw_data_list = re.split(r':', line.strip())
        if len(raw_data_list) < 2:
            return EmptyClass()
        time_info = re.split(r'\s\(', raw_data_list[0].strip())
        if len(time_info) != 2:
            return EmptyClass()
        start_time_data = re.match(StrConstant.TIME_PATTERN, time_info[0].strip())
        if start_time_data is None:
            return EmptyClass()
        start_time_ms = start_time_data.group()
        duration_time_data = re.match(StrConstant.TIME_PATTERN, time_info[1].strip())
        if duration_time_data is None:
            return EmptyClass()
        duration_ms = duration_time_data.group()
        return TimeInfo(raw_data_list, start_time_ms, duration_ms)

    def init(self: any) -> None:
        """
        init syscall presenter
        :return: None
        """
        self.set_model(HostSyscall(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        parse syscall or pthread api data
        :return: None
        """
        try:
            with FileOpen(self.file_name, "r") as file:
                host_syscall_file_patterns = get_host_syscall_compiles()
                if get_file_name_pattern_match(os.path.basename(self.file_name), *host_syscall_file_patterns):
                    self._parse_syscall_api(file.file_reader)
                else:
                    self._parse_pthread_api(file.file_reader)
                logging.info("Finish parsing os runtime api data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error(
                "Error in parsing os runtime api data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH
            )
        finally:
            pass

    def get_runtime_api_data(self: any) -> list:
        """
        get runtime api data from result
        :return: runtime api data
        """
        if not self.cur_model.check_db() or not self.cur_model.has_runtime_api_data():
            self.cur_model.finalize()
            return []
        res = self.cur_model.get_runtime_api_data()
        self.cur_model.finalize()
        return res

    def get_timeline_data(self: any) -> list:
        """
        get os runtime api data,
        :return: format [[name, tid, ts ,dur]]
        """
        runtime_api_data = self.get_runtime_api_data()
        result = []
        for data_item in runtime_api_data:
            # 'name', 'tid', 'ts', 'dur'
            temp_data = [
                data_item[3],
                int(data_item[1]),
                int(data_item[2]),
                InfoConfReader().trans_into_local_time(data_item[7], is_host=True),
                (float(data_item[8]) - float(data_item[7])) / NumberConstant.CONVERSION_TIME,
            ]
            result.append(temp_data)
        return result

    def get_timeline_header(self: any) -> list:
        """
        get timeline header
        :return: timeline headers
        """
        pid = InfoConfReader().get_json_pid_data()
        result = [["process_name", pid, InfoConfReader().get_json_tid_data(), "OS Runtime API"]]
        tid_list = self._get_tid_list()
        for tid in tid_list:
            result.append(["thread_name", pid, tid[0], "Thread " + str(tid[0])])
            result.append(["thread_sort_index", pid, tid[0], tid[0]])
        return result

    def _get_tid_list(self: any) -> list:
        """
        get tid list from db
        :return: tids
        """
        if not self.cur_model.check_db() or not self.cur_model.has_runtime_api_data():
            self.cur_model.finalize()
            return []
        res = self.cur_model.get_all_tid()
        self.cur_model.finalize()
        return res

    def _parse_pthread_api(self: any, file: any) -> None:
        # get pthread data
        for line in file:
            if len(line.strip()) == 0:
                continue
            raw_data_list = line.split()
            duration_sec = HostSyscallPresenter.get_duration(raw_data_list)
            # raw_data_list format [pid xx] timestamp exe->api_name '=' 0 <dur>
            if not duration_sec or len(raw_data_list) < 5:
                continue
            command_tid = raw_data_list[1].strip(']')
            if not is_number(raw_data_list[2]):
                continue
            # to us
            start_time = float(raw_data_list[2]) * NumberConstant.SEC_TO_US
            HostSyscallPresenter._update_thread_gap_time(start_time)
            start_time = start_time - PthreadGapTime().get_gap_time()
            if raw_data_list[3].find("->") >= 0:
                api_name = raw_data_list[3].split("->")[1].split("(")[0]
            else:
                api_name = raw_data_list[4]
            if api_name == "pthread_once":
                continue
            end_time = start_time + float(duration_sec) * NumberConstant.SEC_TO_US
            write_list = [
                "",
                self.pid,
                command_tid,
                api_name,
                "",
                float(duration_sec) * NumberConstant.NS_TIME_RATE,
                "",
                start_time * NumberConstant.USTONS,
                end_time * NumberConstant.USTONS,
            ]
            self.cur_model.insert_single_data(write_list)

    def _parse_syscall_api(self: any, file: any) -> None:
        start_handle_sec = time.time()
        for line in file:
            time_info = self._get_time_info(line.strip())
            if isinstance(time_info, EmptyClass):
                continue

            if not PerfGapTime().is_init():
                start_time_ns, _ = InfoConfReader().get_collect_raw_time()
                if start_time_ns is not None:
                    PerfGapTime().set_init_flag(True)
                    # perfb - perfa + raw
                    PerfGapTime().set_gap_time(
                        float(time_info.start_time_ms) * NumberConstant.CONVERSION_TIME
                        - float(start_time_ns) / NumberConstant.CONVERSION_TIME
                    )
            command_name, command_tid, api_name = HostSyscallPresenter.get_command_api_info(
                time_info.raw_data, self.pid
            )
            if not HostSyscallPresenter.check_api_name(api_name):
                continue
            real_start_us = (
                float(time_info.start_time_ms) * NumberConstant.CONVERSION_TIME - PerfGapTime().get_gap_time()
            )
            tran_duration_us = float(time_info.duration_ms) * NumberConstant.CONVERSION_TIME
            real_end_us = real_start_us + tran_duration_us
            end_time_ms = float(time_info.start_time_ms) + float(time_info.duration_ms)
            if float(end_time_ms) / NumberConstant.CONVERSION_TIME > start_handle_sec:
                continue
            call_api_info = [
                command_name,
                self.pid,
                command_tid,
                api_name,
                float(time_info.start_time_ms) * NumberConstant.MS_TO_NS,
                tran_duration_us * NumberConstant.USTONS,
                end_time_ms * NumberConstant.MS_TO_NS,
                real_start_us * NumberConstant.USTONS,
                real_end_us * NumberConstant.USTONS,
            ]
            self.cur_model.insert_single_data(call_api_info)
