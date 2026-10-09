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

from common_func.common import error
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import MsProfCommonConstant
from msmodel.cluster_info.cluster_info_model import ClusterInfoModel
from msparser.interface.iparser import IParser


class ClusterInfoParser(IParser):
    """
    parser of rank_id data
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, collect_path: str, device_cluster_basic_info: dict) -> None:
        self.collect_path = collect_path
        self.device_cluster_basic_info = device_cluster_basic_info
        self.cluster_info_list = []

    def ms_run(self: any):
        self.parse()
        self.save()
        logging.info("cluster_rank.db created successful!")

    def parse(self: any) -> None:
        logging.info("Start to parse cluster rank data!")
        for dir_name, cluster_basic_info in self.device_cluster_basic_info.items():
            cluster_info = [
                cluster_basic_info.job_info,
                cluster_basic_info.device_id,
                cluster_basic_info.collection_time,
                cluster_basic_info.rank_id,
                dir_name,
            ]
            self.cluster_info_list.append(cluster_info)

    def save(self: any) -> None:
        logging.info("Starting to save cluster_rank data to db!")
        if not self.cluster_info_list:
            error(MsProfCommonConstant.COMMON_FILE_NAME, 'No valid cluster data in the dir(%s).', self.collect_path)
            return
        self.cluster_info_list.sort(key=lambda x: str(x[3]))
        with ClusterInfoModel(self.collect_path) as cluster_info_model:
            cluster_info_model.flush(self.cluster_info_list)


class ClusterBasicInfo:
    def __init__(self: any, collection_path: str):
        self.collection_path = collection_path
        self._is_host_profiling = Constant.NA
        self._job_info = Constant.NA
        self._device_id = Constant.NA
        self._collection_time = Constant.NA
        self._rank_id = Constant.DEFAULT_INVALID_VALUE

    @property
    def is_host_profiling(self: any) -> bool:
        return self._is_host_profiling

    @property
    def job_info(self: any) -> str:
        return self._job_info

    @property
    def device_id(self: any) -> str:
        return self._device_id

    @property
    def collection_time(self: any) -> str:
        return self._collection_time

    @property
    def rank_id(self: any) -> str:
        return self._rank_id

    def init(self: any) -> None:
        InfoConfReader().load_info(self.collection_path)
        self._is_host_profiling = InfoConfReader().is_host_profiling()
        self._job_info = InfoConfReader().get_job_info()
        self._device_id = InfoConfReader().get_device_id()
        self._collection_time = InfoConfReader().get_collect_start_time()
        self._rank_id = InfoConfReader().get_rank_id()
