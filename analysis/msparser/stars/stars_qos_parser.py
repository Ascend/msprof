#!/usr/bin/python3
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

from msmodel.hardware.qos_model import QosModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.stars_qos_bean import StarsQosBean


class StarsQosParser(IStarsParser):
    """
    class used to parse qos data
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = QosModel(result_dir, db, table_list)
        self._decoder = StarsQosBean
        self._data_list = []

    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
        self._data_list = [[data.timestamp, data.die_id] + data.qos_bw_data for data in self._data_list]
