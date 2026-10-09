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
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.parallel.cluster_parallel_model import ClusterParallelModel
from msmodel.parallel.parallel_model import ParallelViewModel
from msparser.interface.iparser import IParser


class ClusterParallelCollector(IParser):
    THREAD_NUM = 10

    def __init__(self: any, collect_path: str) -> None:
        self.collect_path = collect_path
        self._cluster_info = []
        self._cluster_parallel_data = []
        self._cluster_parallel_strategy_data = []
        self._parallel_table_name = Constant.NA

    def ms_run(self) -> None:
        self.parse()
        self.save()

    def parse(self: any) -> None:
        with ClusterInfoViewModel(self.collect_path) as _model:
            if _model.check_table():
                self._cluster_info = _model.get_all_cluster_rank_info()
        if not self._cluster_info:
            return

        _project_path = os.path.join(self.collect_path, self._cluster_info[0].dir_name)
        if not os.path.exists(PathManager.get_db_path(_project_path, DBNameConstant.DB_PARALLEL)):
            return
        with ParallelViewModel(_project_path) as _model:
            self._parallel_table_name = _model.get_parallel_table_name()
        if self._parallel_table_name == Constant.NA:
            return
        logging.info("Start to parse cluster parallel data!")
        self.get_device_parallel_data()

    def save(self: any) -> None:
        if not self._cluster_parallel_data or not self._cluster_parallel_strategy_data:
            logging.warning("Invalid cluster parallel data!")
            return
        with ClusterParallelModel(self.collect_path) as _model:
            _model.create_table(self._parallel_table_name)
            _model.flush(self._parallel_table_name, self._cluster_parallel_data)
            _model.create_table(DBNameConstant.TABLE_CLUSTER_PARALLEL_STRATEGY)
            _model.flush(DBNameConstant.TABLE_CLUSTER_PARALLEL_STRATEGY, self._cluster_parallel_strategy_data)

    def get_device_parallel_data(self: any):
        for cluster_info in self._cluster_info:
            _project_path = os.path.join(self.collect_path, cluster_info.dir_name)
            hwts_freq = InfoConfReader().get_freq(StrConstant.HWTS)
            with ParallelViewModel(_project_path) as _model:
                parallel_index_data = _model.get_parallel_index_data(
                    self._parallel_table_name, cluster_info.rank_id, cluster_info.device_id, hwts_freq
                )
                if parallel_index_data:
                    self._cluster_parallel_data.extend(parallel_index_data)
                parallel_strategy_data = _model.get_parallel_strategy_data()
                if parallel_strategy_data:
                    self._cluster_parallel_strategy_data.extend(parallel_strategy_data)
