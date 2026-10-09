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

from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_host_disk_usage_compiles
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from host_prof.host_disk_usage.presenter.host_disk_usage_presenter import HostDiskUsagePresenter


class DiskUsageAnalysis(MsMultiProcess):
    """
    disk usage analysis class
    """

    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.result_dir = self.sample_config.get("result_dir", "")

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return DiskUsageAnalysis.__name__

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not os.path.exists(self.result_dir):
            logging.error("Data project doesn't exist.")
            return
        file_list = os.listdir(PathManager.get_data_dir(self.result_dir))
        host_disk_usage_file_patterns = get_host_disk_usage_compiles()
        for file_name in file_list:
            host_disk_usage_result = get_file_name_pattern_match(file_name, *host_disk_usage_file_patterns)
            if host_disk_usage_result and is_valid_original_data(file_name, self.result_dir):
                logging.info("start parsing disk usage data file: %s", file_name)
                host_disk_usage_presenter = HostDiskUsagePresenter(self.result_dir, file_name)
                host_disk_usage_presenter.run()
