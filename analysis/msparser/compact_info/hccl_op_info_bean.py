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

# pylint: disable=duplicate-code

from msparser.compact_info.compact_info_bean import CompactInfoBean


class HcclOpInfoBean(CompactInfoBean):
    """
    Hccl op Info Bean
    """

    RELAY_FLAG_BIT = 0
    RETRY_FLAG_BIT = 1
    ALG_TYPE_PHASE_CNT = 4
    ALG_TYPE_BIT_CNT = 4
    ALG_TYPE_BIT_MASK = 0b1111

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._relay = (data[6] >> self.RELAY_FLAG_BIT) & 0x1
        self._retry = (data[6] >> self.RETRY_FLAG_BIT) & 0x1
        self._data_type = data[7]
        self._alg_type = data[8]
        self._count = data[9]
        self._group_name = data[10]

    @property
    def relay(self: any) -> int:
        """
        for relay flag
        """
        return self._relay

    @property
    def retry(self: any) -> int:
        """
        for retry flag
        """
        return self._retry

    @property
    def data_type(self: any) -> str:
        """
        for data type
        """
        return str(self._data_type)

    @property
    def alg_type(self: any) -> str:
        """
        for hccl op alg type
        """
        return str(self._alg_type)

    @property
    def count(self: any) -> int:
        """
        for hccl op transfer data count
        """
        return self._count

    @property
    def group_name(self: any) -> str:
        """
        hash number for hccl group
        """
        return str(self._group_name)

    @property
    def rank_size(self: any) -> int:
        return self._rank_size
