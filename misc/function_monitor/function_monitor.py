# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

import importlib
import logging
import functools
import os
import sys
import time
import threading
import torch
from collections import defaultdict
from pathlib import Path
from file_manager import FileManager


ENABLE_LIBKPERF = os.getenv('ENABLE_LIBKPERF', 'False').lower() == 'true'
ENABLE_FUNCTION_MONITOR = os.getenv('ENABLE_FUNCTION_MONITOR', 'False').lower() == 'true'


class MonitorLogger:
    PID = None
    LOGGER = None
    LOG_PATH = None

    @classmethod
    def get_logger(cls) -> logging.Logger:
        pid = os.getpid()
        if cls.LOGGER is None or cls.PID != pid:
            cls.PID = pid
            cls.LOGGER = logging.getLogger(f"{__name__}_{pid}")
            cls.LOGGER.setLevel(logging.INFO)
            cls.LOGGER.propagate = False
            log_file = cls.create_log_file()
            if log_file is not None:
                file_handler = logging.FileHandler(log_file)
                file_handler.setLevel(logging.INFO)
                formatter = logging.Formatter("[%(asctime)s][%(levelname)s][%(filename)s:%(lineno)d] %(message)s")
                file_handler.setFormatter(formatter)
                cls.LOGGER.addHandler(file_handler)
            else:
                cls.LOGGER.addHandler(logging.StreamHandler(sys.stdout))
        return cls.LOGGER

    @classmethod
    def get_log_path(cls) -> str:
        if cls.LOG_PATH is None:
            log_path = os.getenv('FUNCTION_MONITOR_LOG_PATH', None)
            default_log_path = os.path.join(str(Path.home().absolute()), 'function_monitor_log')
            if not log_path or not isinstance(log_path, str):
                log_path = default_log_path
            else:
                try:
                    FileManager.make_dir_safety(log_path)
                    FileManager.check_path_writeable(log_path)
                except Exception as e:
                    logging.error(
                        "Log path writeable check failed: %s, error: %s, reset to default path: %s",
                        log_path,
                        e,
                        default_log_path,
                    )
                    log_path = default_log_path
            cls.LOG_PATH = log_path
        return cls.LOG_PATH

    @classmethod
    def create_log_file(cls):
        log_path = cls.get_log_path()
        log_file = os.path.join(log_path, f"{__name__}_{cls.PID}.log")
        try:
            FileManager.create_file_by_path(log_file)
        except Exception as err:
            print(f"Error: {err}")
            return None
        return log_file


def get_current_time_us() -> float:
    return time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW) / 1000.0


def get_rank_id() -> int:
    rank_id = os.getenv('RANK', None) or os.getenv('RANK_ID', None)
    if rank_id is None and torch.distributed.is_available() and torch.distributed.is_initialized():
        rank_id = torch.distributed.get_rank()
    try:
        rank_id = int(rank_id) if rank_id is not None else -1
    except (TypeError, ValueError):
        rank_id = -1
    return rank_id


class PerformanceMonitor:
    THREAD_MAP = defaultdict(int)

    def __init__(self, evt_list=None, pid_list=None, cpu_list=None):
        self.evt_list = []
        self.pid_list = pid_list or [0]
        self.cpu_list = cpu_list or [-1]
        self.current_tid = threading.get_native_id()
        self.logger = MonitorLogger.get_logger()
        self.monitor_enabled = True

        try:
            self._kperf = importlib.import_module('kperf')
            # set default perf event list
            self.evt_list = evt_list or ['cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults']
        except Exception as e:
            self.logger.error(f"Failed to import kperf module: {e}")
            self.monitor_enabled = False

    def start(self) -> None:
        try:
            if not self.monitor_enabled:
                return
            if self.current_tid not in self.THREAD_MAP:
                pmu_attr = self._kperf.PmuAttr(evtList=self.evt_list, pidList=self.pid_list, cpuList=self.cpu_list)
                pd = self._kperf.open(self._kperf.PmuTaskType.COUNTING, pmu_attr)

                if pd in (-1, None):
                    self.logger.error(f"Failed to open kperf PMU for TID {self.current_tid}: {self._kperf.error()}")
                    self.monitor_enabled = False
                    return

                self.THREAD_MAP[self.current_tid] = pd
            else:
                pd = self.THREAD_MAP[self.current_tid]

            if self.monitor_enabled:
                self._kperf.enable(pd)
                self._kperf.read(pd)

        except Exception as e:
            self.logger.error(f"Failed to enable kperf PMU for TID {self.current_tid}: {e}")
            self.monitor_enabled = False

    def stop_and_get_perf_info(self) -> str:
        perf_info = ''
        if self.monitor_enabled:
            event_counts = defaultdict(int)
            if self.current_tid in self.THREAD_MAP:
                try:
                    pd = self.THREAD_MAP[self.current_tid]
                    data_iter = self._kperf.read(pd)
                    for data in data_iter:
                        event_counts[data.evt] += data.count
                    self._kperf.disable(pd)
                except Exception as e:
                    self.logger.error(f"Failed to read kperf PMU data for PID {self.current_tid}: {e}")
                    return perf_info
            perf_info = ';'.join(f"event-{evt}:{count}" for evt, count in event_counts.items())
        return perf_info


perf_monitor = PerformanceMonitor()


class FunctionMonitorContext:
    THRESHOLD_MS = 1
    US_TO_MS = 1000.0

    def __init__(self, func_name, process_name='', threshold_ms=THRESHOLD_MS):
        self.logger = MonitorLogger.get_logger()

        if not isinstance(func_name, str):
            self.logger.warning(f"func_name must be a string, but got {func_name}, reset to ''")
            func_name = ''
        if not isinstance(process_name, str):
            self.logger.warning(f"process_name must be a string, but got {process_name}, reset to ''")
            process_name = ''
        if not isinstance(threshold_ms, (int, float)):
            self.logger.warning(f"threshold_ms must be a number, but got {threshold_ms}, reset to 1")
            threshold_ms = 1

        self.func_name = '_'.join(func_name.split())
        self.process_name = process_name or torch.multiprocessing._get_thread_name()
        self.rank_id = get_rank_id()
        self.threshold_ms = threshold_ms or self.THRESHOLD_MS
        self.start_time = 0

    def __enter__(self):
        if not ENABLE_FUNCTION_MONITOR:
            return self
        self.start_time = get_current_time_us()
        if ENABLE_LIBKPERF:
            perf_monitor.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if not ENABLE_FUNCTION_MONITOR:
            return
        end_time = get_current_time_us()
        duration_us = end_time - self.start_time
        if duration_us / self.US_TO_MS >= self.threshold_ms:
            perf_info = perf_monitor.stop_and_get_perf_info() if ENABLE_LIBKPERF else ''
            self.logger.info(
                f"name:{self.func_name} start:{self.start_time} duration:{duration_us} "
                f"rankId:{self.rank_id} pid:{os.getpid()} pname:{self.process_name} "
                f"tid:{threading.get_native_id()} tname:{threading.current_thread().name} extraInfo:{perf_info}"
            )


def function_monitor(func_name='', process_name='', threshold_ms=1):
    def decorator(func):
        name = func.__name__ if not func_name and callable(func) else func_name

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            with FunctionMonitorContext(name, process_name, threshold_ms):
                return func(*args, **kwargs)

        return wrapper

    return decorator
