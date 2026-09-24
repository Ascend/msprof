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

import logging
import os
from typing import List

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.info_conf_reader import InfoConfReader
from mscalculate.ascend_task.ascend_task import DeviceTask
from mscalculate.flip.flip_calculator import FlipCalculator
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msmodel.stars.ffts_log_model import FftsLogModel
from msmodel.task_time.hwts_aiv_model import HwtsAivModel
from msmodel.task_time.hwts_log_model import HwtsLogModel
from msmodel.task_time.runtime_task_time_model import RuntimeTaskTimeModel
from msmodel.v5.v5_stars_model import V5StarsViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel


class DeviceTaskCollector:
    def __init__(self, result_dir: str):
        self.result_dir = result_dir
        self.chip = ChipManager().get_chip_id()
        self.collectors = {
            ChipModel.CHIP_V1_1_0: self._gather_chip_v1_1_0_device_tasks,
            ChipModel.CHIP_V2_1_0: self._gather_chip_v2_1_0_device_tasks,
            ChipModel.CHIP_V3_1_0: self._gather_chip_v3_device_tasks,
            ChipModel.CHIP_V3_2_0: self._gather_chip_v3_device_tasks,
            ChipModel.CHIP_V3_3_0: self._gather_chip_v3_device_tasks,
            ChipModel.CHIP_V4_1_0: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V1_1_1: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V1_1_2: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V1_1_3: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V5_1_0: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V6_1_0: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V6_1_1: self._gather_chip_stars_device_tasks,
            ChipModel.CHIP_V6_2_0: self._gather_chip_stars_device_tasks,
        }

        self.check_dbs = {
            ChipModel.CHIP_V1_1_0: [DBNameConstant.DB_RUNTIME],
            ChipModel.CHIP_V2_1_0: [DBNameConstant.DB_HWTS, DBNameConstant.DB_AI_CPU],
            ChipModel.CHIP_V3_1_0: [DBNameConstant.DB_HWTS, DBNameConstant.DB_HWTS_AIV],
            ChipModel.CHIP_V3_2_0: [DBNameConstant.DB_HWTS],
            ChipModel.CHIP_V3_3_0: [DBNameConstant.DB_HWTS],
            ChipModel.CHIP_V4_1_0: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V1_1_1: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V1_1_2: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V1_1_3: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V5_1_0: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V6_1_0: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V6_1_1: [DBNameConstant.DB_SOC_LOG],
            ChipModel.CHIP_V6_2_0: [DBNameConstant.DB_SOC_LOG],
        }

    def get_all_device_tasks(self: any) -> List[DeviceTask]:
        if not self._check_device_data_db_exists():
            return []
        device_tasks = self.collectors.get(self.chip)(float('-inf'), float('inf'))
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            device_tasks = FlipCalculator.set_device_batch_id(device_tasks, self.result_dir)
        return device_tasks

    def get_device_tasks_by_model_and_iter(self, model_id, iter_id) -> List[DeviceTask]:
        if not self._check_device_data_db_exists():
            return []
        iter_range = IterationRange(model_id=model_id, iteration_id=iter_id, iteration_count=1)
        time_range = MsprofIteration(self.result_dir).get_iteration_time(iter_range)[0]
        if not time_range:
            logging.error("Get time range error")
            return []
        iter_start, iter_end = time_range
        chip = ChipManager().get_chip_id()
        device_tasks = self.collectors.get(chip)(
            iter_start * NumberConstant.NS_TO_US, iter_end * NumberConstant.NS_TO_US
        )
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            device_tasks = FlipCalculator.set_device_batch_id(device_tasks, self.result_dir)
        return device_tasks

    def get_sub_tasks_by_time_range(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        return self._gather_device_ffts_plus_sub_tasks_from_stars(start_time, end_time)

    def _gather_device_tasks_from_hwts(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        """
        gather all device tasks in hwts.
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS)
        if not os.path.exists(db_path):
            logging.warning("no db %s found", DBNameConstant.DB_HWTS)
            return []

        with HwtsLogModel(self.result_dir) as model:
            return model.get_hwts_data_within_time_range(start_time, end_time)

    def _gather_device_tasks_from_hwts_aiv(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        """
        gather all device tasks in hwts_aiv.
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS_AIV)
        if not os.path.exists(db_path):
            logging.warning("no db %s found", DBNameConstant.DB_HWTS_AIV)
            return []

        with HwtsAivModel(self.result_dir, [DBNameConstant.TABLE_HWTS_TASK_TIME]) as model:
            return model.get_hwts_aiv_data_within_time_range(start_time, end_time)

    def _gather_ai_cpu_device_tasks_from_ts(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_AI_CPU)
        if not os.path.exists(db_path):
            logging.warning("no db %s found", DBNameConstant.DB_AI_CPU)
            return []

        with AiCpuModel(self.result_dir) as model:
            return model.get_ai_cpu_data_within_time_range(start_time, end_time)

    def _gather_device_acsq_tasks_from_stars(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_ACSQ_TASK):
            logging.warning("no %s.%s found", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_ACSQ_TASK)
            return []

        with AcsqTaskModel(self.result_dir, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_ACSQ_TASK]) as model:
            return model.get_acsq_data_within_time_range(start_time, end_time)

    def _gather_device_ffts_plus_sub_tasks_from_stars(
        self: any, start_time: float, end_time: float
    ) -> List[DeviceTask]:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_SUBTASK_TIME):
            logging.warning("no %s.%s found", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_SUBTASK_TIME)
            return []

        with FftsLogModel(self.result_dir, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_SUBTASK_TIME]) as model:
            return model.get_ffts_plus_sub_task_data_within_time_range(start_time, end_time)

    def _gather_device_chip_v5_tasks_from_stars(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_V5_TASK):
            logging.warning("no %s.%s found", DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_V5_TASK)
            return []

        with V5StarsViewModel(self.result_dir) as model:
            return model.get_v5_data_within_time_range(start_time, end_time)

    def _gather_device_tasks_from_runtime(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_RUNTIME)
        if not os.path.exists(db_path):
            logging.warning("no db %s found", DBNameConstant.DB_RUNTIME)
            return []

        with RuntimeTaskTimeModel(self.result_dir) as model:
            return model.get_runtime_task_data_within_time_range(start_time, end_time)

    def _gather_chip_v1_1_0_device_tasks(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        device_tasks = self._gather_device_tasks_from_runtime(start_time, end_time)
        if not device_tasks:
            logging.error("no device task found.")
            return []
        return device_tasks

    def _gather_chip_v2_1_0_device_tasks(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        # in this chip  only aicore task will be uploaded in hwts.
        # Since aicpu task can only be collected from ts_track
        ai_core_device_tasks = self._gather_device_tasks_from_hwts(start_time, end_time)

        ai_cpu_device_tasks = self._gather_ai_cpu_device_tasks_from_ts(start_time, end_time)
        if not ai_cpu_device_tasks and not ai_core_device_tasks:
            logging.error("no aicore and ai_cpu device task found.")
            return []

        return [*ai_core_device_tasks, *ai_cpu_device_tasks]

    def _gather_chip_v3_device_tasks(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        # in this chip ai_core and ai_cpu data will be uploaded in hwts data
        ai_core_ai_cpu_tasks = self._gather_device_tasks_from_hwts(start_time, end_time)
        # these tasks only take effects in CHIP_V3_1_0
        aiv_tasks = self._gather_device_tasks_from_hwts_aiv(start_time, end_time)

        device_tasks = [*ai_core_ai_cpu_tasks, *aiv_tasks]

        if not device_tasks:
            logging.error("no aic, aiv and ai_cpu device task found.")
        return device_tasks

    def _gather_chip_stars_device_tasks(self: any, start_time: float, end_time: float) -> List[DeviceTask]:
        # in this chip ai_core and ai_cpu data will be uploaded in soc_stars data
        device_acsq_tasks = self._gather_device_acsq_tasks_from_stars(start_time, end_time)
        device_ffts_tasks = self._gather_device_ffts_plus_sub_tasks_from_stars(start_time, end_time)
        device_v5_tasks = self._gather_device_chip_v5_tasks_from_stars(start_time, end_time)
        device_tasks = [*device_acsq_tasks, *device_ffts_tasks, *device_v5_tasks]

        if not device_tasks:
            logging.error("no acsq, ffts device task found.")
        return device_tasks

    def _check_device_data_db_exists(self: any) -> bool:
        dbs = self.check_dbs.get(self.chip)
        lost_db = []
        for db in dbs:
            db_path = PathManager.get_db_path(self.result_dir, db)
            if os.path.exists(db_path):
                continue
            lost_db.append(db)
        if lost_db:
            logging.warning("No device data db found within %s", lost_db)
        return not (len(lost_db) == len(dbs))
