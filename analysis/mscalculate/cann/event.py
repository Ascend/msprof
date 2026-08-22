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

from typing import List

from mscalculate.cann.additional_record import AdditionalRecord


class Event:
    """
    As the basic scheduling unit of a large queue, this class can represent a task at any layer in the CAN.
    1. An API is invoked to start and end (including event reporting in pairs).
    2. Start and end the same additional data.
    """

    INVALID_EVENT_LEVEL = -1
    INVALID_THREAD_ID = -1
    _ID = 0

    def __init__(self, cann_level: int, thread_id: int, timestamp: float, bound: int, struct_type: str):
        self.cann_level = cann_level
        self.timestamp = timestamp  # begin
        self.thread_id = thread_id
        self.bound = bound  # end
        self.struct_type = struct_type
        # additional record
        self.additional_record: List[AdditionalRecord] = []
        self.id = self._ID
        self.kfc_node_event: list[Event] = []
        Event._ID += 1

    def __lt__(self, other):
        return self.timestamp < other.timestamp or (
            self.timestamp == other.timestamp and self.cann_level < other.cann_level
        )

    def __hash__(self):
        return hash(self.id)

    def __eq__(self, other):
        return self.id == other.id if isinstance(other, Event) else False

    def __str__(self):
        return self.struct_type + "-" + str(self.id)

    @staticmethod
    def invalid_event(cann_level=INVALID_EVENT_LEVEL, thread_id=INVALID_THREAD_ID):
        return Event(cann_level, thread_id, -1, -1, "")

    def is_invalid(self):
        return self.struct_type == ""

    def is_additional(self):
        return self.timestamp == self.bound

    def add_additional_record(self, record):
        self.additional_record.append(record)

    def to_string(self):
        return "level: {}, thread_id: {}, timestamp: {}, type: {}".format(
            self.cann_level, self.thread_id, self.timestamp, self.struct_type
        )
