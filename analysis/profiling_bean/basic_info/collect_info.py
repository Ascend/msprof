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

import os

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from profiling_bean.basic_info.base_info import BaseInfo


class CollectInfo(BaseInfo):
    """
    collect info
    """

    def __init__(self: any) -> None:
        super().__init__()
        self.collection_start_time = ""
        self.collection_end_time = ""
        self.result_size = ""

    def merge_data(self: any) -> any:
        """
        merge data
        :return:
        """
        self.get_collect_time()

    def get_collect_time(self: any) -> None:
        """
        get the time of collect task.
        :return: collect time
        """
        # Compatible for real time and Monotonic time
        if InfoConfReader().get_root_data(StrConstant.COLLECT_RAW_TIME_BEGIN) and InfoConfReader().get_root_data(
            StrConstant.COLLECT_RAW_TIME_END
        ):
            self.collection_start_time, self.collection_end_time = InfoConfReader().get_collect_raw_time()
            return
        self.collection_start_time, self.collection_end_time = InfoConfReader().get_collect_time()

    def get_project_size(self: any, project_path: str) -> None:
        """
        get the project size after analysis.
        :param project_path: project path
        :return: the size of the project
        """
        size = 0
        for dir_path, _, file_names in PathManager.safe_os_walk(project_path):
            for file_name in file_names:
                file_path = os.path.join(dir_path, file_name)
                size += os.path.getsize(file_path)
        self.result_size = (
            str(round(size / (Constant.KILOBYTE * Constant.KILOBYTE), NumberConstant.DECIMAL_ACCURACY)) + " MB"
        )

    def run(self: any, project_path: str) -> None:
        """
        run data
        :param project_path: project path
        :return: None
        """
        self.merge_data()
        self.get_project_size(project_path)
