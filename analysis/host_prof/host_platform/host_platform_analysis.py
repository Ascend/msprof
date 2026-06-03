# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
# -------------------------------------------

import os
import logging
import re
import pathlib

from common_func.common import get_data_dir_sorted_files
from common_func.path_manager import PathManager
from common_func.file_manager import check_dir_writable
from common_func.ms_multi_process import MsMultiProcess
from common_func.file_name_manager import get_host_platform_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from msinterface.msprof_c_interface import export_platform

class HostPlatformAnalysis(MsMultiProcess):
    """
    host platform analysis class
    """

    PLATFORM = "PLATFORM_"
    PROF_RESULT_PATTERN = r".*?PROF_\d{1,10}_\d{1,20}_[A-Z0-9]{5,20}\/"
    PROF_NAME_PATTERN = r"PROF_\d{1,10}_\d{1,20}_[A-Z0-9]{5,20}"

    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.result_dir = self.sample_config.get("result_dir", "")

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return HostPlatformAnalysis.__name__

    @classmethod
    def create_platform_dir(cls: any, path: str) -> str:
        prof_result_path = re.search(cls.PROF_RESULT_PATTERN, path)
        if prof_result_path:
            name = re.search(cls.PROF_NAME_PATTERN, prof_result_path.group(0))
            if name:
                tokens = name.group(0).split("_")
                if len(tokens) > 2 and tokens[2].isnumeric():
                    parent_dir = pathlib.Path(prof_result_path.group(0)).parent
                    try:
                        check_dir_writable(str(parent_dir))
                    except Exception as e:
                        logging.warning(str(e))
                        return ""
                    platform_path = parent_dir.joinpath(cls.PLATFORM + tokens[2])
                    if os.path.exists(platform_path):
                        PathManager.del_dir(str(platform_path))
                    try:
                        os.mkdir(platform_path)
                    except Exception as e:
                        logging.warning("Can't create directory for the platform analysis data: " + str(e))
                        return ""
                    return str(platform_path)
        return ""
    
    def ms_run(self: any) -> None:
        """
        run function
        """
        if not os.path.exists(self.result_dir):
            logging.error(self.result_dir + " directory doesn't exist.")
            return
        
        data_dir = PathManager.get_data_dir(self.result_dir)
        if not os.path.exists(data_dir):
            logging.error(data_dir + " directory doesn't exist.")
            return

        file_list = get_data_dir_sorted_files(data_dir)
        host_platform_compiles = get_host_platform_compiles()

        for file_name in file_list:
            host_platform_result = get_file_name_pattern_match(file_name, *host_platform_compiles)
            if host_platform_result:
                platform_dir = HostPlatformAnalysis.create_platform_dir(data_dir)
                if platform_dir:
                    file_path = str(pathlib.Path(data_dir).joinpath(file_name))
                    export_platform(file_path, platform_dir)
                break

