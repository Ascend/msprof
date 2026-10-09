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

from msmodel.stars.inter_soc_model import InterSocModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.inter_soc import InterSoc


class InterSocParser(IStarsParser):
    """
    class used to parse inter soc Transmission type data"
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = InterSocModel(result_dir, db, table_list)
        self._decoder = InterSoc
        self._data_list = []

    @property
    def decoder(self: any) -> any:
        """
        get decoder
        :return: class decoder
        """
        return self._decoder

    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
        inter_soc_data = []
        for _data_bean in self._data_list:
            inter_soc_data.append([_data_bean.mata_bw_level, _data_bean.l2_buffer_bw_level, _data_bean.sys_time])
        self._data_list = inter_soc_data
