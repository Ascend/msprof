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

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.iter_recorder import IterRecorder
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel


class AICpuFromTsCollector:
    """
    AI Cpu from ts collector
    """

    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"
    AI_CPU_TYPE = 1

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._aicpu_model = AiCpuModel(self._project_path, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        self._iter_recorder = IterRecorder(self._project_path)
        self.aicpu_list = []

    def filter_aicpu(self: any, aicpu_feature: tuple) -> None:
        """
        filter ai cpu
        :param aicpu_feature: aicpu feature
        """
        stream_id, task_id, start, end, task_type = aicpu_feature
        if task_type == self.AI_CPU_TYPE:
            start_ms = InfoConfReader().time_from_syscnt(start)
            end_ms = InfoConfReader().time_from_syscnt(end)
            if not ChipManager().is_chip_v1():
                start_ms = InfoConfReader().time_from_syscnt(start) / NumberConstant.MS_TO_NS
                end_ms = InfoConfReader().time_from_syscnt(end) / NumberConstant.MS_TO_NS

            self.aicpu_list.append([int(stream_id), int(task_id), start_ms, end_ms])

    def save_aicpu(self: any) -> None:
        """
        save ai cpu data
        """
        if not self.aicpu_list:
            logging.info("no aicpu data, no need to flush %s", DBNameConstant.TABLE_AI_CPU_FROM_TS)
            return
        with self._aicpu_model:
            self._aicpu_model.flush(self.aicpu_list, DBNameConstant.TABLE_AI_CPU_FROM_TS)
