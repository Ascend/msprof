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
from profiling_bean.struct_info.struct_decoder import StructDecoder


class LowPowerBean(StructDecoder):
    """
    bean for lowpower sample data
    """

    def __init__(self: any, *args: tuple) -> None:
        field = args[0]
        self._die_id = field[0] >> 10  # 第二个字节的高六位是dieid
        self._sys_cnt = field[3]
        self._lp_sample_info = field[5:]

    @property
    def die_id(self: any) -> int:
        return self._die_id

    @property
    def timestamp(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)

    @property
    def lp_sample_info(self: any) -> list:
        """
        原始数据结构第一个数据是31::16,，第二个数据是15:0，以此类推，需要变换一下顺序
        """
        lp_sample_info = self._lp_sample_info
        result = [lp_sample_info[i + 1] if i % 2 == 0 else lp_sample_info[i - 1] for i in range(len(lp_sample_info))]
        return result
