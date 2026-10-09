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

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from mscalculate.ts_task.task_state_handler import TaskStateHandler
from msmodel.step_trace.ts_track_model import TsTrackModel


class AICpuFromTsCalculator(MsMultiProcess):
    """
    parse ai cpu from ts
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self._ts_model = TsTrackModel(
            self._project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_TASK_TYPE]
        )
        self._aicpu_collector = AICpuFromTsCollector(self._project_path)

    @staticmethod
    def state_to_timeline(ai_cpu_with_state: list) -> list:
        """
        transfer state to start and end
        :param ai_cpu_with_state: ai cpu data with start and end
        :return: ai cpu timeline list
        """
        stream_task_group = {}
        for stream_id, task_id, timestamp, task_state in ai_cpu_with_state:
            task_state_handler = stream_task_group.setdefault(
                (stream_id, task_id), TaskStateHandler(stream_id, task_id)
            )
            task_state_handler.process_record(float(timestamp), task_state)

        aicpu_timeline_list = []
        for task_state_handler in stream_task_group.values():
            aicpu_timeline_list.extend(task_state_handler.task_timeline_list)
        aicpu_timeline_list.sort(key=lambda task_timeline: task_timeline.end_time)
        return aicpu_timeline_list

    def ms_run(self: any) -> None:
        """
        get ai cpu from ts and save to db
        :return:
        """
        self.process_ai_cpu_data()
        self._aicpu_collector.save_aicpu()

    def process_ai_cpu_data(self: any):
        """
        process ai cpu data
        """
        with self._ts_model:
            ai_cpu_with_state = self._ts_model.get_ai_cpu_data(
                MsprofIteration(self._project_path).get_step_syscnt_range_by_iter_range(self._iter_range)
            )

        aicpu_timeline_list = self.state_to_timeline(ai_cpu_with_state)

        for aicpu_timeline in aicpu_timeline_list:
            aicpu_feature = (
                aicpu_timeline.stream_id,
                aicpu_timeline.task_id,
                aicpu_timeline.start_time,
                aicpu_timeline.end_time,
                AICpuFromTsCollector.AI_CPU_TYPE,
            )
            self._aicpu_collector.filter_aicpu(aicpu_feature)
