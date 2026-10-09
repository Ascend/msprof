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


class LpmInfoDataBean(IDataBean):
    """
    lpm info data bean for the data parsing by lpm info data parser
    """

    def __init__(self: any) -> None:
        self._type = None
        self._count = None
        self._lpm_data = []

    @property
    def type(self: any) -> int:
        """
        lpm data type
        """
        return self._type

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

    def decode(self: any, bin_data: any, **kwargs) -> any:
        """
        decode the lpm info bin data
        :param bin_data: lpm info bin data
        :return: instance of lpm info
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.LPM_INFO_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return None

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the lpm info data
        :param args: lpm info bin data
        :return: True or False
        """
        lpm_info_data = args[0]
        data_length = 167
        if lpm_info_data and len(lpm_info_data) == data_length:
            self._count = lpm_info_data[0]
            self._type = lpm_info_data[1]
            for i in range(self._count):
                self._lpm_data.append(LpmData(lpm_info_data[2 + i * 3], lpm_info_data[3 + i * 3]))
            return True
        logging.error("Lpm info data struct is incomplete, please check the file.")
        return False
