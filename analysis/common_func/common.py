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

import ctypes
import logging
import os
import platform
import re
import sys
import time
from logging.handlers import RotatingFileHandler

from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.regex_manager import RegexManagerConstant


class CommonConstant:
    """
    common constant class
    """

    CLIENT_NUM = 16
    SAMPLE_JSON = "sample.json"
    INFO_JSON_PATTERN = re.compile(r"info.json.(\d+)$")
    GE_TABLE_NUM = 2
    ACL_TABLE_MAP = "AclDataMap"
    ACL_HASH_TABLE_MAP = "AclHashMap"

    CONN_MASK = 0o640
    FILE_AUTHORITY = 0o640
    OPEN_AUTHORITY = 0o640
    JOB_ID_PATTERN = r"^[A-Za-z0-9\-\_]+$"
    BYTE_SIZE = 1024
    # Disk available size 100MB and max zip file size is set to 100MB to avoid zip bomb
    DISK_SIZE = 100
    PERCENT = 100.0
    ROUND_SIX = 6
    # Disk available percentage
    MAX_LOG_BYTES = 5 * 1024 * 1024
    MAX_LOG_BACKUPS = 1
    FILE_NAME = os.path.basename(__file__)
    FILE_NAME_DONE_LEN = 5  # filename example: Framework.host.task_desc_info.0.slice_0.done
    LOG_LEVEL = logging.INFO
    MINIMUM_DISK_MEMORY = 512
    MEMORY_BUFFER_NUM = 3

    def get_common_class_name(self: any) -> any:
        """
        get common class name
        """
        return self.__class__.__name__

    def get_common_class_member(self: any) -> any:
        """
        get common class member num
        """
        return self.__dict__


def error(file_name: str, msg: str) -> None:
    """
    print error message
    """
    if file_name is None or msg is None:
        return
    print_msg(
        time.strftime("%a %d %b %Y %H:%M:%S ", time.localtime())
        + f"[ERROR] [MSVP] [{str(os.getpid())}] {file_name}: {msg}",
        flush=True,
    )


def print_info(file_name: str, msg: str) -> None:
    """
    print info message
    """
    if file_name is None or msg is None:
        return
    print_msg(
        time.strftime("%a %d %b %Y %H:%M:%S ", time.localtime())
        + "[INFO] [MSVP] [{0}] {1}: {2}".format(str(os.getpid()), file_name, str(msg)),
        flush=True,
    )


def warn(file_name: str, msg: str) -> None:
    """
    print warning message
    """
    if file_name is None or msg is None:
        return
    print_msg(
        time.strftime("%a %d %b %Y %H:%M:%S ", time.localtime())
        + "[WARNING] [MSVP] [{0}] {1}: {2}".format(str(os.getpid()), file_name, str(msg)),
        flush=True,
    )


def print_msg(*args: any, **kwargs: any) -> None:
    """
    execute print
    """
    print(*args, **kwargs)


class Log:
    """
    Python logger
    """

    def __init__(self: any, log_path: str, logger: any = None) -> None:
        """Init a python logger"""
        if logger:
            self.logger = logging.getLogger(logger)
        else:
            self.logger = logging.getLogger()
        self.logger.setLevel(CommonConstant.LOG_LEVEL)
        self.log_path = PathManager.get_log_dir(log_path)
        if not os.path.exists(self.log_path):
            os.makedirs(self.log_path, mode=NumberConstant.DIR_AUTHORITY)
        self.log_name = os.path.join(self.log_path, "collection.log")
        for log in self.logger.handlers:
            self.logger.removeHandler(log)
        file_handler = RotatingFileHandler(
            self.log_name, 'a', CommonConstant.MAX_LOG_BYTES, CommonConstant.MAX_LOG_BACKUPS
        )
        os.chmod(self.log_name, CommonConstant.OPEN_AUTHORITY)
        file_handler.setLevel(logging.INFO)

        # Handler format.
        formatter = logging.Formatter(
            '[%(asctime)s]  [%(levelname)s] [MSVP] [%(process)d] [%(filename)s:%(lineno)d] %(message)s',
            '%a, %d %b %Y %H:%M:%S',
        )
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        file_handler.close()

    def getlog(self: any) -> any:
        """
        :return: logger object
        """
        return self.logger

    def get_log_path(self: any) -> str:
        """
        :return: logger path
        """
        return self.log_path


class LogFactory:
    """
    Build a logger
    """

    loggers = {}

    @staticmethod
    def get_logger(name: str, collection_path: str) -> any:
        """
        return name of the log
        """
        if not LogFactory.loggers.get(name):
            _logger = Log(collection_path)
            LogFactory.loggers[name] = _logger.getlog()
        return LogFactory.loggers.get(name)

    @staticmethod
    def get_unique_logger(name: str) -> any:
        """
        set logger object
        """
        if not LogFactory.loggers.get(name):
            _logger = Log(name, name)
            LogFactory.loggers[name] = _logger.getlog()
        return LogFactory.loggers.get(name)


