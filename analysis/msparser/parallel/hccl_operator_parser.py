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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.ge.ge_hash_model import GeHashViewModel
from msmodel.parallel.cluster_hccl_model import ClusterHCCLModel
from msmodel.parallel.parallel_model import ParallelViewModel
from msmodel.step_trace.ts_track_model import TsTrackViewModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class HCCLOperatorParser(IParser, MsMultiProcess):
    END_TAG = 1

    def __init__(self: any, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_operator_data = []

    def ms_run(self: any) -> None:
        if not self._file_list.get(DataTag.PARALLEL_STRATEGY, []):
            return
        with ParallelViewModel(self._project_path) as _model:
            if _model.get_parallel_table_name() == Constant.NA:
                return
        logging.info("Start to parse hccl operator data from ts_track!")
        self.parse()
        self.save()

    def parse(self: any) -> None:
        with TsTrackViewModel(self._project_path) as _model:
            hccl_data_list = _model.get_hccl_operator_exe_data()
        with GeHashViewModel(self._project_path) as _model:
            ge_hash_dict = _model.get_ge_hash_data()
        hccl_start_data = None
        for hccl_data in hccl_data_list:
            if hccl_data.tag_id % 2 == self.END_TAG:
                hash_value = ge_hash_dict.get(hccl_data.op_name, None)
                op_name = hash_value if hash_value else hccl_data.op_name
                start_time = NumberConstant.INVALID_OP_EXE_TIME if not hccl_start_data else hccl_start_data.timestamp
                self._hccl_operator_data.append(
                    [
                        hccl_data.model_id,
                        hccl_data.index_id,
                        op_name,
                        hccl_data.op_type,
                        start_time,
                        hccl_data.timestamp,
                    ]
                )
                hccl_start_data = None
            elif not hccl_start_data:
                hccl_start_data = hccl_data
            else:
                self._hccl_operator_data.append(
                    [
                        hccl_start_data.model_id,
                        hccl_start_data.index_id,
                        "",
                        "",
                        hccl_start_data.timestamp,
                        NumberConstant.INVALID_OP_EXE_TIME,
                    ]
                )
                hccl_start_data = hccl_data

    def save(self: any) -> None:
        if not self._hccl_operator_data:
            logging.error('No valid hccl operator exe data.')
            return
        with ClusterHCCLModel(self._project_path) as _model:
            _model.flush(self._hccl_operator_data, DBNameConstant.TABLE_HCCL_OPERATOR_EXE)
