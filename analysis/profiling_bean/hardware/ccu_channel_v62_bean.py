# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
import struct

from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CCUChannelV62Bean(StructDecoder):
    """
    ccu channel data bean for chip v6_2 data parsing.
    """

    WORD_COUNT = 96
    CHANNEL_COUNT = 32
    VALID_WORD_COUNT = 78
    SYSCNT_RESTORE_SCALE = 4  # Hardware stores syscnt right-shifted by two bits.
    CHANNEL_ID_START_INDEX = 0
    CHANNEL_ID_END_INDEX = 10
    CHANNEL_VID_INDEX = 11
    SYSCNT_LOW_INDEX = 12
    SYSCNT_HIGH_INDEX = 13
    METRIC_START_INDEX = 14

    def __init__(self: any) -> None:
        self._channels_delay_data = []
        self._filtered_channel_count = 0

    @property
    def channels_delay_data(self: any) -> list:
        return self._channels_delay_data

    @property
    def filtered_channel_count(self: any) -> int:
        return self._filtered_channel_count

    @staticmethod
    def _get_bits(value: int, high: int, low: int) -> int:
        return (value >> low) & ((1 << (high - low + 1)) - 1)

    @staticmethod
    def _merge_u64(low_word: int, high_word: int) -> int:
        return (high_word << 32) | low_word

    @staticmethod
    def _is_channel_valid(channel_vid: int, channel_index: int) -> bool:
        return ((channel_vid >> channel_index) & 0x1) == 1

    def decode(self: any, bin_data: any) -> any:
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_CHANNEL_V62_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        channel_data = args[0]
        if not channel_data or len(channel_data) != self.WORD_COUNT:
            logging.error("CCU channel v6_2 data struct is incomplete, please check the file.")
            return False
        channel_ids = self._parse_channel_ids(channel_data)
        channel_vid = channel_data[self.CHANNEL_VID_INDEX]
        syscnt = str(
            self._merge_u64(channel_data[self.SYSCNT_LOW_INDEX], channel_data[self.SYSCNT_HIGH_INDEX])
            * self.SYSCNT_RESTORE_SCALE
        )
        for channel_index, channel_id in enumerate(channel_ids):
            if not self._is_channel_valid(channel_vid, channel_index):
                self._filtered_channel_count += 1
                continue
            avg_delay = channel_data[self.METRIC_START_INDEX + channel_index * 2]
            max_delay = channel_data[self.METRIC_START_INDEX + channel_index * 2 + 1]
            self._channels_delay_data.append([channel_id, syscnt, max_delay, avg_delay])
        return True

    def _parse_channel_ids(self: any, channel_data: tuple) -> list:
        channel_ids = []
        for idx in range(self.CHANNEL_ID_START_INDEX, self.CHANNEL_ID_END_INDEX):
            word = channel_data[idx]
            channel_ids.append(self._get_bits(word, 9, 0))
            channel_ids.append(self._get_bits(word, 19, 10))
            channel_ids.append(self._get_bits(word, 29, 20))
        last_word = channel_data[self.CHANNEL_ID_END_INDEX]
        channel_ids.append(self._get_bits(last_word, 9, 0))
        channel_ids.append(self._get_bits(last_word, 19, 10))
        return channel_ids
