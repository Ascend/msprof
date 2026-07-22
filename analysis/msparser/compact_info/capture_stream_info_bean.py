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
from msparser.compact_info.compact_info_bean import CompactInfoBean


class CaptureStreamInfoBean(CompactInfoBean):
    """
    capture stream info bean (V1)
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._capture_status = data[6]
        self._model_stream_id = data[7]
        self._original_stream_id = data[8]
        self._model_id = data[9]
        self._device_id = data[10]

    @property
    def capture_status(self: any) -> int:
        """
        capture status
        """
        return self._capture_status

    @property
    def model_stream_id(self: any) -> int:
        """
        capture physic stream id
        """
        return self._model_stream_id

    @property
    def original_stream_id(self: any) -> int:
        """
        capture ori stream id
        """
        return self._original_stream_id

    @property
    def model_id(self: any) -> int:
        """
        capture model_id from rts
        """
        return self._model_id

    @property
    def device_id(self: any) -> int:
        """
        device_id
        """
        return self._device_id


class CaptureStreamInfoV2Bean(CompactInfoBean):
    """
    capture stream info bean (V2): deviceId first, streamId u32, modelId u32
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._device_id = data[6]
        self._capture_status = data[7]
        self._stream_id = data[9]
        self._original_stream_id = data[10]
        self._model_id = data[11]

    @property
    def capture_status(self: any) -> int:
        """capture status: 0=start, 1=end"""
        return self._capture_status

    @property
    def model_stream_id(self: any) -> int:
        """capture stream id (uint32_t in V2)"""
        return self._stream_id

    @property
    def original_stream_id(self: any) -> int:
        """capture ori stream id (uint32_t in V2)"""
        return self._original_stream_id

    @property
    def model_id(self: any) -> int:
        """capture model_id (uint32_t in V2)"""
        return self._model_id

    @property
    def device_id(self: any) -> int:
        """device_id"""
        return self._device_id
