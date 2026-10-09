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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo


class IterInfoManager:
    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.iter_to_iter_info = {}
        self._ts_track_model = TsTrackModel(
            self.project_path, DBNameConstant.DB_STEP_TRACE, [ProfilingScene().get_step_table_name()]
        )
        self._ge_model = GeInfoModel(self.project_path)

    @classmethod
    def check_parallel(cls: any, result_dir: str) -> bool:
        """
        check parallel by num of items satisfied condition
        """
        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(result_dir, DBNameConstant.DB_STEP_TRACE), DBNameConstant.TABLE_STEP_TRACE_DATA
        ):
            return False

        with TsTrackModel(
            result_dir, DBNameConstant.DB_STEP_TRACE, [ProfilingScene().get_step_table_name()]
        ) as ts_track_model:
            step_trace_data = ts_track_model.get_step_trace_data(ProfilingScene().get_step_table_name())
        if not step_trace_data:
            return False
        step_trace_data = sorted(step_trace_data, key=lambda x: x.step_start)
        last_timestamp = -1
        for data in step_trace_data:
            if data.step_start < last_timestamp:
                return True
            last_timestamp = data.step_end
        return False

    def initial_iter_to_info(self: any) -> None:
        """
        get behind parallel iter of each iter and aicore info, then register them to each iter
        """
        if not self._ts_track_model.check_table():
            return
        with self._ts_track_model:
            step_trace_data = self._ts_track_model.get_step_trace_data(ProfilingScene().get_step_table_name())
        self.register_parallel_set(step_trace_data)

        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK
        ):
            return
        with self._ge_model:
            static_task_dict = self._ge_model.get_ge_task_data(Constant.GE_STATIC_SHAPE)
            dynamic_task_dict = self._ge_model.get_ge_task_data(Constant.GE_DYNAMIC_SHAPE)
        self.register_aicore_set(static_task_dict, dynamic_task_dict)

    def register_parallel_set(self: any, step_trace_data: list) -> None:
        """
        get behind parallel iter of each iter by two for loops
        """
        is_parallel = self.check_parallel(self.project_path)
        for index, step_trace_datum in enumerate(step_trace_data):
            iter_info = self.iter_to_iter_info.setdefault(
                step_trace_datum.iter_id,
                IterInfo(
                    step_trace_datum.model_id,
                    step_trace_datum.index_id,
                    step_trace_datum.iter_id,
                    step_trace_datum.step_start,
                    step_trace_datum.step_end,
                ),
            )
            if not is_parallel:
                iter_info.behind_parallel_iter.add(step_trace_datum.iter_id)
                continue
            for behind_datum in step_trace_data[index:]:
                behind_iter_info = self.iter_to_iter_info.setdefault(
                    behind_datum.iter_id,
                    IterInfo(
                        behind_datum.model_id,
                        behind_datum.index_id,
                        behind_datum.iter_id,
                        behind_datum.step_start,
                        behind_datum.step_end,
                    ),
                )
                if behind_iter_info.start_time < iter_info.end_time <= behind_iter_info.end_time:
                    iter_info.behind_parallel_iter.add(behind_datum.iter_id)

    def register_aicore_set(self: any, static_task_dict: dict, dynamic_task_dict: dict) -> None:
        """
        get aicore info for each task
        """
        for iter_info_bean in self.iter_to_iter_info.values():
            iter_info_bean.static_aic_task_set = static_task_dict.get(iter_info_bean.model_id, set([]))
            iter_info_bean.dynamic_aic_task_set = dynamic_task_dict.get(
                GeInfoModel.MODEL_INDEX_KEY_FMT.format(iter_info_bean.model_id, iter_info_bean.index_id), set([])
            )
