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

from common_func.common import error
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.file_manager import FileOpen
from host_prof.host_disk_usage.model.host_disk_usage import HostDiskUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


class HostDiskUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host disk usage data
    """

    FILE_NAME = os.path.basename(__file__)
    FREQ_TO_NS = 10**9
    TYPE_CONVERSION_DICT = {
        'B/s': 1.0 / NumberConstant.KILOBYTE,
        'K/s': 1.0,
        'M/s': NumberConstant.KILOBYTE,
        'G/s': NumberConstant.KILOBYTE**2,
    }

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.disk_usage_info = []

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get timeline header
        """
        return [
            ["process_name", InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data(), "Disk Usage"]
        ]

    def init(self: any) -> None:
        """
        init class
        """
        self.set_model(HostDiskUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        parse disk usage data
        """
        try:
            with FileOpen(self.file_name, "r") as file:
                self._parse_disk_usage(file.file_reader)
                logging.info("Finish parsing host disk usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error(
                "Error in parsing host disk usage data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH
            )

    def get_disk_usage_data(self: any) -> dict:
        """
        get disk usage data
        """
        with self.cur_model as model:
            if model.has_disk_usage_data():
                res = model.get_disk_usage_data()
                return res
        return MsvpConstant.EMPTY_DICT

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        disk_usage_data = self.get_disk_usage_data()
        result = []
        if not disk_usage_data:
            logging.error("No disk usage data...")
            return result

        for data_item in disk_usage_data.get("data"):
            temp_data = ["Disk Usage", data_item["start"], {"Usage(%)": data_item["usage"]}]
            result.append(temp_data)
        return result

    def get_summary_data(self: any) -> list:
        """
        get summary data
        """
        with self.cur_model as model:
            if model.has_disk_usage_data():
                res_read = model.get_recommend_value('disk_read', DBNameConstant.TABLE_HOST_DISK_USAGE)
                res_write = model.get_recommend_value('disk_write', DBNameConstant.TABLE_HOST_DISK_USAGE)
                return [tuple(res_read + res_write)]
        return MsvpConstant.EMPTY_LIST

    def _parse_disk_data(self: any, start_time: float, interval: float, usage_items: any) -> None:
        end_time = start_time
        for item in usage_items:
            start_time = end_time
            end_time = start_time + interval
            disk_read = item[1]
            disk_write = item[2]
            swap_in = item[3]
            io_percent = float(item[4])
            self.disk_usage_info.append([start_time, end_time, disk_read, disk_write, swap_in, io_percent])
        self.cur_model.insert_disk_usage_data(self.disk_usage_info)

    def _parse_disk_usage(self: any, file: any) -> None:
        start_time, usage_items = self._get_disk_usage_items(file)
        disk_freq = ConfigMgr.get_disk_freq(self.result_dir)
        if is_number(disk_freq) and disk_freq > 0:
            # convert freq to interval(ns)
            interval = 1 / float(disk_freq) * self.FREQ_TO_NS
            self._parse_disk_data(start_time, interval, usage_items)
        else:
            error(self.FILE_NAME, "Host disk freq is invalid, please check sample.json file!")

    def _convert_to_k(self: any, speed_type: str):
        """
        convert other speed type to KB/s
        """
        if speed_type in self.TYPE_CONVERSION_DICT:
            return self.TYPE_CONVERSION_DICT.get(speed_type, 1.0)
        else:
            logging.warning("Unknown speed type appears! Unknown type: %s", speed_type)
            return 1.0

    def _get_disk_usage_items(self: any, file: any) -> tuple:
        start_time = 0.0
        usage_items = []

        for line in file:
            fields = line.split()
            if len(fields) < 2:  # invalid format
                continue
            if not fields[1].isdigit():  # invalid pid
                continue
            if fields[0] == 'start_time' and fields[1].isdigit():
                start_time = float(fields[1])
                continue

            if len(fields) < 8:  # skip this line if #fields less than 8 to avoid index out of range
                continue

            pid = int(fields[1])
            try:
                disk_read = float(fields[4]) * self._convert_to_k(fields[5])
            except ValueError:
                logging.warning('host of disk read has a wrong data type!')
                disk_read = 0
            try:
                disk_write = float(fields[6]) * self._convert_to_k(fields[7])
            except ValueError:
                logging.warning('host of disk write has a wrong data type!')
                disk_write = 0
            if line.find('unavailable') > 0 or len(fields) < 11:
                swap_in = '0.00'
                io_percent = '0.00'
            else:
                swap_in = fields[8]
                io_percent = fields[10]

            usage_items.append((pid, disk_read, disk_write, swap_in, io_percent))

        return start_time, usage_items
