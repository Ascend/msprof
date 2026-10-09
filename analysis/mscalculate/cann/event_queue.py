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

import logging

from mscalculate.cann.event import Event


class EventQueue:
    """
    This class is used for unified scheduling of events.
    1. Provides access to the event with the highest priority.
    """

    INIT_LEN = 100000

    def __init__(self, thread_id, init_len=INIT_LEN):
        self.thread_id = thread_id
        # for performance, use list to replace queue
        self.queue = [0] * init_len
        self.size = 0
        self.index = 0
        self._max_bound = -1
        self._lock = False

    def malloc_new_size(self, size: int):
        new_queue = [0] * size
        new_queue[: len(self.queue)] = self.queue
        self.queue = new_queue
        logging.info("malloc %d size for new thread %d queue", size, self.thread_id)

    def add(self, event: Event):
        if self._lock:
            logging.error("queue %d is locked, add function is illegal", self.thread_id)
            return
        if self.size >= len(self.queue):
            self.malloc_new_size(2 * len(self.queue))
        self.queue[self.size] = event
        self._max_bound = max(event.bound, self._max_bound)
        self.size += 1

    def lock(self):
        self.queue[0 : self.size] = sorted(self.queue[: self.size])
        self._lock = True

    def pop(self):
        if not self._lock:
            logging.error("queue %d is not locked, pop function is illegal", self.thread_id)
            return Event.invalid_event()
        if self.index >= self.size:
            return Event.invalid_event()
        event = self.queue[self.index]
        self.index += 1
        return event

    def top(self) -> Event:
        if not self._lock:
            logging.error("queue %d is not locked, top function is illegal", self.thread_id)
            return Event.invalid_event()
        if self.index >= self.size:
            return Event.invalid_event()
        return self.queue[self.index]

    def empty(self):
        return self.index == self.size

    def get_max_bound(self):
        return self._max_bound
