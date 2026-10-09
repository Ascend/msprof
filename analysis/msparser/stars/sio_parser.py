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

from common_func.platform.chip_manager import ChipManager
from msmodel.stars.sio_model import SioModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.sio_bean import SioDecoderImpl
from profiling_bean.stars.sio_bean import SioDecoderV6


class SioParser(IStarsParser):
    """
    class used to parse sio data"
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = SioModel(result_dir, db, table_list)
        self._decoder = SioDecoderV6 if ChipManager().is_chip_v6() else SioDecoderImpl
        self._data_list = []

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
        sio_data = []
        for _data_bean in self._data_list:
            bandwidth = _data_bean.data_size
            sio_data.append([_data_bean.acc_id, *bandwidth, _data_bean.timestamp])
        self._data_list = sio_data
