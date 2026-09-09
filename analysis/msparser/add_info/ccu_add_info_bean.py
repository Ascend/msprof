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
from profiling_bean.struct_info.struct_decoder import StructDecoder
from common_func.hccl_info_common import ReduceOpType
from common_func.hccl_info_common import DataType


class CCUAddInfoBean(StructDecoder):
    """
    ccu channel data bean for the data parsing by ccu task info parser
    """

    def __init__(self: any) -> None:
        self._version = None
        self._work_flow_mode = None
        self._item_id = None
        self._group_name = None
        self._rank_id = None
        self._rank_size = None
        self._stream_id = None
        self._task_id = None
        self._die_id = None
        self._mission_id = None
        self._instr_id = None

    @property
    def version(self: any) -> int:
        return self._version

    @property
    def work_flow_mode(self: any) -> int:
        return self._work_flow_mode

    @property
    def item_id(self: any) -> str:
        return self._item_id

    @property
    def group_name(self: any) -> str:
        return self._group_name

    @property
    def rank_size(self: any) -> int:
        return self._rank_size

    @property
    def rank_id(self: any) -> int:
        return self._rank_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def die_id(self: any) -> int:
        return self._die_id

    @property
    def mission_id(self: any) -> int:
        return self._mission_id

    @property
    def instr_id(self: any) -> int:
        return self._instr_id

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the CCU add info data
        :param args: CCU Task Info bin data
        :return: True or False
        """
        pass


class CCUTaskInfoBean(CCUAddInfoBean):
    """
    ccu channel data bean for the data parsing by ccu task info parser
    """

    def decode(self: any, bin_data: any, **kwargs) -> any:
        """
        decode the CCU Task Info bin data
        :param bin_data: CCU Task Info bin data
        :return: instance of CCU Task Info
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_TASK_INFO_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return None

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the CCU Task Info data
        :param args: CCU Task Info bin data
        :return: True or False
        """
        task_info_data = args[0]
        data_lens = 209
        if task_info_data and len(task_info_data) == data_lens:
            self._version = task_info_data[6]
            self._work_flow_mode = task_info_data[7]
            self._item_id = str(task_info_data[11])
            self._group_name = str(task_info_data[12])
            self._rank_id = task_info_data[13]
            self._rank_size = task_info_data[14]
            self._stream_id = task_info_data[15]
            self._task_id = task_info_data[17]
            self._die_id = task_info_data[18]
            self._mission_id = task_info_data[19]
            self._instr_id = task_info_data[20]
            return True
        logging.error("Ccu task info data struct is incomplete, please check the file.")
        return False


class CCUWaitSignalInfoBean(CCUAddInfoBean):
    """
    ccu channel data bean for the data parsing by ccu wait signal info parser
    """

    def __init__(self: any) -> None:
        super().__init__()
        self._cke_id = None
        self._mask = None
        self._channel_id = []
        self._remote_rank_id = []

    @property
    def cke_id(self) -> int:
        return self._cke_id

    @property
    def mask(self) -> int:
        return self._mask

    @property
    def channel_id(self) -> list:
        return self._channel_id

    @property
    def remote_rank_id(self) -> list:
        return self._remote_rank_id

    def decode(self: any, bin_data: any, **kwargs) -> any:
        """
        decode the CCU Wait Signal Info bin data
        :param bin_data: CCU Wait Signal Info bin data
        :return: instance of CCU Wait Signal Info
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_WAIT_SIGNAL_INFO_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return None

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the CCU Wait Signal Info data
        :param args: CCU Wait Signal Info bin data
        :return: True or False
        """
        wait_signal_info_data = args[0]
        data_lens = 140
        if wait_signal_info_data and len(wait_signal_info_data) == data_lens:
            self._version = wait_signal_info_data[6]
            self._item_id = str(wait_signal_info_data[11])
            self._group_name = str(wait_signal_info_data[12])
            self._rank_id = wait_signal_info_data[13]
            self._rank_size = wait_signal_info_data[14]
            self._work_flow_mode = wait_signal_info_data[15]
            self._stream_id = wait_signal_info_data[17]
            self._task_id = wait_signal_info_data[18]
            self._die_id = wait_signal_info_data[19]
            self._instr_id = wait_signal_info_data[21]
            self._mission_id = wait_signal_info_data[22]
            self._cke_id = wait_signal_info_data[26]
            self._mask = wait_signal_info_data[27]
            for i in range(16):
                self._channel_id.append(wait_signal_info_data[28 + i])
                self._remote_rank_id.append(wait_signal_info_data[44 + i])
            return True
        logging.error("Ccu wait signal info data struct is incomplete, please check the file.")
        return False


class CCUGroupInfoBean(CCUAddInfoBean):
    """
    ccu channel data bean for the data parsing by ccu group info parser
    """

    def __init__(self: any) -> None:
        super().__init__()
        self._reduce_op_type = None
        self._input_data_type = None
        self._output_data_type = None
        self._data_size = None
        self._channel_id = []
        self._remote_rank_id = []

    @property
    def reduce_op_type(self) -> str:
        try:
            reduce_op_type = ReduceOpType(self._reduce_op_type).name
        except ValueError:
            logging.warning("Invalid ccu reduce op type, type enum is %s, please check!", self._reduce_op_type)
            reduce_op_type = self._reduce_op_type
        return reduce_op_type

    @property
    def input_data_type(self) -> str:
        try:
            input_data_type = DataType(self._input_data_type).name
        except ValueError:
            logging.warning("Invalid ccu input data type, type enum is %s, please check!", self._input_data_type)
            input_data_type = self._input_data_type
        return input_data_type

    @property
    def output_data_type(self) -> str:
        try:
            output_data_type = DataType(self._output_data_type).name
        except ValueError:
            logging.warning("Invalid ccu output data type, type enum is %s, please check!", self._output_data_type)
            output_data_type = self._output_data_type
        return output_data_type

    @property
    def data_size(self) -> int:
        return self._data_size

    @property
    def channel_id(self) -> list:
        return self._channel_id

    @property
    def remote_rank_id(self) -> list:
        return self._remote_rank_id

    def decode(self: any, bin_data: any, **kwargs) -> any:
        """
        decode the CCU Group Info bin data
        :param bin_data: CCU Group Info bin data
        :return: instance of CCU Group Info
        """
        args = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.CCU_GROUP_INFO_FMT, bin_data)
        if self.construct_bean(args):
            return self
        return None

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the CCU Group Info data
        :param args: CCU Group Info bin data
        :return: True or False
        """
        group_info_data = args[0]
        data_lens = 139
        if group_info_data and len(group_info_data) == data_lens:
            self._version = group_info_data[6]
            self._item_id = str(group_info_data[11])
            self._group_name = str(group_info_data[12])
            self._rank_id = group_info_data[13]
            self._rank_size = group_info_data[14]
            self._work_flow_mode = group_info_data[15]
            self._stream_id = group_info_data[17]
            self._task_id = group_info_data[18]
            self._die_id = group_info_data[19]
            self._instr_id = group_info_data[21]
            self._mission_id = group_info_data[22]
            self._reduce_op_type = group_info_data[23]
            self._input_data_type = group_info_data[24]
            self._output_data_type = group_info_data[25]
            self._data_size = group_info_data[26]
            for i in range(16):
                self._channel_id.append(group_info_data[27 + i])
                self._remote_rank_id.append(group_info_data[43 + i])
            return True
        logging.error("Ccu group info data struct is incomplete, please check the file.")
        return False
