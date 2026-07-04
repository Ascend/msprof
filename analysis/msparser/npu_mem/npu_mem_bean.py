# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging
from common_func.constant import Constant
from profiling_bean.struct_info.struct_decoder import StructDecoder


class NpuMemDataBean(StructDecoder):
    """
    Npu mem data bean for the data parsing by npu mem parser
    """

    def __init__(self: any) -> None:
        self._event = None
        self._ddr = Constant.DEFAULT_VALUE
        self._hbm = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE

    @property
    def event(self: any) -> str:
        """
        event type
        :return: event type
        """
        return self._event

    @property
    def ddr(self: any) -> int:
        """
        ddr
        :return: ddr
        """
        return self._ddr

    @property
    def hbm(self: any) -> int:
        """
        hbm
        :return: hbm
        """
        return self._hbm

    @property
    def timestamp(self: any) -> int:
        """
        timestamp
        :return: timestamp
        """
        return self._timestamp

    def npu_mem_decode(self: any, record: any) -> any:
        """
        decode the npu mem data
        :param record: unpacked npu mem record
        :return: instance of npu mem
        """
        if self.construct_bean(record):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        """
        refresh the npu mem data
        :param args: unpacked npu mem data
        :return: True or False
        """
        _npu_mem_data = args[0]
        if _npu_mem_data:
            self._event = _npu_mem_data[1]
            self._ddr = _npu_mem_data[3]
            self._hbm = _npu_mem_data[4]
            self._timestamp = _npu_mem_data[0]
            return True
        logging.error("NPU mem data struct is incomplete, please check the npu mem file.")
        return False


class NpuMemDataBeanV2(NpuMemDataBean):
    """
    Npu mem data bean for collection version 2.0 parsing.
    """

    def construct_bean(self: any, *args: dict) -> bool:
        _npu_mem_data = args[0]
        if _npu_mem_data:
            self._event = _npu_mem_data[1]
            self._timestamp = _npu_mem_data[2]
            self._ddr = _npu_mem_data[3]
            self._hbm = _npu_mem_data[4]
            return True
        logging.error("NPU mem data struct is incomplete, please check the npu mem file.")
        return False
