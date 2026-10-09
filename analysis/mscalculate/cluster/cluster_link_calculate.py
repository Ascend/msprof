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

import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_number
from common_func.path_manager import PathManager
from msmodel.cluster_info.communication_model import CommunicationModel


class ClusterLinkCalculator:
    """
    class used to calculate slow link in cluster
    """

    def __init__(self: any, file_list: list) -> None:
        self._file_list = file_list
        self.slow_link_list = []
        self.result_dict = {}

    def get_cluster_link_data(self: any, link_data: dict, link_type: str) -> None:
        """
        :return:
        """
        max_value = 0
        link_dict = {}
        if link_data.get(link_type, []):
            for value in link_data.get(link_type, []):
                key = "{0}-{1}".format(value[1], value[2])
                max_value = max(max_value, value[3])
                slow_link = "Slow rank link {0}, with {1}% lower bandwidth than average.".format(key, max_value)
                link_dict.update({key: slow_link})
            self.result_dict.setdefault(link_type, []).extend(list(link_dict.values()))

    def get_slow_link_dict(self: any) -> None:
        """
        save hccl data into database
        :return: None
        """

        for link_data in self.slow_link_list:
            for link_type in Constant.LINK_TYPE_LIST:
                self.get_cluster_link_data(link_data, link_type)

    def get_single_slow_link_list(self: any) -> None:
        """
        get slow link dict in single project
        """
        for _file_path in self._file_list:
            self.slow_link_list.append(ClusterSingleLinkCalculator(_file_path).ms_run())

    def run(self: any) -> dict:
        """
        entrance for calculating ge
        :return: None
        """
        self.get_single_slow_link_list()
        self.get_slow_link_dict()
        return self.result_dict


class ClusterSingleLinkCalculator(MsMultiProcess):
    """
    class used to calculate slow link in PROF
    """

    LINK_THRESHOLD_RATIO = 20

    def __init__(self: any, project_path: str) -> None:
        super().__init__({'result_dir': project_path})
        self._project_path = project_path
        self.model = None
        self.hccl_data = []
        self.link_list = []
        self.link_dict = {}
        self.average_data = {}
        self.result_dict = {}

    def calculate_cluster_link_list(self: any, link_type: str) -> list:
        """
        select cluster link data
        :param link_type:
        :return:
        """
        cluster_link_list = []
        try:
            for link_type_data in self.link_dict.get(link_type, []):
                link_bw = round(
                    (self.average_data.get(link_type) - link_type_data[0])
                    / self.average_data.get(link_type)
                    * NumberConstant.PERCENTAGE,
                    NumberConstant.ROUND_TWO_DECIMAL,
                )
                if link_bw >= self.LINK_THRESHOLD_RATIO:
                    link_type_data.extend([link_bw])
                    cluster_link_list.append(link_type_data)
            return cluster_link_list
        except ZeroDivisionError:
            return cluster_link_list

    def get_slow_link_by_type(self: any, link_type: str) -> None:
        """
        get type link data
        :return: None
        """

        if self.link_dict.get(link_type, []):
            try:
                type_average = sum(float(i[0]) for i in self.link_dict.get(link_type)) / len(
                    self.link_dict.get(link_type)
                )
            except ZeroDivisionError:
                return
            self.average_data.setdefault(link_type, type_average)
            type_list = self.calculate_cluster_link_list(link_type)
            if type_list:
                self.result_dict.setdefault(link_type, type_list)

    def get_slow_link_data(self: any) -> None:
        """
        get cluster link data
        :return: None
        """
        for link_type in Constant.LINK_TYPE_LIST:
            self.get_slow_link_by_type(link_type)

    def get_all_link_dict(self: any) -> None:
        """
        get link dict
        :return: None
        """
        for hccl_data in self.hccl_data:
            if not is_number(hccl_data.bandwidth):
                continue
            if hccl_data.local_rank in (hccl_data.remote_rank, Constant.ILLEGAL_RANK):
                continue
            if hccl_data.transport_type in Constant.LINK_TYPE_LIST:
                self.link_dict.setdefault(hccl_data.transport_type, []).append(
                    [float(hccl_data.bandwidth), hccl_data.local_rank, hccl_data.remote_rank]
                )

    def calculate(self: any) -> None:
        """
        calculate hccl data
        :return: None
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            return
        with CommunicationModel(self._project_path) as self.model:
            self.hccl_data = self.model.get_all_communication_data()
            if not self.hccl_data:
                return
            self.get_all_link_dict()
            self.get_slow_link_data()

    def ms_run(self: any) -> dict:
        """
        entrance for calculating cluster link data
        :return: None
        """
        self.calculate()
        return self.result_dict
