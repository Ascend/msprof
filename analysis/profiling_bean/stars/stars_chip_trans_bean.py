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

from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StarsChipTransBean(StructDecoder):
    """
    bean for stars soc data
    """

    def __init__(self: any, *args) -> None:
        self.filed = args[0]
        self._func_type = Utils.get_func_type(self.filed[0])
        self._cnt = Utils.get_cnt(self.filed[0])
        self._data_tag = self.filed[1]
        self._sys_cnt = self.filed[3]
        self._event_id = self.filed[5]
        self._value_1 = self.filed[9]
        self._value_2 = self.filed[10]

    @property
    def func_type(self: any) -> str:
        """
        for func type
        """
        return self._func_type

    @property
    def acc_type(self: any) -> str:
        """
        for acc type
        l2 buffer bw level is the higher 6 bits of the byte.
        64512 is 0b1111110000000000 keep the higher 6 bits which represent acc_type
        """
        return bin(self.filed[0] & 64512)[2:8].zfill(6)

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self._sys_cnt

    @property
    def sys_time(self: any) -> float:
        """
        for sys time
        """
        sys_time = InfoConfReader().time_from_syscnt(self._sys_cnt)
        return sys_time

    @property
    def event_id(self: any) -> float:
        """
        for event id
        mata bw level is the lower 6 bits of the byte.
        63 is 0b111111 keep lower 6 bits which represent PA link ID/PCIE ID
        """
        return self._event_id & 63

    @property
    def pa_rx_or_pcie_write_bw(self: any) -> int:
        """
        for PA link traffic monit rx/PCIE write bandwidth(DMA local2remote)
        mata bw level is the lower 6 bits of the byte.
        7 is 0b111 keep lower 3 bits which represent mata bw level
        """
        return self._value_1

    @property
    def pa_tx_or_pcie_read_bw(self: any) -> any:
        """
        for PA link traffic monit tx/PCIE read bandwidth (DMA remote2local)
        l2 buffer bw level is the 4 - 11 bits of the byte.
        3040 is 0b11111111000 keep 7 - 10 bits which represent cnt
        """
        return self._value_2
