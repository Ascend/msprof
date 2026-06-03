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

import os

from common_func import file_name_manager
from common_func.constant import Constant
from common_func.file_manager import check_dir_readable
from common_func.path_manager import PathManager
from common_func.config_mgr import ConfigMgr
from common_func.msprof_common import get_path_dir
from common_func.ms_constant.str_constant import StrConstant
from common_func.profiling_scene import ProfilingScene
from common_func.cpp_enable_scene import DataCheckScene


class DataCheckManager:
    """
    The data check manager
    """

    DATA_NAME_LENGTH = 4
    HOST_DATA_NAME_LENGTH = 3
    DEVICE_ID_INDEX = 2

    @staticmethod
    def check_prof_level0(result_dir: str) -> bool:
        sub_dirs = sorted(get_path_dir(result_dir), reverse=True)
        sample_config = {}
        for sub_dir in sub_dirs:
            sub_path = os.path.realpath(os.path.join(result_dir, sub_dir))
            if os.path.exists(os.path.join(sub_path, Constant.SAMPLE_FILE)):
                sample_config = ConfigMgr.read_sample_config(sub_path)
                break
        return (
            sample_config.get("profLevel", "") == StrConstant.PROF_LEVEL_0
            or sample_config.get("prof_level", "") == StrConstant.PROF_LEVEL_0_HISI
        )

    @classmethod
    def check_export_with_so(cls):
        return DataCheckScene(ProfilingScene().is_all_export()).is_cpp_enable()

    @classmethod
    def check_data_exist(cls: any, result_dir: str, file_patterns: any, device_id: any = None) -> bool:
        """
        check data path readable
        """
        data_path = PathManager.get_data_dir(result_dir)
        check_dir_readable(data_path)
        for file_name in os.listdir(data_path):
            for file_pattern_compile in file_patterns:
                if not file_pattern_compile.match(file_name):
                    continue
                if not cls._check_file_size(file_name, data_path):
                    continue
                if cls._check_device_id_and_file_name(device_id, file_name):
                    return True
        return False

    @classmethod
    def contain_info_json_data(cls: any, result_dir: str, device_info_only: bool = False) -> bool:
        """
        The data path contain job info data or not
        """
        if not os.path.isdir(result_dir):
            return False
        info_json_compiles = file_name_manager.get_info_json_compiles(device_info_only)
        for file_name in os.listdir(result_dir):
            for info_json_compile in info_json_compiles:
                if info_json_compile.match(file_name):
                    return True
        return False

    @classmethod
    def _check_file_size(cls: any, file_name: str, data_dir: str) -> bool:
        if file_name.endswith(Constant.DONE_TAG) or file_name.endswith(Constant.COMPLETE_TAG):
            return False
        _file_path = os.path.realpath(os.path.join(data_dir, file_name))
        file_size = os.path.getsize(_file_path)
        return file_size > 0

    @classmethod
    def _check_device_id_and_file_name(cls: any, device_id: int, file_name: str) -> bool:
        if device_id is None:
            return True
        split_list = file_name.split('.')
        if len(split_list) == cls.HOST_DATA_NAME_LENGTH:
            return True
        if len(split_list) == cls.DATA_NAME_LENGTH and int(split_list[cls.DEVICE_ID_INDEX]) == device_id:
            return True
        return False
