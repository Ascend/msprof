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
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.file_manager import FileOpen
from host_prof.host_mem_usage.model.host_mem_usage import HostMemUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


class HostMemUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host mem usage data
    """

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.mem_usage_info = []

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get mem timeline header
        """
        return [
            ["process_name", InfoConfReader().get_json_pid_data(), InfoConfReader().get_json_tid_data(), "Memory Usage"]
        ]

    def init(self: any) -> None:
        """
        init class
        """
        self.set_model(HostMemUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        parse prof data
        """
        try:
            with FileOpen(self.file_name, "r") as file:
                self._parse_mem_usage(file.file_reader)
                logging.info("Finish parsing host mem usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError, TypeError) as parse_file_except:
            logging.error(
                "Error in parsing host mem usage data:%s", str(parse_file_except), exc_info=Constant.TRACE_BACK_SWITCH
            )
        finally:
            pass

    def get_mem_usage_data(self: any) -> dict:
        """
        return mem usage data
        """
        with self.cur_model as model:
            if model.has_mem_usage_data():
                res = self.cur_model.get_mem_usage_data()
                return res
        return MsvpConstant.EMPTY_DICT

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        mem_usage_data = self.get_mem_usage_data()
        result = []
        if not mem_usage_data:
            logging.error("Mem usage data is empty, please check if it exists!")
            return result
        for data_item in mem_usage_data.get("data"):
            if is_number(data_item["start"]):
                temp_data = [
                    "Memory Usage",
                    data_item["start"],
                    {"Usage(%)": round(data_item["usage"], NumberConstant.ROUND_THREE_DECIMAL)},
                ]
                result.append(temp_data)
        return result

    def get_summary_data(self: any) -> tuple:
        """
        get summary data
        """
        with self.cur_model as model:
            if model.has_mem_usage_data():
                res = model.get_recommend_value('usage', DBNameConstant.TABLE_HOST_MEM_USAGE)
                mem_total = InfoConfReader().get_mem_total()
                if res and mem_total:
                    res[0] = round(res[0] * mem_total / NumberConstant.PERCENTAGE, NumberConstant.ROUND_THREE_DECIMAL)
                    res[1] = round(res[1] * mem_total / NumberConstant.PERCENTAGE, NumberConstant.ROUND_THREE_DECIMAL)
                    return [tuple([mem_total, *res])]
        return MsvpConstant.EMPTY_LIST

    def _parse_mem_data(self: any, file: any, mem_total) -> None:
        if not mem_total or not is_number(mem_total):
            logging.error("mem_total is invalid, please check file info.json...")
            return
        last_timestamp = None

        for line in file:
            usage_detail = line.split()
            # parse data from usage_detail
            if len(usage_detail) < 3:
                logging.error("parse usage detail failed, line: %s", line)
                continue
            curr_timestamp = usage_detail[0]
            # The used memory is four times the physical memory
            usage_data = Decimal(usage_detail[2]) * 4 / mem_total * NumberConstant.PERCENTAGE
            usage = str(usage_data.quantize(NumberConstant.USAGE_PLACES))

            if last_timestamp is not None:
                item = [last_timestamp, curr_timestamp, usage]
                self.mem_usage_info.append(item)

            last_timestamp = curr_timestamp

        self.cur_model.insert_mem_usage_data(self.mem_usage_info)

    def _parse_mem_usage(self: any, file: any) -> None:
        mem_total = InfoConfReader().get_mem_total()
        self._parse_mem_data(file, mem_total)
