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
from decimal import Decimal

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from common_func.file_manager import FileOpen
from host_prof.host_cpu_usage.model.cpu_time_info import CpuTimeInfo
from host_prof.host_cpu_usage.model.host_cpu_usage import HostCpuUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


class HostCpuUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host cpu usage data
    """

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.cpu_info = []
        self.cpu_usage_info = []
        self.process_usage_info = []
        self.sort_map = {}

    @staticmethod
    def _compute_jiffies(curr: float, last: float) -> float:
        """
        compute jiffies
        """
        return (curr.utime - last.utime) + (curr.stime - last.stime)

    def init(self: any) -> None:
        """
        init class
        """
        self.set_model(HostCpuUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        implement parent class for parse data
        """
        try:
            with FileOpen(self.file_name) as file:
                self._parse_cpu_info()
                self._parse_cpu_usage(file.file_reader)
                logging.info("Finish parsing host cpu usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error(
                "Error in parsing host cpu usage data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH
            )
        finally:
            pass

    def get_cpu_usage_data(self: any) -> dict:
        """
        return cpu usage data
        """
        with self.cur_model as model:
            if model.has_cpu_usage_data():
                res = model.get_cpu_usage_data()
                return res
        return MsvpConstant.EMPTY_DICT

    def get_timeline_header(self: any) -> list:
        """
        return cpu usage timeline header
        """
        pid = InfoConfReader().get_json_pid_data()
        result = [["process_name", pid, InfoConfReader().get_json_tid_data(), "CPU Usage"]]
        cpu_list = self._get_cpu_list()
        for index, cpu_info in enumerate(cpu_list):
            cpu_no = " ".join(["CPU", cpu_info[0]])
            result.append(["thread_name", pid, index, cpu_no])
            result.append(["thread_sort_index", pid, index, index])
            self.sort_map[cpu_no] = index
        return result

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        result = self.get_cpu_usage_data()
        for data in result:
            data.append(self.sort_map.get(data[0]))
        return result

    def get_summary_data(self: any) -> tuple:
        """
        get summary data
        """
        with self.cur_model as model:
            if model.has_cpu_usage_data():
                res = model.get_num_of_used_cpus()
                if res:
                    total_cpu_nums = InfoConfReader().get_cpu_info()[0]
                    return [tuple([total_cpu_nums, *res])]
        return MsvpConstant.EMPTY_LIST

    def _parse_cpu_info(self: any) -> None:
        """
        parse cpu info
        """
        # first line: cpu_num clk
        self.cpu_info = InfoConfReader().get_cpu_info()
        self.cur_model.insert_cpu_info_data(self.cpu_info)

    def _process_per_usage(self: any, curr_info: list, last_info: list) -> None:
        """ "
        compute every cpu usage data
        """
        curr_info_data = {"curr_timestamp": curr_info[0], "curr_jiffies": curr_info[1], "curr_data": curr_info[2]}
        last_info_data = {"last_timestamp": last_info[0], "last_jiffies": last_info[1], "last_data": last_info[2]}

        delta_jiffies = curr_info_data.get("curr_jiffies") - last_info_data.get("last_jiffies")
        if not self.cpu_info or delta_jiffies == 0:
            return
        cpu_jiffies, total_delta = self._compute_process_usage(curr_info_data, delta_jiffies, last_info_data)
        self._compute_cpu_usage(cpu_jiffies, curr_info_data, delta_jiffies, last_info_data)
        self._compute_cpu_usage_average(curr_info_data, delta_jiffies, last_info_data, total_delta)

    def _compute_cpu_usage_average(self, curr_info_data, delta_jiffies, last_info_data, total_delta):
        usage = 0
        cpu_num = self.cpu_info[0]
        if not NumberConstant.is_zero(cpu_num):
            # compute cpu avg usage
            usage = (total_delta * NumberConstant.PERCENTAGE / (delta_jiffies * cpu_num)).quantize(
                NumberConstant.USAGE_PLACES
            )
        self.cpu_usage_info.append(
            [last_info_data.get("last_timestamp"), curr_info_data.get("curr_timestamp"), 'Avg', float(usage)]
        )

    def _compute_cpu_usage(self, cpu_jiffies, curr_info_data, delta_jiffies, last_info_data):
        # compute cpu usage,count multi process on same cpu
        for cpu_no, jiffies in cpu_jiffies.items():
            usage = (jiffies * NumberConstant.PERCENTAGE / delta_jiffies).quantize(NumberConstant.USAGE_PLACES)
            # Shield the error value caused by cpu switching
            if usage > 100:
                continue
            self.cpu_usage_info.append(
                [last_info_data.get("last_timestamp"), curr_info_data.get("curr_timestamp"), str(cpu_no), float(usage)]
            )

    def _compute_process_usage(self, curr_info_data, delta_jiffies, last_info_data):
        total_delta = 0
        cpu_jiffies = {}
        for pid, cpu_times in curr_info_data.get("curr_data").items():
            if pid not in last_info_data.get("last_data"):
                continue
            last_cputimes = last_info_data.get("last_data")[pid]
            process_jiffies = self._compute_jiffies(cpu_times, last_cputimes)
            process_jiffies = min(process_jiffies, delta_jiffies)
            usage = (process_jiffies * NumberConstant.PERCENTAGE / delta_jiffies).quantize(NumberConstant.USAGE_PLACES)

            self.process_usage_info.append(
                [
                    curr_info_data.get("curr_timestamp"),
                    str(curr_info_data.get("curr_jiffies")),
                    pid,
                    cpu_times.tid,
                    str(process_jiffies),
                    str(cpu_times.cpu_no),
                    float(usage),
                ]
            )
            if cpu_times.cpu_no in cpu_jiffies:
                cpu_jiffies[cpu_times.cpu_no] += process_jiffies
            else:
                cpu_jiffies[cpu_times.cpu_no] = process_jiffies
            total_delta += process_jiffies
        return cpu_jiffies, total_delta

    def _parse_cpu_usage(self: any, cpu_file: any) -> None:
        """
        parse cpu usage
        """
        curr_timestamp = None
        last_timestamp = None
        curr_jiffies = None
        last_jiffies = None
        uptime_valid = False
        last_data = {}
        curr_data = {}
        cpu_clk = self.cpu_info[1]

        for line in cpu_file:
            if line.startswith('time'):  # find timestamp
                if last_jiffies is not None:  # compute last process cpu usage
                    curr_info = (curr_timestamp, curr_jiffies, curr_data)
                    last_info = (last_timestamp, last_jiffies, last_data)
                    self._process_per_usage(curr_info, last_info)

                # save process time
                last_data = Utils.dict_copy(curr_data)
                last_jiffies = curr_jiffies
                last_timestamp = curr_timestamp
                curr_data.clear()
                # /proc/uptime
                curr_timestamp = line.split()[1]
                uptime_valid = True
            elif uptime_valid:  # uptime
                curr_jiffies = Decimal(line.split()[0]) * cpu_clk
                uptime_valid = False
            else:
                cur_cpu_time_info = CpuTimeInfo(line)
                curr_data[cur_cpu_time_info.pid] = cur_cpu_time_info

        # last one
        if curr_data and last_data:
            curr_info = (curr_timestamp, curr_jiffies, curr_data)
            last_info = (last_timestamp, last_jiffies, last_data)
            self._process_per_usage(curr_info, last_info)

        self.cur_model.insert_cpu_usage_data(self.cpu_usage_info)
        self.cur_model.insert_process_usage_data(self.process_usage_info)

    def _get_cpu_list(self: any) -> dict:
        """
        get cpu list from db
        """
        with self.cur_model as model:
            if model.has_cpu_usage_data():
                res = model.get_cpu_list()
                return res
        return MsvpConstant.EMPTY_DICT
