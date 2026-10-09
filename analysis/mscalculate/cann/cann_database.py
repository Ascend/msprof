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

from common_func.constant import Constant
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.event import Event
from profiling_bean.db_dto.api_data_dto import ApiDataDto


class CANNDatabase:
    LEVELS_MAP = {
        'root': Constant.ROOT_LEVEL,
        "acl": Constant.ACL_LEVEL,
        "model": Constant.MODEL_LEVEL,
        "node": Constant.NODE_LEVEL,
        "communication": Constant.HCCL_LEVEL,
        "runtime": Constant.TASK_LEVEL,
    }

    def __init__(self, thread_id):
        self.thread_id = thread_id


class ApiDataDatabase(CANNDatabase):
    def __init__(self, thread_id):
        super().__init__(thread_id)
        self._data = dict()

    def put(self, data: ApiDataDto) -> Event:
        event = Event(
            self.LEVELS_MAP.get(data.level, data.level), data.thread_id, data.start, data.end, data.struct_type
        )

        self._data[event] = data
        return event

    def get(self, event: Event) -> ApiDataDto:
        return self._data.get(event, ApiDataDto())


class AdditionalRecordDatabase(CANNDatabase):
    def __init__(self, thread_id):
        super().__init__(thread_id)
        self._data = dict()

    def put(self, data: AdditionalRecord) -> Event:
        dto = data.dto
        event = Event(self.LEVELS_MAP.get(dto.level), dto.thread_id, dto.timestamp, dto.timestamp, dto.struct_type)
        self._data[event] = data
        return event

    def get(self, event: Event) -> AdditionalRecord:
        return self._data.get(event, AdditionalRecord())
