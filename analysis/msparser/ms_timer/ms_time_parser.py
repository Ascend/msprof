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

import configparser
import logging
import os
import sqlite3

from common_func.common import check_number_valid
from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_host_start_compiles
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.utils import Utils
from msmodel.ms_timer.ms_time_model import MsTimeModel
from msparser.interface.iparser import IParser
from profiling_bean.basic_info.host_start import TimerBean


class MsTimeParser(IParser, MsMultiProcess):
    """
    parser of time data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._message_dict = {}

    @staticmethod
    def parse_host_start(config: any, index: int) -> tuple:
        """
        parse host_start.log
        :param config: config parser
        :param index: device id
        :return: host monotonic time and host wall time
        """
        if not isinstance(index, int):
            return -1, -1
        if not config.has_section(f"Device{index}"):
            return -1, -1
        times = dict(config.items(f"Device{index}"))
        timer = TimerBean(times, InfoConfReader().get_host_freq())
        return str(timer.host_wall), str(timer.host_mon)

    @staticmethod
    def parse_dev_start(dev_: any) -> tuple:
        """
        parse dev_start.log
        :param dev_: file reader
        :return: device time
        """
        dev_wall, dev_mon, dev_cntvct = 0, 0, 0
        while True:
            line = dev_.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            replay_status, timestamp = line.strip().split(":")
            if replay_status == 'clock_realtime':
                dev_wall = int(float(timestamp))
            elif replay_status == 'clock_monotonic_raw':
                dev_mon = int(float(timestamp))
            elif replay_status == 'cntvct':
                dev_cntvct = int(float(timestamp))
            elif dev_wall and dev_mon and dev_cntvct:
                break
        return dev_wall, dev_mon, dev_cntvct

    def parse(self: any) -> None:
        """
        parse job info
        :return: None
        """
        file_patterns = get_host_start_compiles()
        for file_name in os.listdir(self._project_path):
            host_start_result = get_file_name_pattern_match(file_name, *file_patterns)
            if host_start_result and is_valid_original_data(file_name, self._project_path, is_conf=True):
                self._message_dict = self.get_sync_data(file_name)
                FileManager.add_complete_file(self._project_path, file_name)

    def get_sync_data(self: any, file_name: str) -> dict:
        """
        path:/FILE_PATH/analysis/analysis_num
        get time sync data from dev_start.log and host_start.log
        :param file_name: file name
        :return: sync time dict
        """
        try:
            if not os.path.exists(os.path.join(self._project_path, file_name)):
                logging.error("No data file was generated.")
                raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
        except (OSError, ProfException) as err:
            logging.error("Error in parsing sync data:%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return {}
        logging.info("Start parsing sync data ...")
        # initialize time synchronize messages
        message = self.__parse_time_data()
        logging.info("Parsing sync data finished...")
        return message

    def save(self: any) -> None:
        try:
            if self._check_time_format(self._message_dict):
                message_list = self._pre_time_data(self._message_dict)
                ms_time_model = MsTimeModel(self._project_path)
                if ms_time_model.init():
                    ms_time_model.flush(message_list)
                    ms_time_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save time failed, %s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def ms_run(self: any) -> None:
        """
        entrance of time parser
        :return: None
        """
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        self.save()

    def _check_time_format(self, message: dict) -> bool:
        result = False
        if not message:
            return result
        for items in message["data"]:
            check_int_item = [
                items["dev_mon"],
                items["dev_wall"],
                items["dev_cntvct"],
                items["host_mon"],
                items["host_wall"],
            ]
            if any(not check_number_valid(item) for item in check_int_item):
                logging.error("time_sync message check integer value failed")
                return result
        return True

    def _pre_time_data(self, message: dict) -> list:
        message_list = Utils.generator_to_list(
            [
                pb_time_mesg["device_id"],
                pb_time_mesg["dev_mon"],
                pb_time_mesg["dev_wall"],
                pb_time_mesg["dev_cntvct"],
                pb_time_mesg["host_mon"],
                pb_time_mesg["host_wall"],
            ]
            for pb_time_mesg in message["data"]
        )
        return message_list

    def __parse_time_data_helper(self: any, message: dict, index: int) -> None:
        host_start_path = os.path.join(self._project_path, "host_start.log.{}".format(index))
        dev_start_path = os.path.join(self._project_path, "dev_start.log.{}".format(index))
        if os.path.exists(host_start_path) and os.path.exists(dev_start_path):
            check_path_valid(host_start_path, True)
            config = configparser.ConfigParser()
            config.read(host_start_path)
            host_wall, host_mon = self.parse_host_start(config, index)
            with FileOpen(dev_start_path, "r") as dev_:
                dev_wall, dev_mon, dev_cntvct = self.parse_dev_start(dev_.file_reader)

            data = {
                "device_id": index,
                "dev_mon": int(dev_mon),
                "dev_wall": int(dev_wall),
                "dev_cntvct": int(dev_cntvct),
                "host_mon": int(host_mon),
                "host_wall": int(host_wall),
            }
            message["data"].append(data)

    def __parse_time_data(self: any) -> dict:
        """
        path:/FILE_PATH/analysis/
        call parse_host_start to parse host_start.log
        call parse_dev_start to parse dev_start.log
        """
        message = {"data": []}
        try:
            for index in range(8):
                self.__parse_time_data_helper(message, index)
            return message
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error('Parse time sync data error: %s', str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return message
