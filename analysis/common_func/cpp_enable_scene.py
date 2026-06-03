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
from abc import ABC
from typing import List

from common_func.file_manager import check_so_valid
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel


class CppEnableSceneConstant:
    """
    C++ enable scene constant
    """

    DB = "db"
    SUMMARY = "summary"
    TIMELINE = "timeline"


class CppEnableScene(ABC):
    """
    C++ enable scene base class
    """

    # 场景特定的芯片黑名单（子类可以覆盖）
    SCENE_CHIP_BLACKLIST: List[ChipModel] = []

    # 场景特定的芯片白名单（子类可以覆盖，如果设置则优先使用白名单）
    SCENE_CHIP_WHITELIST: List[ChipModel] = []

    def is_cpp_enable(self) -> bool:
        """
        check whether cpp is enabled
        :return: cpp enabled
        """
        so_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "lib64", "msprof_analysis.so")
        if not check_so_valid(so_path):
            return False
        return self._check_scene_conditions()

    def _check_chip_support(self) -> bool:
        """
        check if current chip supports this scene
        :return: True if chip is supported
        """
        # 如果设置了白名单，优先使用白名单
        if self.SCENE_CHIP_WHITELIST:
            return ChipManager().chip_id in self.SCENE_CHIP_WHITELIST
        # 否则使用黑名单
        return ChipManager().chip_id not in self.SCENE_CHIP_BLACKLIST

    def _check_scene_conditions(self) -> bool:
        """
        check scene specific conditions
        :return: whether scene conditions are met
        """
        return self._check_chip_support()


class ExportTimelineScene(CppEnableScene):
    """
    Export timeline scene: check whether cpp export timeline is enabled
    """

    SCENE_CHIP_WHITELIST = [ChipModel.CHIP_V4_1_0, ChipModel.CHIP_V6_1_0, ChipModel.CHIP_V6_2_0]

    def __init__(self, command_type: str = None, is_sample_based: bool = False):
        self.command_type = command_type
        self.is_sample_based = is_sample_based

    def _check_scene_conditions(self) -> bool:
        """
        check export timeline scene conditions
        :return: whether export timeline scene conditions are met
        """
        if self.command_type is None or self.command_type != CppEnableSceneConstant.TIMELINE:
            return False
        if self.is_sample_based:
            return False
        return self._check_chip_support()


class ExportSummaryScene(CppEnableScene):
    """
    Export summary scene: check whether cpp export summary is enabled
    """

    SCENE_CHIP_WHITELIST = [ChipModel.CHIP_V4_1_0, ChipModel.CHIP_V6_1_0, ChipModel.CHIP_V6_2_0]

    def __init__(self, command_type: str = None, is_all_export: bool = False):
        self.command_type = command_type
        self.is_all_export = is_all_export

    def _check_scene_conditions(self) -> bool:
        """
        check export summary scene conditions
        :return: whether export summary scene conditions are met
        """
        if self.command_type is None or self.command_type != CppEnableSceneConstant.SUMMARY:
            return False
        if not self.is_all_export:
            return False
        return self._check_chip_support()


class CannCalculatorScene(CppEnableScene):
    """
    Cann calculator scene: check whether cpp cann calculator is enabled
    """

    SCENE_CHIP_BLACKLIST = [ChipModel.CHIP_V5_1_0, ChipModel.CHIP_V6_2_0]


class DeviceParseScene(CppEnableScene):
    """
    Parse scene: check whether cpp parse is enabled
    """

    SCENE_CHIP_WHITELIST = [ChipModel.CHIP_V4_1_0]


class ExportDBScene(CppEnableScene):
    """
    Export DB scene: check whether cpp export unified db is enabled
    """

    SCENE_CHIP_WHITELIST = [
        ChipModel.CHIP_V2_1_0,
        ChipModel.CHIP_V3_1_0,
        ChipModel.CHIP_V3_2_0,
        ChipModel.CHIP_V3_3_0,
        ChipModel.CHIP_V4_1_0,
        ChipModel.CHIP_V1_1_1,
        ChipModel.CHIP_V6_1_0,
        ChipModel.CHIP_V6_2_0,
    ]


class DataCheckScene(CppEnableScene):
    """
    Data check scene: check whether cpp data check is enabled
    """

    SCENE_CHIP_WHITELIST = [ChipModel.CHIP_V4_1_0, ChipModel.CHIP_V6_1_0, ChipModel.CHIP_V6_2_0]

    def __init__(self, is_all_export: bool = False):
        """
        initialize DataCheckScene
        :param is_all_export: whether all export is enabled
        """
        self.is_all_export = is_all_export

    def _check_scene_conditions(self) -> bool:
        """
        check data check scene conditions
        :return: whether data check scene conditions are met
        """
        return self._check_chip_support() and self.is_all_export
