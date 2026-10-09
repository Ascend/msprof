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

from msconfig.ai_core_config import AICoreConfig
from msconfig.ai_cpu_config import AICPUConfig
from msconfig.constant_config import ConstantConfig
from msconfig.ctrl_cpu_config import CtrlCPUConfig
from msconfig.data_calculator_config import DataCalculatorConfig
from msconfig.data_parsers_config import DataParsersConfig
from msconfig.filename_introduction_config import FilenameIntroductionConfig
from msconfig.l2_cache_config import L2CacheConfig
from msconfig.meta_config import MetaConfig
from msconfig.msprof_export_data_config import MsProfExportDataConfig
from msconfig.prof_condition_config import ProfConditionConfig
from msconfig.soc_pmu_config import SocPmuConfig
from msconfig.stars_config import StarsConfig, FuncTypeConfig
from msconfig.tables_config import TablesConfig
from msconfig.tables_operator_config import TablesOperatorConfig
from msconfig.tables_training_config import TablesTrainingConfig
from msconfig.ts_cpu_config import TsCPUConfig


class ConfigManager:
    PROF_CONDITION = 'ProfConditionConfig'
    DATA_PARSERS = 'DataParsersConfig'
    STARS = 'StarsConfig'
    TABLES = 'TablesConfig'
    TABLES_TRAINING = 'TablesTrainingConfig'
    TABLES_OPERATOR = 'TablesOperatorConfig'
    MSPROF_EXPORT_DATA = 'MsProfExportDataConfig'
    DATA_CALCULATOR = 'DataCalculatorConfig'
    AI_CORE = "AICoreConfig"
    AI_CPU = "AICPUConfig"
    CTRL_CPU = "CtrlCPUConfig"
    TS_CPU = "TsCPUConfig"
    CONSTANT = "ConstantConfig"
    L2_CACHE = "L2CacheConfig"
    SOC_PMU = "SocPmuConfig"
    FILENAME_INTRODUCTION = "FilenameIntroductionConfig"
    FUNC_TYPE = "FuncTypeConfig"

    CONFIG_MAP = {
        PROF_CONDITION: ProfConditionConfig,
        DATA_PARSERS: DataParsersConfig,
        STARS: StarsConfig,
        TABLES: TablesConfig,
        TABLES_TRAINING: TablesTrainingConfig,
        TABLES_OPERATOR: TablesOperatorConfig,
        MSPROF_EXPORT_DATA: MsProfExportDataConfig,
        DATA_CALCULATOR: DataCalculatorConfig,
        AI_CORE: AICoreConfig,
        AI_CPU: AICPUConfig,
        CTRL_CPU: CtrlCPUConfig,
        TS_CPU: TsCPUConfig,
        CONSTANT: ConstantConfig,
        L2_CACHE: L2CacheConfig,
        SOC_PMU: SocPmuConfig,
        FILENAME_INTRODUCTION: FilenameIntroductionConfig,
        FUNC_TYPE: FuncTypeConfig,
    }

    @classmethod
    def get(cls: any, config_name: str):
        config_class = cls.CONFIG_MAP.get(config_name)
        if not config_class:
            config_class = MetaConfig
            logging.error('%s not found', config_name)
        return config_class()