def is_linux() -> bool:
    """
    check whether our system is linux or not
    """
    result = False
    plat, _ = get_platform_info()
    if plat.lower() == "linux":
        result = True
    return result


def get_platform_info() -> tuple:
    """
    get platform info
    """
    info = platform.uname()
    return info[0], info[4]


def get_data_dir_sorted_files(data_path: str) -> list:
    """
    sorted the files under the data directory
    """
    slice_regx = re.compile(RegexManagerConstant.REGEX_SLICE)
    num_regx = re.compile(RegexManagerConstant.REGEX_NUM)
    file_all = os.listdir(data_path)

    # find the file name with slice_num, and sorted by num.

    def _file_filer(file: any) -> any:
        if not file:
            return 0
        slice_res = slice_regx.search(file)
        if not slice_res:
            return 0
        num_res = num_regx.search(slice_res.group())
        if not num_res:
            return 0
        # This is to distinguish file types and sort slices,
        # take the first character of the file name to ascall * weight(10000) and + slice number
        return ord(file[:1]) * 10000 + int(num_res.group())

    file_all.sort(key=_file_filer)
    return file_all


def _strip_and_split_str_with_char(item: any, ch: str) -> list:
    return str(item).strip().split(ch, maxsplit=1)[0].split('.')


def check_number_valid(item: any) -> bool:
    """
    check validity of integer or float input
    """
    # check int and check float
    if not item and item != 0:
        logging.error("item is empty")
        return False
    danger_list = ["rm", "reboot", "chown", "chmod", "shutdown", "halt", ">", "<"]
    sum_split = sum(n.isdigit() for n in _strip_and_split_str_with_char(item, "e")) != 2
    sum_danger_list = str(item).strip().rsplit("e", maxsplit=1)[-1] in danger_list
    if not str(item).isdigit() and (sum_split or sum_danger_list):
        logging.error("invalid item %s", item)
        return False
    if float(item) < 0:
        logging.error("item %s should be bigger than 0", item)
        return False
    return True


def byte_per_us2_mb_pers(byte_perus: any) -> any:
    """
    transform byte/us to mb/s
    """
    return round(byte_perus * Constant.BYTE_US_TO_MB_S, NumberConstant.ROUND_THREE_DECIMAL)


def ns2_us(ns: any) -> any:
    """
    transform ns to us
    """
    return round(ns / NumberConstant.NS_TO_US, NumberConstant.ROUND_THREE_DECIMAL)


def init_log(output_path: str) -> None:
    """
    create data and corresponding directories
    """
    log_path = PathManager.get_collection_log_path(output_path)
    root_logger = logging.getLogger()
    for log in root_logger.handlers:
        root_logger.removeHandler(log)
    logging.basicConfig(
        level=CommonConstant.LOG_LEVEL,
        format='%(asctime)s [%(levelname)s] [%(filename)s:%(lineno)d] - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S',
        filename=log_path,
        filemode='a+',
    )


def get_data_file_memory(result_dir: str) -> float:
    """
    to get data memory
    :param result_dir:
    :return:
    """
    if not os.path.isdir(result_dir):
        return Constant.MAX_READ_FILE_BYTES
    size = 0
    data_path = PathManager.get_data_dir(result_dir)
    for file in os.listdir(data_path):
        size += os.path.getsize(os.path.join(data_path, file))
    return float(size / CommonConstant.BYTE_SIZE / CommonConstant.BYTE_SIZE)


def get_free_memory(result_dir: str) -> float:
    """
    to get free space
    :param result_dir:
    :return:
    """
    if platform.system() == 'Windows':
        free_bytes = ctypes.c_ulonglong(0)
        ctypes.windll.kernel32.GetDiskFreeSpaceExW(ctypes.c_wchar_p(result_dir), None, None, ctypes.pointer(free_bytes))
        return float(free_bytes.value / CommonConstant.BYTE_SIZE / CommonConstant.BYTE_SIZE)

    dir_info = os.statvfs(result_dir)
    return float(dir_info.f_bavail * dir_info.f_frsize / CommonConstant.BYTE_SIZE / CommonConstant.BYTE_SIZE)


def check_free_memory(result_dir: str) -> None:
    """
    check memory
    :param result_dir:
    :return:
    """
    data_file_memory = get_data_file_memory(result_dir)
    free_memory = get_free_memory(result_dir)
    need_free_memory = data_file_memory * CommonConstant.MEMORY_BUFFER_NUM
    if free_memory < CommonConstant.MINIMUM_DISK_MEMORY:
        warn(os.path.basename(__file__), "The disk space is less than 512 MB, please check")
    if free_memory < need_free_memory:
        warn(
            os.path.basename(__file__),
            "Requires {:.2f}MB of space to store parsed data, actually only {:.2f}MB, "
            "parsed data may not be complete.".format(need_free_memory, free_memory),
        )


def call_sys_exit(status: any = None) -> None:
    """
    call sys.exit to exit the program.
    :param status: the exit status or other object to be printed.
    :return:
    """
    sys.exit(status)
