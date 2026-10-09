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

from common_func.decode_tool.decode_tool import DecodeTool
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FlowBean(StructDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        self._stat_rcmd_num = filed[0]
        self._stat_wcmd_num = filed[1]
        self._stat_rlat_raw = filed[2]
        self._stat_wlat_raw = filed[3]
        self._stat_flux_rd = filed[4]
        self._stat_flux_wr = filed[5]
        self._stat_flux_rd_l2 = filed[6]
        self._stat_flux_wr_l2 = filed[7]
        self._timestamp = filed[8]
        self._l2_cache_hit = filed[9]

    @property
    def stat_rcmd_num(self: any) -> int:
        """
        get stat rcmd num
        :return: stat rcmd num
        """
        return self._stat_rcmd_num

    @property
    def stat_wcmd_num(self: any) -> int:
        """
        get stat wcmd num
        :return: stat wcmd num
        """
        return self._stat_wcmd_num

    @property
    def stat_rlat_raw(self: any) -> int:
        """
        get stat rlat raw
        :return: stat rlat raw
        """
        return self._stat_rlat_raw

    @property
    def stat_wlat_raw(self: any) -> int:
        """
        get stat wlat raw
        :return: stat wlat raw
        """
        return self._stat_wlat_raw

    @property
    def stat_flux_rd(self: any) -> int:
        """
        get stat flux rd
        :return:  stat flux rd
        """
        return self._stat_flux_rd

    @property
    def stat_flux_wr(self: any) -> int:
        """
        get stat flux wr
        :return: stat flux wr
        """
        return self._stat_flux_wr

    @property
    def stat_flux_rd_l2(self: any) -> int:
        """
        get stat flux rd l2
        :return: stat flux rd l2
        """
        return self._stat_flux_rd_l2

    @property
    def stat_flux_wr_l2(self: any) -> int:
        """
        get stat flux wr l2
        :return: stat flux wr l2
        """
        return self._stat_flux_wr_l2

    @property
    def timestamp(self: any) -> str:
        """
        get timestamp
        :return: timestamp
        """
        return str(self._timestamp)

    @property
    def l2_cache_hit(self: any) -> int:
        """
        get L2 cache hit
        :return: L2 cache hit
        """
        return self._l2_cache_hit

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary dato to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        fmt = cls.get_fmt()
        decode_tool = DecodeTool()
        return cls((decode_tool.decode_byte(fmt, binary_data)))
