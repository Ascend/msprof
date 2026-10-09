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

from common_func.config_mgr import ConfigMgr
from common_func.ms_constant.str_constant import StrConstant
from framework.config_data_parsers import ConfigDataParsers
from framework.iprof_factory import IProfFactory
from msconfig.config_manager import ConfigManager


class ParserFactory(IProfFactory):
    """
    factory for create data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict, chip_model: str) -> None:
        self._sample_config = sample_config
        self._file_list = file_list
        self._chip_model = chip_model
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def generate(self: any, chip_model: any) -> dict:
        """
        generate parser
        """
        task_flag = ConfigMgr.is_ai_core_task_based(self._project_path) and not ConfigMgr.is_custom_pmu_scene(
            self._project_path
        )
        return ConfigDataParsers.get_parsers(ConfigManager.DATA_PARSERS, chip_model, task_flag)

    def run(self: any) -> None:
        """
        run function
        """
        self._launch_parser_list(self._sample_config, self._file_list, self.generate(self._chip_model))
