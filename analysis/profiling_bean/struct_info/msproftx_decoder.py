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


from profiling_bean.struct_info.struct_decoder import StructDecoder


class MsprofTxDecoder(StructDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: list) -> None:
        filed = args[0]
        self._magic = filed[0]
        self._info_type = filed[6]
        self._seg_idx = filed[7]
        self._data_tag = filed[10]
        self._process_id = filed[11]
        self._thread_id = filed[12]
        self._category = filed[13]
        self._event_type = filed[14]
        self._payload_type = filed[15]
        self._payload_value = filed[16]
        self._start_time = filed[17]
        self._end_time = filed[18]
        self._mark_id = filed[19]
        self._domain = filed[20]
        self._message_type = filed[21]
        self._message = self.decode_byte(filed[22])

    @property
    def magic(self: any) -> int:
        return self._magic

    @property
    def info_type(self: any) -> int:
        return self._info_type

    @property
    def seg_idx(self: any) -> int:
        return self._seg_idx

    @property
    def data_tag(self: any) -> int:
        return self._data_tag

    @property
    def process_id(self: any) -> int:
        return self._process_id

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @property
    def category(self: any) -> int:
        return self._category

    @property
    def event_type(self: any) -> int:
        return self._event_type

    @property
    def payload_type(self: any) -> int:
        return self._payload_type

    @property
    def payload_value(self: any) -> int:
        return self._payload_value

    @property
    def start_time(self: any) -> int:
        return self._start_time

    @property
    def end_time(self: any) -> int:
        return self._end_time

    @property
    def mark_id(self) -> int:
        return self._mark_id

    @property
    def domain(self) -> int:
        return self._domain

    @property
    def message_type(self: any) -> int:
        return self._message_type

    @property
    def message(self: any) -> str:
        return self._message

    def decode_byte(self: any, value: str) -> str:
        zero_ind = value.find(b'\x00')
        str_value = bytes.decode(value[0:zero_ind])
        return str_value
