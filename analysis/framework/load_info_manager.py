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

from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from msconfig.ai_core_config import AICoreConfig


class LoadInfoManager:
    """
    class used to load config
    """

    @staticmethod
    def load_manager() -> None:
        """
        manager load info
        :return:
        """

    @classmethod
    def load_info(cls: any, result_dir: str) -> None:
        """
        load info.json and init profiling scene
        :param result_dir:
        :return: None
        """
        InfoConfReader().load_info(result_dir)
        ChipManager().load_chip_info()
        AiCoreMetricsManager.set_aicore_metrics_list(ChipManager().chip_id)
        AICoreConfig.set_config_data(ChipManager().chip_id)
        ProfilingScene().init(result_dir)
