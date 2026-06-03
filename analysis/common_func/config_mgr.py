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
import os

from common_func.common import CommonConstant, error
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.file_manager import FileOpen


class ConfigMgr:
    """
    config class
    """

    COMMON_FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def get_ddr_bit_width() -> int:
        """
        get ddr bit width
        """
        platform_version = InfoConfReader().get_root_data(Constant.PLATFORM_VERSION)
        if platform_version in (Constant.CHIP_V2_1_0, Constant.CHIP_V3_1_0, Constant.CHIP_V3_2_0, Constant.CHIP_V3_3_0):
            return 256
        return 128

    @staticmethod
    def has_llc_capacity(result_dir: str) -> bool:
        """
        check whether capacity config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM

    @staticmethod
    def has_llc_bandwidth(result_dir: str) -> bool:
        """
        check whether bandwidth config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM

    @staticmethod
    def has_llc_read_write(result_dir: str) -> bool:
        """
        check whether read or write config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) in [
            StrConstant.LLC_PROFILING_READ_EVENT,
            StrConstant.LLC_PROFILING_WRITE_EVENT,
        ]

    @staticmethod
    def is_ai_core_sample_based(result_dir: str) -> bool:
        """
        check scene of ai core sample-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE

    @staticmethod
    def is_ai_core_task_based(result_dir: str) -> bool:
        """
        check scene of ai core task-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        aic_mode = sample_config.get(StrConstant.AICORE_PROFILING_MODE)
        return aic_mode == StrConstant.AIC_TASK_BASED_MODE if aic_mode != "" else True

    @staticmethod
    def is_aiv_sample_based(result_dir: str) -> bool:
        """
        check scene of aiv sample-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE

    @staticmethod
    def get_disk_freq(result_dir: str) -> any:
        """
        get disk profile freq
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.HOST_DISK_FREQ)

    @staticmethod
    def read_sample_config(collection_path: str) -> any:
        """
        read sample config by the collection path
        :return: the sample config
        """
        sample_file = os.path.join(collection_path, Constant.SAMPLE_FILE)
        try:
            with FileOpen(sample_file, "r") as json_file:
                return json.load(json_file.file_reader)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            message = f"Failed to load {sample_file}. {err}"
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message) from err

    @staticmethod
    def pre_check_sample(result_dir: str, event: any) -> dict:
        sample_config = ConfigMgr.read_sample_config(result_dir)
        if not sample_config:
            error(CommonConstant.FILE_NAME, 'Failed to generate sample configuration table.')
            return {}
        profiling_events = sample_config.get(event, '')
        for i in profiling_events.split(','):
            try:
                int(i, Constant.HEX_NUMBER)
            except ValueError:
                error(CommonConstant.FILE_NAME, 'Failed to verify configuration file parameters.')
                return {}
        return sample_config

    @staticmethod
    def is_custom_pmu_scene(result_dir: str) -> bool:
        """
        check scene is custom pmu
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        metrics = sample_config.get("ai_core_metrics")
        return metrics.startswith("Custom") if metrics else False
