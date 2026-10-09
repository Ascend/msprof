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

from abc import abstractmethod

from msmodel.stars.acc_pmu_model import AccPmuModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.acc_pmu import AccPmuDecoder


class AccPmuParser(IStarsParser):
    """
    class used to parse acc_pmu type data"
    """

    SAMPLE_BASED = 'sample_based'

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = AccPmuModel(result_dir, db, table_list)
        self._decoder = AccPmuDecoder

    @property
    def decoder(self: any) -> any:
        """
        get decoder
        :return: class decoder
        """
        return self._decoder

    @abstractmethod
    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
