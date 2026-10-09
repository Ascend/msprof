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
import struct

from msparser.data_struct_size_constant import StructFmt
from msparser.interface.idata_bean import IDataBean
from profiling_bean.struct_info.lpm_info_struct import LpmData


class FreqLpmConvBean(IDataBean):
    """
    Frequency bean data for the data parsing by freq parser.
    """

    COUNT_INDEX = 0
    SYSCNT_BEGIN_INDEX = 2
    FREQ_BEGIN_INDEX = 3
    INTERVAL = 3
    FREQ_DATA_NUM = len(StructFmt.FREQ_FMT)

    def __init__(self: any) -> None:
        self._count = 0
        self._lpm_data = []

    @property
    def count(self: any) -> int:
        """
        lpm data report number
        """
        return self._count

    @property
    def lpm_data(self: any) -> list:
        """
        lpm data
        """
        return self._lpm_data

    def decode(self: any, bin_data: bytes) -> None:
        """
        decode the freq bin data
        :param bin_data: freq bin data
        :return: instance of freq
        """
        if not self.construct_bean(struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.FREQ_FMT, bin_data)):
            logging.error("freq data struct is incomplete, please check the freq file.")

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the freq data
        :param args: freq data
        :return: True or False
        """
        freq_data = args[0]
        if len(freq_data) != self.FREQ_DATA_NUM:
            return False
        self._count = freq_data[self.COUNT_INDEX]
        self._lpm_data = []
        for idx in range(self.count):
            syscnt = freq_data[self.SYSCNT_BEGIN_INDEX + self.INTERVAL * idx]
            freq = freq_data[self.FREQ_BEGIN_INDEX + self.INTERVAL * idx]
            self._lpm_data.append(LpmData(syscnt, freq))
        return True
