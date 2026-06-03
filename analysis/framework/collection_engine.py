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

import json
import logging
import multiprocessing
import os
import sys
import traceback

from common_func.common import print_msg
from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import check_dir_writable
from common_func.msvp_common import clear_project_dirs
from common_func.msvp_common import error
from common_func.path_manager import PathManager
from host_prof.host_cpu_usage.cpu_usage_analysis import CpuUsageAnalysis
from host_prof.host_disk_usage.disk_usage_analysis import DiskUsageAnalysis
from host_prof.host_mem_usage.mem_usage_analysis import MemUsageAnalysis
from host_prof.host_network_usage.network_usage_analysis import NetworkUsageAnalysis
from host_prof.host_syscall.host_syscall_analysis import HostSyscallAnalysis
from host_prof.host_platform.host_platform_analysis import HostPlatformAnalysis


class AI:
    """
    provides APIs to call methods of parsing data, creating databases
    and insert them into databases
    """

    FILE_NAME = os.path.basename(__file__)
    FOLDER_MASK = 0o750

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        sys.path.append(os.path.realpath(os.path.dirname(os.path.realpath(sys.argv[0])) + "/.."))
        self._version = None

    @classmethod
    def project_preparation(cls: any, project_dir: str) -> None:
        """
        project preparation before collect start
        """
        try:
            cls.create_project_dirs(project_dir)
        except (OSError, SystemError, IOError) as err:
            error(cls.FILE_NAME, err)

    @classmethod
    def create_project_dirs(cls: any, project_dir: str) -> None:
        """
        create data and corresponding directories
        """
        check_dir_writable(project_dir)
        if not os.path.exists(PathManager.get_data_dir(project_dir)):
            os.makedirs(PathManager.get_data_dir(project_dir), cls.FOLDER_MASK)
        if not os.path.exists(PathManager.get_sql_dir(project_dir)):
            os.makedirs(PathManager.get_sql_dir(project_dir), cls.FOLDER_MASK)
        cls._create_collection_log(project_dir)
        clear_project_dirs(project_dir)

    @classmethod
    def _create_collection_log(cls: any, project_dir: str) -> None:
        check_dir_writable(PathManager.get_log_dir(project_dir), create_dir=True)
        try:
            if os.path.exists(PathManager.get_log_dir(project_dir)) and not os.path.exists(
                PathManager.get_collection_log_path(project_dir)
            ):
                file_path = PathManager.get_collection_log_path(project_dir)
                with FdOpen(file_path):
                    os.chmod(file_path, NumberConstant.FILE_AUTHORITY)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(cls.FILE_NAME, err)

    def import_control_flow(self: any) -> None:
        """
        parse the collected data and load data into database
        """
        try:
            self.data_analysis()
        except multiprocessing.ProcessError as err:
            logging.error(str(err))
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': traceback.format_exc()}))
        logging.info('Database process finished.')
        logging.info('Analysis finished.')

    def formula_list(self: any, data_class: any) -> None:
        """
        use multi-processing to run data class
        :param data_class: class ready to be parsed
        :return:
        """
        parsing_obj = []
        for parsing_class in data_class:
            parsing_obj.append(parsing_class(self.sample_config))
            # start parsing processor
        for item in parsing_obj:
            item.start()
        # join parsing processor
        for item in parsing_obj:
            item.join()

    def data_analysis(self: any) -> None:
        """
        parsing data file
        """
        # init data parsing object
        if InfoConfReader().is_host_profiling():
            self.formula_list(
                [
                    CpuUsageAnalysis,
                    MemUsageAnalysis,
                    DiskUsageAnalysis,
                    NetworkUsageAnalysis,
                    HostSyscallAnalysis,
                    HostPlatformAnalysis,
                ]
            )
