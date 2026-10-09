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
import re

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_pid_mem_compiles
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from msmodel.hardware.sys_mem_model import SysMemModel
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingMemoryData(MsMultiProcess):
    """
    parse system and process memory data
    """

    PID_DATA_PATTERN = r'^(\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)'
    SYS_DATA_METRIC = (
        'MemTotal',
        'MemFree',
        'Buffers',
        'Cached',
        'Shmem',
        'CommitLimit',
        'Committed_AS',
        'HugePages_Total',
        'HugePages_Free',
    )

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.project_path = sample_config.get("result_dir", "")
        self._file_list = file_list
        self._model = SysMemModel(
            self.project_path,
            DBNameConstant.DB_HOST_SYS_USAGE_MEM,
            [DBNameConstant.TABLE_SYS_MEM, DBNameConstant.TABLE_PID_MEM],
        )
        self.data_dict = {'pid_data_list': [], 'sys_data_list': []}

    def match_sys_data(self: any, line: str, tmp_list: list) -> tuple:
        """
        match sys data in original file
        """
        match_flag = True
        if re.match(r'{}:'.format(self.SYS_DATA_METRIC[0]), line):
            tmp_list[1] = line.split(' ')[-2]
            tmp_list[-1] = line.split(' ')[-1].strip()
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[1]), line):
            tmp_list[2] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[2]), line):
            tmp_list[3] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[3]), line):
            tmp_list[4] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[4]), line):
            tmp_list[5] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[5]), line):
            tmp_list[6] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[6]), line):
            tmp_list[7] = line.split(' ')[-2]
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[7]), line):
            tmp_list[8] = line.split(':')[1].strip()
        elif re.match(r'{}:'.format(self.SYS_DATA_METRIC[8]), line):
            tmp_list[9] = line.split(':')[1].strip()
        elif line == '\n':
            self.data_dict.setdefault('sys_data_list', []).append(tmp_list)
            tmp_list = [None, None, None, None, None, None, None, None, None, None, None]
            match_flag = False

        return tmp_list, match_flag

    def get_sys_data(self: any, file_name: str) -> None:
        """
        get system memory data
        """
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        try:
            with FileOpen(file_path, "r") as file_:
                self._match_sys_data(file_.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def get_pid_data(self: any, file_name: str) -> None:
        """
        get pid memory data
        """
        pid = get_file_name_pattern_match(file_name, *get_pid_mem_compiles()).groups()[0]
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        try:
            with FileOpen(file_path, "r") as file_:
                self._match_pid_data(file_.file_reader, pid)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def main(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in sorted(self._file_list.get(DataTag.SYS_MEM, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, self.project_path):
                    logging.info("start parsing memory data file: %s", file_name)
                    self.get_sys_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)

        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        try:
            for file_name in sorted(self._file_list.get(DataTag.PID_MEM, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, self.project_path):
                    logging.info("start parsing memory data file: %s", file_name)
                    self.get_pid_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data
        """
        if (self.data_dict.get('sys_data_list', []) or self.data_dict.get('pid_data_list', [])) and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.data_dict)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        run function
        """
        try:
            if self._file_list.get(DataTag.SYS_MEM, []) or self._file_list.get(DataTag.PID_MEM, []):
                self.main()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _match_sys_data(self: any, file: any) -> None:
        match_flag = False
        tmp_list = [None, None, None, None, None, None, None, None, None, None, None]
        while True:
            line = file.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            if re.match(r'TimeStamp:\d+', line):
                match_flag = True
                tmp_list[0] = line.split(':')[1].strip()
            if match_flag:
                tmp_list, match_flag = self.match_sys_data(line, tmp_list)

    def _match_pid_data(self: any, file: any, pid: str) -> None:
        match_flag = False
        tmp_list = [None, None, None, None, None]
        while True:
            line = file.readline(Constant.MAX_READ_LINE_BYTES)
            if re.match(r'TimeStamp:\d+', line):
                match_flag = True
                tmp_list[0] = line.split(':')[1].strip()
            if match_flag:
                if 'ProcessName' in line:
                    tmp_list[1] = ''.join(line.split(':')[1:]).strip()
                result = re.match(self.PID_DATA_PATTERN, line)
                if result:
                    tmp_list[2], tmp_list[3], tmp_list[4], _, _, _, _ = result.groups()
            if line == '\n':
                tmp_list.append(pid)
                self.data_dict.setdefault('pid_data_list', []).append(tmp_list)
                tmp_list = [None, None, None, None, None]
                match_flag = False
            if not line:
                break
