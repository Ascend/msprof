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
import struct

from msparser.data_struct_size_constant import StructFmt
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CCUMissionBean(StructDecoder):
    """
    ccu mission data bean for the data parsing by ccu mission parser
    """

    def __init__(self: any, relative_time_scale: int = 4) -> None:
        self._stream_id = None
        self._task_id = None
        self._lp_instr_id = None
        self._lp_start_time = None
        self._lp_end_time = None
        self._setckebit_instr_id = None
        self._setckebit_start_time = None
        self._rel_end_time = []
        self._relative_time_scale = relative_time_scale

    @property
    def stream_id(self: any) -> str:
        return self._stream_id

    @property
    def task_id(self: any) -> str:
        return self._task_id

    @property
    def lp_instr_id(self: any) -> str:
        return self._lp_instr_id

    @property
    def lp_start_time(self: any) -> str:
        return self._lp_start_time

    @property
    def lp_end_time(self: any) -> str:
        return self._lp_end_time

    @property
    def setckebit_instr_id(self: any) -> str:
        return self._setckebit_instr_id

    @property
    def setckebit_start_time(self: any) -> str:
        return self._setckebit_start_time

    @property
    def rel_end_time(self: any) -> list:
        return self._rel_end_time

    def decode(self: any, bin_data: any) -> any:
        """
        decode the ccu mission bin data
        :param bin_data: ccu mission bin data
        :return: instance of ccu mission
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_MISSION_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        """
        refresh the ccu mission data
        :param args: ccu mission bin data
        :return: True or False
        """
        mission_data = args[0]
        data_lens = 23
        if mission_data and len(mission_data) == data_lens:
            self._stream_id = mission_data[21]
            self._task_id = StarsCommon.set_unique_task_id(mission_data[21], mission_data[22])
            self._lp_instr_id = mission_data[20]
            self._lp_start_time = mission_data[19]
            self._lp_end_time = mission_data[18]
            self._setckebit_instr_id = mission_data[17]
            setckebit_start_syscnt = mission_data[16]
            self._setckebit_start_time = setckebit_start_syscnt
            for i in range(0, 16):
                if setckebit_start_syscnt != 0:
                    device_id = 15 - i
                    end_time = setckebit_start_syscnt + mission_data[i] * self._relative_time_scale
                    self._rel_end_time.append((device_id, end_time))
            return True
        logging.error("CCU mission data struct is incomplete, please check the file.")
        return False


class CCUChannelBean(StructDecoder):
    """
    ccu channel data bean for the data parsing by ccu channel parser
    """

    def __init__(self: any) -> None:
        self._channels_bw_data = []

    @property
    def channels_bw_data(self: any) -> list:
        return self._channels_bw_data

    def decode(self: any, bin_data: any) -> any:
        """
        decode the ccu channel bin data
        :param bin_data: ccu channel bin data
        :return: instance of ccu channel
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_CHANNEL_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        """
        refresh the ccu channel data
        :param args: ccu channel bin data
        :return: True or False
        """
        channel_data = args[0]
        data_lens = 704
        if channel_data and len(channel_data) == data_lens:
            channel_id = 0
            for i in self.get_channel_range():
                if channel_id in (120, 124):
                    # channel 120,124中data[0]~data[5]为保留字段
                    syscnt = (channel_data[i + 7] << 32) + channel_data[i + 6]
                    avg_bw = channel_data[i + 8]
                    min_bw = channel_data[i + 9]
                    max_bw = channel_data[i + 10]
                else:
                    syscnt = (channel_data[i + 1] << 32) + channel_data[i]
                    avg_bw = channel_data[i + 2]
                    min_bw = channel_data[i + 3]
                    max_bw = channel_data[i + 4]
                self._channels_bw_data.append([channel_id, syscnt, max_bw, min_bw, avg_bw])
                channel_id = channel_id + 1
            return True
        logging.error("ccu channel data struct is incomplete, please check the file.")
        return False

    def get_channel_range(self):
        """
        获取channel分割的range，上报数据格式channel分割步长不固定，每个step为一个channel的长度
        """
        # 分割步长不固定，步长为 ([5] * 11 + [9]) * 10 + ([11] + [5] * 2 + [11]) * 2
        bin_intervals = ([5] * 11 + [9]) * 10 + ([11] + [5] * 2 + [11]) * 2
        sequence = [0]
        value = 0
        for step in bin_intervals:
            value += step
            sequence.append(value)
        # 最后一个序号是最终长度，不作为起始坐标返回
        sequence.pop()
        return sequence
