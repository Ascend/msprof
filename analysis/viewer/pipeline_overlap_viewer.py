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
import decimal
import logging
import os
from enum import Enum

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.section_calculator import SectionCalculator
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.hccl.hccl_model import HcclViewModel
from msmodel.stars.op_summary_model import OpSummaryModel
from msmodel.task_time.ascend_task_model import AscendTaskViewModel


class OverlapType(Enum):
    COMPUTE_TIME = 0
    COMMUNICATION_TIME = 1
    COMMUNICATION_NOT_OVERLAPPED = 2
    FREE_TIME = 3


class PipelineOverlapViewer:
    OVERLAP_TYPE_NAME = {
        "COMPUTE_TIME": "Computing",
        "COMMUNICATION_TIME": "Communication",
        "COMMUNICATION_NOT_OVERLAPPED": "Communication(Not Overlapped)",
        "FREE_TIME": "Free",
    }
    FILTER_TYPE = Constant.COMP_TASK_TYPE + [Constant.PROFILING_ENABLE, Constant.PROFILING_DISABLE]

    def __init__(self, configs: dict, params: dict):
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()

    def get_timeline_data(self):
        result = []
        compute_data = []
        if os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            sample_config = {
                'result_dir': self._project_path,
                'iter_id': Constant.DEFAULT_INVALID_VALUE,
                'model_id': Constant.DEFAULT_INVALID_VALUE,
            }
            with OpSummaryModel(sample_config) as _model:
                compute_data = _model.get_operator_data_separated_by_kfc_stream()

        compute_data = SectionCalculator.merge_continuous_intervals(compute_data)
        result.extend(self._format_timeline_data(OverlapType.COMPUTE_TIME, data) for data in compute_data)

        communication_data = []
        if os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            with HcclViewModel(
                self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]
            ) as _model:
                communication_data = _model.get_hccl_op_time_section(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE)
                communication_data += _model.get_hccl_op_time_section(DBNameConstant.TABLE_KFC_OP)
        communication_data = SectionCalculator.merge_continuous_intervals(communication_data)
        result.extend(self._format_timeline_data(OverlapType.COMMUNICATION_TIME, data) for data in communication_data)

        if not result:
            logging.warning("Both task data and hccl data are missing, no need to calculate the overlap.")
            return []

        latest_task, earliest_task = None, None
        if os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_ASCEND_TASK)):
            with AscendTaskViewModel(self._project_path, [DBNameConstant.TABLE_ASCEND_TASK]) as _model:
                latest_task, earliest_task = _model.get_ascend_task_time_extremes(self.FILTER_TYPE)
        latest_time = latest_task.start_time + latest_task.duration if latest_task else 0
        earliest_time = earliest_task.start_time if earliest_task else float('inf')
        pure_communication_section, free_time_section = SectionCalculator.compute_pipeline_overlap(
            communication_data, compute_data, latest_time, earliest_time
        )
        result.extend(
            self._format_timeline_data(OverlapType.COMMUNICATION_NOT_OVERLAPPED, data)
            for data in pure_communication_section
        )
        result.extend(self._format_timeline_data(OverlapType.FREE_TIME, data) for data in free_time_section)
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, result)
        _trace.extend(
            TraceViewManager.metadata_event(
                [
                    [
                        "process_name",
                        self._pid,
                        InfoConfReader().get_json_tid_data(),
                        TraceViewHeaderConstant.PROCESS_OVERLAP_ANALYSE,
                    ]
                ]
            )
        )
        _trace.extend(
            TraceViewManager.metadata_event(
                [
                    ["thread_name", self._pid, overlap_type.value, self.OVERLAP_TYPE_NAME.get(overlap_type.name)]
                    for overlap_type in OverlapType
                ]
            )
        )
        return _trace

    def _format_timeline_data(self, overlap_type, data):
        start = InfoConfReader().trans_into_local_time(data.start_time)
        end = InfoConfReader().trans_into_local_time(data.end_time)
        return [
            self.OVERLAP_TYPE_NAME.get(overlap_type.name),
            self._pid,
            overlap_type.value,
            start,
            float(decimal.Decimal(end) - decimal.Decimal(start)),
        ]
