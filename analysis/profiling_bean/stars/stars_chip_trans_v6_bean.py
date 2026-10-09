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

from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StarsChipTransV6Bean(StructDecoder):
    """
    bean for stars chip trans v6
    """

    def __init__(self: any, *args: any) -> None:
        args = args[0]
        # total 16 bit, get lower 6 bit
        self._func_type = Utils.get_func_type(args[0])
        # total 16 bit, get 6 to 9 bit
        self._cnt = bin(args[0] & 960)[2:6].zfill(6)
        # total 16 bit, get high 6 bit
        self._die_id = args[0] >> 10
        self._sys_cnt = args[3]
        self._write_bandwidth = args[4]
        self._read_bandwidth = args[5]

    @property
    def func_type(self: any) -> str:
        """
        for func type
        """
        return self._func_type

    @property
    def die_id(self: any) -> int:
        """
        for die id
        """
        return self._die_id

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self.sys_cnt

    @property
    def sys_time(self: any) -> float:
        """
        for sys time
        """
        sys_time = InfoConfReader().time_from_syscnt(self._sys_cnt)
        return sys_time

    @property
    def pcie_write_bw(self: any) -> int:
        """
        PCIE write bandwidth(DMA local2remote)
        """
        return self._write_bandwidth

    @property
    def pcie_read_bw(self: any) -> any:
        """
        PCIE write bandwidth(DMA local2remote)
        """
        return self._read_bandwidth
