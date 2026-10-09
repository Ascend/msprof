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
from msmodel.stars.ffts_log_model import FftsLogModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.ffts_log import FftsLogDecoder
from profiling_bean.stars.ffts_plus_log import FftsPlusLogDecoder


class FftsLogParser(IStarsParser):
    """
    class used to parse ffts thread log type data"
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = FftsLogModel(result_dir, db, table_list)
        self._decoder = self._get_log_decoder()

    @property
    def decoder(self: any) -> any:
        """
        get decoder
        :return: class decoder
        """
        return self._decoder

    @classmethod
    def _get_log_decoder(cls: any) -> any:
        if ChipManager().is_ffts_plus_type():
            return FftsPlusLogDecoder
        return FftsLogDecoder

    @abstractmethod
    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
