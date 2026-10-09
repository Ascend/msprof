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

from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from common_func.info_conf_reader import InfoConfReader
from msparser.compact_info.stream_expand_spec_reader import StreamExpandSpecReader
from msmodel.sqe_type_map import SqeType


class StarsCommon:
    """
    class for decode binary data
    """

    STREAM_JUDGE_BIT12_OPERATOR = 0x1000
    STREAM_JUDGE_BIT13_OPERATOR = 0x2000
    STREAM_JUDGE_BIT15_OPERATOR = 0x8000
    TASK_LOW_OPERATOR = 0x1FFF
    STREAM_HIGH_OPERATOR = 0xE000
    COMMON_LOW_OPERATOR = 0x0FFF
    COMMON_HIGH_OPERATOR = 0xF000
    EXPANDING_LOW_OPERATOR = 0x7FFF

    def __init__(
        self: any,
        task_id: int,
        stream_id: int,
        timestamp: int or float,
        sqe_type: SqeType.StarsSqeType = SqeType.StarsSqeType.AI_CORE,
    ) -> None:
        self._stream_id = self.set_stream_id(stream_id, task_id, sqe_type)
        self._task_id = self.set_task_id(stream_id, task_id, sqe_type)
        self._timestamp = timestamp

    @property
    def task_id(self: any) -> int:
        """
        get task id
        :return: task id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def timestamp(self: any) -> float:
        """
        get timestamp
        :return: timestamp
        """
        return InfoConfReader().time_from_syscnt(self._timestamp)

    @classmethod
    def set_stream_id(cls, stream_id, task_id, sqe_type=SqeType.StarsSqeType.AI_CORE):
        """
        In the FFTS scenario, stream ID composition varies based on stream ID bits and SQE type:

        When stream expansion is enabled:
        - For PLACE_HOLDER_SQE and EVENT_RECORD_SQE types, return low bits of stream_id
          (using EXPANDING_LOW_OPERATOR mask)
        - If bit15 of stream_id is set, return low bits of task_id
          (using EXPANDING_LOW_OPERATOR mask)
        - Otherwise, return low bits of stream_id
          (using EXPANDING_LOW_OPERATOR mask)

        When stream expansion is disabled:
        - If bit12 of stream_id is set, return processed stream_id via Utils.get_stream_id()
        - If bit13 of stream_id is set, replace stream_id with low bits of task_id
          (using COMMON_LOW_OPERATOR mask), then return processed result via Utils.get_stream_id()
        - Otherwise, return processed stream_id via Utils.get_stream_id()
        """
        if ChipManager().is_chip_v6():
            return stream_id
        if StreamExpandSpecReader().is_stream_expand:
            if sqe_type in [SqeType.StarsSqeType.PLACE_HOLDER_SQE, SqeType.StarsSqeType.EVENT_RECORD_SQE]:
                return stream_id & cls.EXPANDING_LOW_OPERATOR

            if stream_id & cls.STREAM_JUDGE_BIT15_OPERATOR:
                return task_id & cls.EXPANDING_LOW_OPERATOR
            else:
                return stream_id & cls.EXPANDING_LOW_OPERATOR

        else:
            if stream_id & cls.STREAM_JUDGE_BIT12_OPERATOR:
                return Utils.get_stream_id(stream_id)

            if stream_id & cls.STREAM_JUDGE_BIT13_OPERATOR:
                stream_id = task_id & cls.COMMON_LOW_OPERATOR

            return Utils.get_stream_id(stream_id)

    @classmethod
    def set_task_id(cls, stream_id, task_id, sqe_type=0, need_merge=True):
        """
        In the FFTS scenario, task ID composition varies based on stream ID bits and SQE type:

        When stream expansion is enabled:
        - For PLACE_HOLDER_SQE and EVENT_RECORD_SQE types, return original task_id directly
        - If bit15 of stream_id is set, combine:
            - High bits from task_id (using STREAM_JUDGE_BIT15_OPERATOR mask)
            - Low bits from stream_id (using EXPANDING_LOW_OPERATOR mask)
        - Otherwise, return original task_id

        When stream expansion is disabled:
        - If bit12 of stream_id is set, combine:
            - Low bits from task_id (using TASK_LOW_OPERATOR mask)
            - High bits from stream_id (using STREAM_HIGH_OPERATOR mask)
        - If bit13 of stream_id is set, combine:
            - Low bits from stream_id (using COMMON_LOW_OPERATOR mask)
            - High bits from task_id (using COMMON_HIGH_OPERATOR mask)
        - Otherwise, return original task_id
        """
        if ChipManager().is_chip_v6():
            return cls.set_unique_task_id(stream_id, task_id, need_merge)
        if StreamExpandSpecReader().is_stream_expand:
            if sqe_type in [SqeType.StarsSqeType.PLACE_HOLDER_SQE, SqeType.StarsSqeType.EVENT_RECORD_SQE]:
                return task_id

            if stream_id & cls.STREAM_JUDGE_BIT15_OPERATOR:
                return task_id & cls.STREAM_JUDGE_BIT15_OPERATOR | stream_id & cls.EXPANDING_LOW_OPERATOR
            else:
                return task_id
        else:
            if stream_id & cls.STREAM_JUDGE_BIT12_OPERATOR:
                task_id = task_id & cls.TASK_LOW_OPERATOR
                task_id |= stream_id & cls.STREAM_HIGH_OPERATOR
            elif stream_id & cls.STREAM_JUDGE_BIT13_OPERATOR:
                task_id = (stream_id & cls.COMMON_LOW_OPERATOR) | (task_id & cls.COMMON_HIGH_OPERATOR)
            return task_id

    @staticmethod
    def set_unique_task_id(stream_id, task_id, need_merge=True):
        """
        唯一id场景下，原本16bit task id扩展到32bit，
        使用原stream id的bit位，原stream id低位、task id高位，合并为新task id
        param:
            stream_id: 原stream id
            task_id: 原task id
            need_merge: 是否需要合并stream id和task id, 用于32bit sqe id场景，默认True
                        传入task id为32bit时，设置为False
        return:
            合并后的task id
        """
        if not need_merge:
            return task_id
        return (task_id << 16) | stream_id
