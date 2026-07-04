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

import configparser
import decimal
import json
import logging
import os
import re
from decimal import Decimal
from typing import Dict

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_dev_start_compiles
from common_func.file_name_manager import get_end_info_compiles
from common_func.file_name_manager import get_host_start_compiles
from common_func.file_name_manager import get_info_json_compiles
from common_func.file_name_manager import get_sample_json_compiles
from common_func.file_name_manager import get_start_info_compiles
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_number
from common_func.msvp_common import is_valid_original_data
from common_func.singleton import singleton
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from profiling_bean.basic_info.host_start import TimerBean


@singleton
class InfoConfReader:
    """
    class used to read data from info.json
    """

    INFO_PATTERN = r"^info\.json\.?(\d?)$"
    FREQ = "38.4"
    NPU_PROFILING_TYPE = "npu_profiling"
    HOST_PROFILING_TYPE = "host_profiling"
    HOST_DEFAULT_FREQ = NumberConstant.NANO_SECOND
    ANALYSIS_VERSION = "1.0"
    ALL_EXPORT_VERSION = 0x072211  # 467473

    def __init__(self: any) -> None:
        self._info_json = None
        self._start_info = None
        self._end_info = None
        self._sample_json = None
        self._start_log_time = 0
        self._host_host_mon = 0
        self._host_host_cnt = 0
        self._host_mon = 0
        self._host_cnt = 0
        self._host_freq = None
        self._dev_cnt = 0
        self._host_local_time_offset = 0
        self._local_time_offset = 0

    @staticmethod
    def trans_syscnt_into_local_time(raw_timestamp: int) -> str:
        time_stamp = InfoConfReader().time_from_syscnt(raw_timestamp, NumberConstant.MICRO_SECOND)
        local_time = InfoConfReader().trans_into_local_time(raw_timestamp=time_stamp, use_us=True)
        return local_time

    @staticmethod
    def __get_json_data(info_json_path: str) -> dict:
        """
        read json data from file
        :param info_json_path:info json path
        :return:
        """
        if not info_json_path or not os.path.exists(info_json_path) or not os.path.isfile(info_json_path):
            return {}
        try:
            with FileOpen(info_json_path, "r") as json_reader:
                json_data = json_reader.file_reader.readline(Constant.MAX_READ_LINE_BYTES)
                json_data = json.loads(json_data)
                return json_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            logging.error("json data decode fail")
            return {}

    @classmethod
    def get_json_tid_data(cls: any) -> int:
        """
        get timeline json tid data
        """
        return TraceViewHeaderConstant.DEFAULT_TID_VALUE

    @classmethod
    def get_conf_file_path(cls: any, project_path: str, conf_patterns: tuple) -> str:
        """
        get the config file path with pattern
        """
        for _file in os.listdir(project_path):
            for conf_pattern in conf_patterns:
                if conf_pattern.match(_file) and is_valid_original_data(_file, project_path, is_conf=True):
                    return os.path.join(project_path, _file)
        return ""

    @classmethod
    def _get_instr_profiling_frequency_from_sample(cls, sample_json: Dict) -> int:
        instr_profiling_freq0 = sample_json.get("instr_profiling_freq")
        instr_profiling_freq1 = sample_json.get("instrProfilingFreq")
        if instr_profiling_freq0 is None and instr_profiling_freq1 is None:
            logging.error("instr profiling frequency not found in sample.json", exc_info=Constant.TRACE_BACK_SWITCH)
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        instr_profiling_freq_val = instr_profiling_freq0 if instr_profiling_freq1 is None else instr_profiling_freq1
        instr_profiling_freq = int(instr_profiling_freq_val)

        if instr_profiling_freq <= 0:
            logging.error("Instr Profiling Frequency is invalid.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        return instr_profiling_freq

    def load_info(self: any, result_path: str) -> None:
        """
        load all info
        """
        self._load_json(result_path)
        self._load_dev_start_time(result_path)
        self._load_host_start_time(result_path)
        self._load_local_time_offset()

    def get_start_timestamp(self: any) -> int:
        """
        get start time
        """
        return self._start_log_time

    def get_root_data(self: any, datatype: any) -> str:
        """
        write data into info.json
        :param datatype: desired data type
        :return:
        """
        return self._info_json.get(datatype)

    def get_device_list(self: any) -> list:
        """
        get device list from project path
        isdigit()不能确保所有的字符都能被正确识别，Unicode上标字符'²'等也能通过校验，但无法被转为int
        :return: devices list in the format: "0,1,2,3"
        """
        devices = list(filter(None, self._sample_json.get("devices", "").strip().split(",")))
        if not devices:
            logging.error("Can't find correct devices in sample.json")
            raise ProfException(ProfException.PROF_INVALID_CONFIG_ERROR)
        for device_id in devices:
            try:
                int(device_id)
            except ValueError as err:
                logging.error("device id in sample.json is invalid, device id is: %s", device_id)
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR) from err
        return devices

    def get_rank_id(self: any) -> int:
        """
        get rank_id
        :return: rank_id
        """
        rank_id = self._info_json.get("rank_id", Constant.DEFAULT_INVALID_VALUE)
        return rank_id

    def is_version_matched(self):
        """
        check the version between data-collection and the data-analysis
        """
        return self.get_collection_version() == self.ANALYSIS_VERSION

    def get_collection_version(self):
        return self._info_json.get("version", Constant.NA)

    def is_all_export_version(self):
        """
        check the version whether support all data export
        """
        return self._info_json is not None and self.get_drv_version() >= self.ALL_EXPORT_VERSION

    def get_drv_version(self):
        return self._info_json.get("drvVersion", 0)

    def get_device_id(self: any) -> str:
        """
        get device_id
        :return: device id
        """
        device_id = self._info_json.get("devices", Constant.NA)
        if device_id and not device_id.isdigit():
            logging.error("Device id : %s is not a digit!", device_id)
            return Constant.NA
        return device_id

    def get_job_info(self: any) -> str:
        """
        get job info message
        """
        return self._info_json.get("jobInfo", Constant.NA)

    def get_freq(self: any, search_type: any) -> float:
        """
        get HWTS frequency from info.json
        :param search_type: search type aic|hwts
        :return:desired frequency
        """
        freq = str(self.get_data_under_device("{}_frequency".format(search_type)))
        try:
            if not freq or float(freq) <= 0:
                logging.error("unable to get %s frequency.", search_type)
                raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        except (ValueError, TypeError) as err:
            logging.error(err)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT) from err
        return float(freq) * NumberConstant.FREQ_TO_MHz

    def get_collect_time(self: any) -> tuple:
        """
        Compatibility for getting collection time
        """
        return self._start_info.get(StrConstant.COLLECT_TIME_BEGIN), self._end_info.get(StrConstant.COLLECT_TIME_END)

    def get_collect_start_time(self: any) -> str:
        """
        Compatibility for getting collection start time
        """
        collect_time = self._start_info.get(StrConstant.COLLECT_DATE_BEGIN, Constant.NA)
        if not collect_time:
            return Constant.NA
        return collect_time

    def get_collect_raw_time(self: any) -> tuple:
        """
        Compatibility for getting collection raw time
        """
        return self._start_info.get(StrConstant.COLLECT_RAW_TIME_BEGIN), self._end_info.get(
            StrConstant.COLLECT_RAW_TIME_END
        )

    def get_collect_date(self: any) -> tuple:
        """
        Compatibility for getting collection date
        """
        return self._start_info.get(StrConstant.COLLECT_DATE_BEGIN), self._end_info.get(StrConstant.COLLECT_DATE_END)

    def get_dev_cnt(self: any) -> int:
        """
        Compatibility for getting dev_cnt (The dev_cnt is obtained from the cntvct in dev_start.log file.)
        """
        return int(self._dev_cnt * NumberConstant.NANO_SECOND)

    def time_from_syscnt(self: any, sys_cnt: int, time_fmt: int = NumberConstant.NANO_SECOND) -> float:
        """
        transfer sys cnt to time unit.
        :param sys_cnt: sys cnt
        :param time_fmt: time format
        :return: sys time
        """
        hwts_freq = self.get_freq(StrConstant.HWTS)
        return (
            float(sys_cnt - self._dev_cnt * NumberConstant.NANO_SECOND) / hwts_freq * time_fmt
            + self._host_mon * time_fmt
        )

    def duration_from_syscnt(self: any, delta_syscnt: int, time_fmt: int = NumberConstant.MICRO_SECOND) -> float:
        hwts_freq = self.get_freq(StrConstant.HWTS)
        return float(delta_syscnt) / hwts_freq * time_fmt

    def time_from_host_syscnt(
        self: any, sys_cnt: int, time_fmt: int = NumberConstant.NANO_SECOND, is_host: bool = True
    ) -> float:
        """
        transfer sys cnt to host_time unit.
        1.task_duration_sys_count: data_sys_count - start_sys_count
        2.task_duration_timestamp: task_duration_sys_count / freq
        3.data_timestamp(host): task_duration_timestamp + start_timestamp(host)
        :param sys_cnt: host sys count
        :param time_fmt: time format
        :param is_host: use host's host_monotonic or dev's host_monotonic
        :return: sys timestamp
        """
        host_freq = self.get_host_freq()
        if host_freq != self.HOST_DEFAULT_FREQ:
            host_mon = self._host_host_mon if is_host else self._host_mon
            host_cnt = self._host_host_cnt if is_host else self._host_cnt
            time = float(sys_cnt - host_cnt * NumberConstant.NANO_SECOND) / host_freq * time_fmt + host_mon * time_fmt
            return time if time >= 0.0 else 0
        return sys_cnt * time_fmt / NumberConstant.NANO_SECOND

    def get_host_duration(self: any, host_syscnt_duration: int, time_fmt: int = NumberConstant.NANO_SECOND) -> float:
        """
        transfer sys cnt duration to time duration.
        :param host_syscnt_duration: host sys counts duration
        :param time_fmt: time format
        :return: sys time duration
        """
        host_freq = self.get_host_freq()
        if host_freq != self.HOST_DEFAULT_FREQ:
            return host_syscnt_duration / host_freq * time_fmt
        return host_syscnt_duration * time_fmt / NumberConstant.NANO_SECOND

    def get_host_syscnt_from_dev_time(self: any, dev_timestamp: float) -> float:
        """
        transfer dev timestamp to host sys count, Inverse operation of time_from_syscnt()
        :param dev_timestamp: device timestamp
        :return: host sys count
        """
        host_freq = self.get_host_freq()
        if host_freq != self.HOST_DEFAULT_FREQ:
            return (
                dev_timestamp - self._host_mon * NumberConstant.NANO_SECOND
            ) / NumberConstant.NANO_SECOND * host_freq + self._host_cnt * NumberConstant.NANO_SECOND
        return dev_timestamp

    def get_json_pid_data(self: any) -> int:
        """
        get pid message
        """
        process_id = self._info_json.get("pid")
        return int(process_id) if is_number(process_id) else TraceViewHeaderConstant.DEFAULT_PID_VALUE

    def get_json_pid_name(self: any) -> str:
        """
        get profiling pid name
        """
        return self._info_json.get("pid_name")

    def get_cpu_info(self: any) -> list:
        """
        get cpu info
        """
        return [self._info_json.get(StrConstant.CPU_NUMS), self._info_json.get(StrConstant.SYS_CLOCK_FREQ)]

    def get_net_info(self: any) -> tuple:
        """
        get net info
        """
        return self._info_json.get(StrConstant.NET_CARD_NUMS), self._info_json.get(StrConstant.NET_CARD)

    def get_mem_total(self: any) -> int:
        """
        get net info
        """
        return self._info_json.get(StrConstant.MEMORY_TOTAL)

    def get_info_json(self: any) -> dict:
        """
        get info json
        """
        return self._info_json

    def is_host_profiling(self: any) -> bool:
        """
        get profiling type by device info if exist
        """
        device_info = self._info_json.get("DeviceInfo")
        return not device_info

    def get_data_under_device(self: any, data_type: any) -> str:
        """
        get ai core number
        :param data_type: desired data type
        :return:
        """
        device_items = self._info_json.get("DeviceInfo")
        if device_items is None:
            logging.error("unable to get DeviceInfo from info.json")
            return ""

        if isinstance(device_items, list) and device_items and device_items[0]:
            return device_items[0].get(data_type, "")
        return ""

    def get_delta_time(self: any) -> float:
        """
        calculate time difference between host and device
        """
        return self._host_mon - self._start_log_time / NumberConstant.NANO_SECOND

    def get_instr_profiling_freq(self: any) -> int:
        """
        get instr_profiling_freq from info json
        """
        return self._get_instr_profiling_frequency_from_sample(self._sample_json)

    def get_job_basic_info(self: any) -> list:
        job_info = self.get_job_info()
        device_id = self.get_device_id()
        rank_id = self.get_rank_id()
        collection_time, _ = InfoConfReader().get_collect_date()
        if not collection_time:
            collection_time = Constant.NA
        return [job_info, device_id, collection_time, rank_id]

    def get_host_freq(self: any) -> float:
        if self._host_freq is not None:
            return self._host_freq
        host_cpu_info = self._info_json.get('CPU', [])
        if host_cpu_info and isinstance(host_cpu_info, list) and host_cpu_info[0]:
            freq = host_cpu_info[0].get('Frequency')
            if is_number(freq) and float(freq) > 0.0:
                self._host_freq = float(freq) * NumberConstant.FREQ_TO_MHz
            else:
                logging.info("No host frequency, or the frequency is invalid.")
                self._host_freq = self.HOST_DEFAULT_FREQ
            return self._host_freq
        logging.error("No host info json.")
        raise ProfException(ProfException.PROF_NONE_ERROR)

    def get_host_time_by_sampling_timestamp(self: any, timestamp: any) -> int:
        """
        Obtain the actual time based on the sampling timestamp (us).
        :return: int
        """
        return (
            int(
                timestamp * NumberConstant.USTONS
                + self.get_start_timestamp()
                + self.get_delta_time() * NumberConstant.NANO_SECOND
            )
            / NumberConstant.NS_TO_US
        )

    def get_absolute_time_by_sampling_timestamp(self: any, timestamp: any) -> int:
        """
        Obtain the absolute timestamp(ns) based on the sampling timestamp(us).
        """
        return int(
            timestamp * NumberConstant.USTONS
            + self.get_start_timestamp()
            + self.get_delta_time() * NumberConstant.NANO_SECOND
            + self.get_local_time_offset() * NumberConstant.NS_TO_US
        )

    def format_absolute_ns_to_us(self: any, timestamp: any) -> str:
        """
        Format an absolute timestamp(ns) to the trace timestamp unit(us).
        """
        return str((Decimal(str(timestamp)) / Decimal(NumberConstant.NS_TO_US)).quantize(Decimal('0.000')))

    def get_ai_core_profiling_mode(self):
        return self._sample_json.get("ai_core_profiling_mode")

    def trans_into_local_time(self: any, raw_timestamp: float, use_us: bool = False, is_host: bool = False) -> str:
        """
        transfer raw time(ns or us) into local time
        return: local time(str)
        """
        if use_us:
            res = Decimal(str(raw_timestamp)) + Decimal(str(self.get_local_time_offset(is_host)))
        else:
            res = Decimal(str(raw_timestamp)) / Decimal(NumberConstant.USTONS) + Decimal(
                str(self.get_local_time_offset(is_host))
            )
        res = res.quantize(decimal.Decimal('0.000'))
        return str(res)

    def trans_from_local_time_into_dev_raw_time(self: any, local_time: int, is_host: bool = False) -> int:
        """
        transfer local time into raw time(ns)
        return: raw_timestamp(int)
        """
        res = (Decimal(local_time) - Decimal(str(self.get_local_time_offset(is_host)))) * Decimal(
            NumberConstant.NS_TO_US
        )
        return int(res)

    def trans_from_start_info_raw_time_into_host_cnt(self: any) -> int:
        """
        transfer raw time(ns) from start info json to host cnt
        return: host cnt
        """
        start_info_raw_time, _ = self.get_collect_raw_time()
        res = (
            (Decimal(int(start_info_raw_time)) - Decimal(self._host_mon * NumberConstant.NANO_SECOND))
            * Decimal(self.get_host_freq() / NumberConstant.NANO_SECOND)
        ) + Decimal(self._host_cnt * NumberConstant.NANO_SECOND)
        return int(res)

    def get_local_time_offset(self: any, is_host: bool = False) -> float:
        """
        get the offset between local time and monotonic raw
        add the offset to monotonic raw to get the local time
        return: offset(us)
        """
        return self._host_local_time_offset if is_host else self._local_time_offset

    def get_qos_events(self: any) -> str:
        """
        get qosEvents from sample.json
        """
        return str(self._sample_json.get("qosEvents", ""))

    def is_level0(self) -> bool:
        """
        check prof level0
        """
        prof_level = self._get_prof_level()
        return prof_level in (StrConstant.PROF_LEVEL_0, StrConstant.PROF_LEVEL_0_HISI)

    def get_cann_version(self) -> tuple:
        """
        get cann version info(format: 9.0.0 or 9.0.0-beta.0 or 9.0.T106) from info.json
        return: major, minor, micro
        """
        cann_version = self._info_json.get("cannVersion", "")
        # Extract major and minor version numbers
        version_match = re.match(r"^(\d+)\.(\d+)", cann_version)
        if version_match:
            major, minor = version_match.groups()
            return int(major), int(minor)
        logging.warning("Cann version format is invalid or missing in info.json, version string: %s", cann_version)
        return 0, 0

    def _get_prof_level(self: any) -> str:
        """
        get prof level from sample.json
        """
        prof_level = self._sample_json.get("profLevel")
        return prof_level if prof_level else self._sample_json.get("prof_level")  # hisi key name

    def _load_json(self: any, result_path: str) -> None:
        """
        load info.json once
        """
        self._info_json = self.__get_json_data(self.get_conf_file_path(result_path, get_info_json_compiles()))
        self._sample_json = self.__get_json_data(self.get_conf_file_path(result_path, get_sample_json_compiles()))
        self._start_info = self.__get_json_data(self.get_conf_file_path(result_path, get_start_info_compiles()))
        self._end_info = self.__get_json_data(self.get_conf_file_path(result_path, get_end_info_compiles()))

    def _load_dev_start_path_line_by_line(self: any, log_file: any) -> None:
        self._dev_cnt = 0
        self._start_log_time = 0
        while True:
            line = log_file.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            split_str = line.strip().split(":")
            if len(split_str) != 2 or not is_number(split_str[1]):
                continue
            if split_str[0] == StrConstant.MONOTONIC_TIME:
                self._start_log_time = int(split_str[1])
            elif split_str[0] == StrConstant.DEVICE_SYSCNT:
                self._dev_cnt = float(split_str[1]) / NumberConstant.NANO_SECOND
            elif self._start_log_time and self._dev_cnt:
                break

    def _load_dev_start_time(self: any, result_path: str) -> None:
        """
        load start log
        """
        dev_start_path = self.get_conf_file_path(result_path, get_dev_start_compiles())
        if not os.path.exists(dev_start_path):
            return
        try:
            with FileOpen(dev_start_path, "r") as log_file:
                self._load_dev_start_path_line_by_line(log_file.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _check_monotonic_and_cnt(self, host_start_file: str) -> bool:
        if host_start_file == '' and self.is_host_profiling():
            self._host_freq = self.HOST_DEFAULT_FREQ
            return False
        return self._host_mon <= 0 or (self._dev_cnt <= 0 and self._host_cnt <= 0)

    def _load_host_start_time(self: any, project_path: str) -> None:
        """
        load host start time
        :return: None
        """
        host_start_file = self.get_conf_file_path(project_path, get_host_start_compiles())
        try:
            if os.path.exists(host_start_file):
                check_path_valid(host_start_file, True)
                config = configparser.ConfigParser()
                config.read(host_start_file)
                sections = config.sections()
                if not sections:
                    return
                time = dict(config.items(sections[0]))
                timer = TimerBean(time, self.get_host_freq())
                self._host_mon = float(timer.host_mon) / NumberConstant.NANO_SECOND
                self._host_cnt = float(timer.cntvct) / NumberConstant.NANO_SECOND
                if self.is_host_profiling():
                    self._host_host_mon = self._host_mon
                    self._host_host_cnt = self._host_cnt
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error('Parse time sync data error: %s', str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        if self._check_monotonic_and_cnt(host_start_file):
            logging.error(
                "The monotonic time %s, dev_cntvct %s or host_cntvct %s is unusual, maybe get data from driver failed",
                self._host_mon,
                self._dev_cnt,
                self._host_cnt,
            )

    def _load_local_time_offset(self: any) -> None:
        """
        load local time offset(us) from start info
        :return: None
        """
        collect_time_begin, _ = self.get_collect_time()
        collect_raw_time, _ = self.get_collect_raw_time()
        if collect_time_begin and collect_raw_time:
            self._local_time_offset = float(collect_time_begin) - float(collect_raw_time) / NumberConstant.NS_TO_US
            if self.is_host_profiling():
                self._host_local_time_offset = self._local_time_offset
            return
        logging.error("No start info, or start info is invalid.")
        raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
