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

# pylint: disable=duplicate-code

import logging

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.section_calculator import SectionCalculator
from mscalculate.ts_task.ai_cpu.aicpu_from_ts import AICpuFromTsCalculator
from msmodel.iter_rec.iter_rec_model import HwtsIterViewModel
from msmodel.parallel.cluster_hccl_model import ClusterHCCLViewModel
from msmodel.parallel.parallel_model import ParallelModel, ParallelViewModel
from msmodel.step_trace.ts_track_model import TsTrackViewModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class ParallelParser(IParser, MsMultiProcess):
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_op_data = []
        self._merged_compute_op_data = []
        self._iter_time_data = []
        self._hccl_overlap_time_data = []
        self._iter_compute_time_data = []

    def ms_run(self: any) -> None:
        if not self._file_list.get(DataTag.PARALLEL_STRATEGY, []):
            return
        with ParallelViewModel(self._project_path) as _model:
            if _model.get_parallel_table_name() == Constant.NA:
                return
        logging.info("Start to parse parallel index related data.")
        self.parse()
        self.save()

    def parse(self: any) -> None:
        if not self._prepare_for_parse():
            return
        hccl_op_overlap_time = SectionCalculator.compute_overlap_time(self._hccl_op_data, self._merged_compute_op_data)
        iter_overlap_time = SectionCalculator.compute_overlap_time(self._iter_time_data, self._merged_compute_op_data)
        for hccl_op in hccl_op_overlap_time:
            self._hccl_overlap_time_data.append(
                [
                    hccl_op.model_id,
                    hccl_op.index_id,
                    hccl_op.op_name,
                    hccl_op.op_type,
                    hccl_op.start_time,
                    hccl_op.end_time,
                    hccl_op.overlap_time,
                ]
            )
        for iter_data in iter_overlap_time:
            self._iter_compute_time_data.append(
                [
                    iter_data.model_id,
                    iter_data.index_id,
                    iter_data.end_time - iter_data.start_time,
                    iter_data.overlap_time,
                ]
            )

    def save(self: any) -> None:
        if not self._hccl_overlap_time_data:
            return
        if not self._iter_compute_time_data:
            return
        with ParallelModel(self._project_path) as _model:
            _model.flush(DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP, self._hccl_overlap_time_data)
            _model.flush(DBNameConstant.TABLE_COMPUTATION_TIME, self._iter_compute_time_data)

    def _prepare_for_parse(self: any) -> bool:
        with ClusterHCCLViewModel(self._project_path) as _model:
            self._hccl_op_data = _model.get_hccl_op_data()
        if not self._hccl_op_data:
            logging.error("Invalid hccl op data from ts_track!")
            return False
        with HwtsIterViewModel(self._project_path) as _model:
            ai_core_op_data = _model.get_ai_core_op_data()
        with TsTrackViewModel(self._project_path) as _model:
            ai_cpu_data = _model.get_ai_cpu_data()
            self._iter_time_data = _model.get_iter_time_data()
        ai_cpu_op_data = AICpuFromTsCalculator.state_to_timeline(ai_cpu_data)
        if not ai_core_op_data and not ai_cpu_op_data:
            logging.error("Invalid compute op data from hwts and ts_track!")
            return False
        if not self._iter_time_data:
            logging.error("Invalid step trace data from ts_track!")
            return False
        self._merged_compute_op_data = SectionCalculator.merge_continuous_intervals(ai_core_op_data + ai_cpu_op_data)
        return True
