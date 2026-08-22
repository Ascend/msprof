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

from common_func.constant import Constant
from mscalculate.cann.cann_event_generator import CANNThreadDB
from mscalculate.cann.event import Event
from mscalculate.cann.tree import TreeNode
from profiling_bean.db_dto.api_data_dto import invalid_dto


class CANNAnalysisChain:
    """
    The analysis chain for CANN software stack. Each instance processes the data of a thread in the CANN.
    """

    def __init__(self, thread_id: int, db: CANNThreadDB, gears: dict):
        """
        thread_id：
        event_q: priority queue with thread id
        gears: gear set, one gear process one cann level
        """
        self.thread_id = thread_id
        self.event_q = db.event_q
        self.db = db
        self.gears = gears
        self.last_event_record = {
            Constant.ACL_LEVEL: Event.invalid_event(),
            Constant.MODEL_LEVEL: Event.invalid_event(),
            Constant.NODE_LEVEL: Event.invalid_event(),
            Constant.TASK_LEVEL: Event.invalid_event(),
            Constant.HCCL_LEVEL: Event.invalid_event(),
        }
        self.now_stack = {
            Constant.ACL_LEVEL: Event.invalid_event(),
            Constant.MODEL_LEVEL: Event.invalid_event(),
            Constant.NODE_LEVEL: Event.invalid_event(),
            Constant.TASK_LEVEL: Event.invalid_event(),
            Constant.HCCL_LEVEL: Event.invalid_event(),
        }

    def start(self):
        root_dto = invalid_dto(Constant.ROOT_LEVEL, self.thread_id, 0, self.db.get_time_bound() + 1, "root")
        root_event = self.db.add_api(root_dto)
        # Associate the upper-level and lower-level relationships of events
        # based on timestamp points to build a callstack tree.
        root = TreeNode(root_event)
        event_tree = self.build_tree(root)
        logging.info("Finish build event tree for thread %d", self.thread_id)
        # Perform inter-layer association.
        self.run(event_tree)

    def run(self, node: TreeNode, depth: int = 0):
        depth_limit = 20  # 限制最大递归深度
        if depth > depth_limit:
            logging.error("Recursion depth exceeds limit!")
            return
        depth += 1
        self.gears.get(node.event.cann_level).run(node.event, self.now_stack)
        self.now_stack[node.event.cann_level] = node.event
        for child in node.children:
            self.run(child, depth)
            if child.event.cann_level == Constant.NODE_LEVEL and node.event.cann_level == child.event.cann_level:
                # hccl aicpu下发场景, 会出现2层node嵌套
                # [=======================Node(hcom_allReduce_)==============================]
                #      [===Node(hcomAicpuInit)==]     [===Node(allreduceAicpuKernel)==]
                self.now_stack[node.event.cann_level] = node.event
        self.now_stack[node.event.cann_level] = Event.invalid_event()

    def build_tree(self, parent: TreeNode, depth: int = 0) -> TreeNode:
        depth_limit = 20  # 限制最大递归深度
        if depth > depth_limit:
            logging.error("Recursion depth exceeds limit, when build cann tree!")
            return parent
        depth += 1
        while True:
            # All processing is complete
            if self.event_q.empty():
                return parent
            event = self.event_q.top()

            # Some data in parent level is lost or not be uploaded.
            if event.bound > parent.event.bound:
                return parent
            # acl层级只能挂载在root层级下,否则不参与建树
            if event.cann_level == Constant.ACL_LEVEL and parent.event.cann_level != Constant.ROOT_LEVEL:
                self.event_q.pop()
                continue

            # the current level has been processed.
            if not event.is_additional() and event.cann_level < parent.event.cann_level:
                return parent

            event = self.event_q.pop()
            if event.is_additional():
                last_event = self.last_event_record.get(event.cann_level)
                if last_event and last_event.timestamp <= event.timestamp <= last_event.bound:
                    # This event is a supplementary information, not an independent task unit.
                    # Events are sorted by timestamp. Therefore, this information is recorded in the last
                    # processed event.
                    last_event.add_additional_record(self.db.get_record(event))
                else:
                    # This scenario indicates that the event corresponding to the additional information is not
                    # reported. Hanging on a tree as an empty node
                    # 1. Start and end tasks of runtime
                    # 2 ....
                    empty_dto = invalid_dto(event.cann_level, event.thread_id)
                    empty_event: Event = self.db.add_api(empty_dto)
                    empty_event.add_additional_record(self.db.get_record(event))
                    parent.add_child(TreeNode(empty_event))
                continue

            self.last_event_record[event.cann_level] = event
            child_node = TreeNode(event)
            child_tree = self.build_tree(child_node, depth)
            if event.cann_level == Constant.NODE_LEVEL and event.cann_level == parent.event.cann_level:
                # hccl aicpu下发场景, 会出现2层node嵌套
                # [=======================Node(hcom_allReduce_)==============================]
                #      [===Node(hcomAicpuInit)==]     [===Node(allreduceAicpuKernel)==]
                self.last_event_record[event.cann_level] = parent.event
                parent.event.kfc_node_event.append(event)
            parent.add_child(child_tree)
