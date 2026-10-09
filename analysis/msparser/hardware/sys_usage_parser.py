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

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msmodel.hardware.sys_usage_model import SysUsageModel
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingCpuUsageData(MsMultiProcess):
    """
    parsing cpu usage data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.project_path = sample_config.get("result_dir", "")
        self._file_list = file_list
        self._model = SysUsageModel(
            self.project_path,
            DBNameConstant.DB_HOST_SYS_USAGE_CPU,
            [DBNameConstant.TABLE_SYS_USAGE, DBNameConstant.TABLE_PID_USAGE],
        )
        self.data_dict = {"sys_data_list": [], "pid_data_list": []}
        self.file_all = None

    def get_sys_cpu_data(self: any, file_name: str) -> None:
        """
        get system memory data
        """
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        try:
            with FileOpen(file_path, "r") as file_:
                self._generate_sys_data(file_.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def get_proc_cpu_data(self: any, file_name: str) -> None:
        """
        get pid cpu data
        """
        file_path = PathManager.get_data_file_path(self.project_path, file_name)
        try:
            with FileOpen(file_path, "r") as file_path:
                self._generate_proc_cpu_data(file_path.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data
        """
        if (self.data_dict.get('sys_data_list', []) or self.data_dict.get('pid_data_list', [])) and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.data_dict)
            self._model.finalize()

    def main(self: any) -> None:
        """
        run parsing data file
        """
        try:
            for file_name in sorted(self._file_list.get(DataTag.SYS_USAGE, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, self.project_path):
                    logging.info("start parsing cpu data file: %s", file_name)
                    self.get_sys_cpu_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        try:
            for file_name in sorted(self._file_list.get(DataTag.PID_USAGE, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, self.project_path):
                    logging.info("start parsing pid data file: %s", file_name)
                    self.get_proc_cpu_data(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not os.path.exists(os.path.join(self.project_path, 'data')):
            logging.warning("No CPU usage path found in project path.")
            return
        try:
            if self._file_list.get(DataTag.SYS_USAGE, []) or self._file_list.get(DataTag.PID_USAGE, []):
                self.main()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _generate_sys_data(self: any, file: any) -> None:
        match_flag = False
        tmp_list = Utils.generator_to_list(None for _ in range(13))
        while True:
            line = file.readline(Constant.MAX_READ_LINE_BYTES)
            if re.match(r'TimeStamp:\d+', line):
                match_flag = True
                tmp_list[0] = line.split(':')[1].strip()
                timestamp = tmp_list[0]
            if match_flag:
                if re.match(r'cpu', line):
                    # pylint: disable=possibly-used-before-assignment
                    self._sys_data_match_cpu(tmp_list, line, timestamp)
                    tmp_list = Utils.generator_to_list(None for _ in range(13))
                elif line == '\n':
                    match_flag = False
            if not line:
                break

    def _generate_proc_cpu_data(self: any, file: any) -> None:
        match_flag = False
        proc_data_list = Utils.generator_to_list(None for _ in range(8))
        while True:
            line = file.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            if re.match(r'TimeStamp:', line):
                match_flag = True
                proc_data_list[6] = line.split(':')[1].strip()
            if match_flag:
                proc_data_list = self._proc_data_match_process(proc_data_list, line)
                if line == '\n':
                    match_flag = False

    def _proc_data_match_process(self: any, proc_data_list: list, line: str) -> list:
        if re.match(r'\d+', line):
            proc_data_list[0] = line.split(' ')[0].strip()
            proc_data_list[2] = line.split(' ')[13].strip()
            proc_data_list[3] = line.split(' ')[14].strip()
            proc_data_list[4] = line.split(' ')[15].strip()
            proc_data_list[5] = line.split(' ')[16].strip()
        if "ProcessName" in line:
            proc_data_list[1] = ''.join(line.split(':')[1:]).strip()
        if re.match(r'cpu', line):
            proc_data_list[7] = sum(Utils.generator_to_list(float(i) for i in line.split(' ')[1:] if i != ''))
            self.data_dict.setdefault("pid_data_list", []).append(proc_data_list)
            return Utils.generator_to_list(None for _ in range(8))
        return proc_data_list

    def _sys_data_match_cpu(self: any, tmp_list: list, line: str, timestamp: int) -> None:
        ctrl_cpu = InfoConfReader().get_data_under_device("ctrl_cpu")
        ctrl_cpu = Utils.generator_to_list("cpu" + i for i in ctrl_cpu.split(',') if i.isdigit())
        ai_cpu = InfoConfReader().get_data_under_device("ai_cpu")
        ai_cpu = Utils.generator_to_list("cpu" + i for i in ai_cpu.split(',') if i.isdigit())
        line_data = Utils.generator_to_list(i for i in line.split(' ') if i != "")
        if len(line_data) >= 11:  # 10 is the minimum length for line.split
            tmp_list[0] = timestamp
            tmp_list[1:12] = line_data[0:11]
        if ctrl_cpu and tmp_list[1] in ctrl_cpu:
            tmp_list[12] = "ctrlcpu"
        elif ai_cpu and tmp_list[1] in ai_cpu:
            tmp_list[12] = "aicpu"
        elif InfoConfReader().is_host_profiling():
            tmp_list[12] = "host"
        self.data_dict.setdefault("sys_data_list", []).append(tmp_list)
