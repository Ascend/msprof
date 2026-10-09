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

import json
import logging
import os
from json import JSONDecodeError

from common_func.common import error
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.file_manager import check_file_readable
from common_func.path_manager import PathManager
from msmodel.parallel.parallel_model import ParallelModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class ParallelStrategyParser(IParser, MsMultiProcess):
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parallel_strategy_data = []

    @classmethod
    def _get_parallel_mode(cls: any, parallel_type: str, stage_num: int) -> str:
        if parallel_type == StrConstant.STAND_ALONE:
            return StrConstant.STAND_ALONE
        stage_num = 1 if not stage_num else stage_num
        if stage_num > 1:
            return StrConstant.PIPELINE_PARALLEL
        if not parallel_type or parallel_type == StrConstant.DATA_PARALLEL:
            return StrConstant.DATA_PARALLEL
        return StrConstant.MODEL_PARALLEL

    def ms_run(self) -> None:
        parallel_files = self._file_list.get(DataTag.PARALLEL_STRATEGY, [])
        if not parallel_files:
            return
        logging.info("Start to parse parallel strategy data!")
        self.parse(parallel_files)
        self.save()

    def parse(self: any, parallel_files: list) -> None:
        parallel_data = ""
        for _parallel_file in parallel_files:
            parallel_file = PathManager.get_data_file_path(self._project_path, _parallel_file)
            check_file_readable(parallel_file)
            with FileOpen(parallel_file, 'rt') as _file:
                parallel_data = parallel_data + _file.file_reader.read()
        try:
            parallel_data = json.loads(parallel_data).get("config", {})
        except JSONDecodeError:
            error(self.FILE_NAME, "Invalid parallel strategy data.")
            return
        if not parallel_data:
            error(self.FILE_NAME, "Invalid parallel strategy data.")
            return
        parallel_mode = self._get_parallel_mode(parallel_data.get("parallelType"), parallel_data.get("stage_num"))
        self._parallel_strategy_data.append(
            [
                parallel_data.get("ai_framework_type"),
                parallel_data.get("stage_num"),
                parallel_data.get("rankId"),
                parallel_data.get("stageId"),
                parallel_data.get("parallelType"),
                str(parallel_data.get("stageDevices")),
                parallel_mode,
            ]
        )

    def save(self: any) -> None:
        if not self._parallel_strategy_data:
            return
        with ParallelModel(self._project_path) as _model:
            _model.flush(DBNameConstant.TABLE_PARALLEL_STRATEGY, self._parallel_strategy_data)
