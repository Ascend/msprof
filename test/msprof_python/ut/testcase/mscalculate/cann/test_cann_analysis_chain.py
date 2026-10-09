#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import collections
import unittest
from unittest import mock

from common_func.constant import Constant
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_analysis_chain import CANNAnalysisChain
from mscalculate.cann.cann_analysis_gear import RootGear
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.cann_event_generator import CANNThreadDB
from mscalculate.cann.event import Event
from mscalculate.cann.event_queue import EventQueue
from mscalculate.cann.tree import TreeNode
from profiling_bean.db_dto.api_data_dto import ApiDataDto, invalid_dto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto

NAMESPACE = 'mscalculate.cann.cann_analysis_chain'


class TestCANNAnalysisChain(unittest.TestCase):
    event_col = collections.namedtuple('Event', 'level, thread, start, end, type_')

    def test_start(self):
        chain = CANNAnalysisChain(1, CANNThreadDB(1, EventQueue(1), ApiDataDatabase(1), AdditionalRecordDatabase(1)),
                                  {Constant.ROOT_LEVEL: RootGear("")})
        tree = TreeNode(Event(Constant.ROOT_LEVEL, 1, 0, 300, 'root'))
        tree.children.append(TreeNode(Event(Constant.ROOT_LEVEL, 1, 100, 200, 'root')))
        with mock.patch(NAMESPACE + '.CANNAnalysisChain.build_tree',
                        return_value=tree):
            chain.start()

    def test_build_tree(self):
        def add_api_event(q, api_db: ApiDataDatabase, event_col):
            api = ApiDataDto()
            api.level = event_col.level
            api.thread_id = event_col.thread
            api.start = event_col.start
            api.end = event_col.end
            api.struct_type = event_col.type_
            event = api_db.put(api)
            q.add(event)

        def add_node_basic_record_event(q, record_db: AdditionalRecordDatabase, event_col):
            node_basic = NodeBasicInfoDto()
            node_basic.level = event_col.level
            node_basic.thread_id = event_col.thread
            node_basic.timestamp = event_col.start
            node_basic.struct_type = event_col.type_
            record = AdditionalRecord(node_basic, event_col.start, event_col.type_)
            event = record_db.put(record)
            q.add(event)

        def add_task_track_record_event(q, record_db: AdditionalRecordDatabase, event_col):
            task_track = TaskTrackDto()
            task_track.level = event_col.level
            task_track.thread_id = event_col.thread
            task_track.timestamp = event_col.start
            task_track.struct_type = event_col.type_
            record = AdditionalRecord(task_track, event_col.start, event_col.type_)
            event = record_db.put(record)
            q.add(event)

        def build_event_q():
            q = EventQueue(1)
            api_db = ApiDataDatabase(1)
            record_db = AdditionalRecordDatabase(1)
            add_api_event(q, api_db, self.event_col('model', 1, 100, 200, 'ModelLoad'))
            add_api_event(q, api_db, self.event_col('node', 1, 110, 130, 'node1'))
            add_api_event(q, api_db, self.event_col('node', 1, 150, 170, 'node2'))
            add_api_event(q, api_db, self.event_col('node', 1, 121, 127, 'node3'))
            add_node_basic_record_event(q, record_db, self.event_col(
                'node', 1, 130, 130, 'node_basic_info1'))
            add_node_basic_record_event(q, record_db, self.event_col(
                'node', 1, 170, 170, 'node_basic_info2'))
            add_node_basic_record_event(q, record_db, self.event_col(
                'node', 1, 127, 127, 'node_basic_info3'))
            add_api_event(q, api_db, self.event_col("runtime", 1, 116, 120, 'task1'))
            add_api_event(q, api_db, self.event_col("runtime", 1, 152, 160, 'task2'))
            add_task_track_record_event(q, record_db, self.event_col(
                "runtime", 1, 118, 118, 'task_track1'))
            add_task_track_record_event(q, record_db, self.event_col(
                "runtime", 1, 155, 155, 'task_track2'))
            add_api_event(q, api_db, self.event_col("runtime", 1, 180, 190, 'task3'))
            add_api_event(q, api_db, self.event_col("runtime", 1, 210, 222, 'task4'))
            return q, api_db, record_db

        event_q, api_db, record_db = build_event_q()
        event_q.lock()
        chain = CANNAnalysisChain(1, CANNThreadDB(1, event_q, api_db, record_db), {})
        root_api = invalid_dto('root', 1, 0, 300, 'root')
        root_event = chain.db.add_api(root_api)
        root_node = TreeNode(root_event)
        tree = chain.build_tree(root_node, 0)
        # check level1
        self.assertEqual(len(tree.children), 2)
        self.assertEqual(tree.children[1].event.struct_type, "task4")
        # check level2
        self.assertEqual(len(tree.children[0].children), 3)
        self.assertEqual(tree.children[0].children[0].event.struct_type, "node1")
        self.assertEqual(tree.children[0].children[1].event.struct_type, "node2")
        self.assertEqual(tree.children[0].children[2].event.struct_type, "task3")
        self.assertEqual(len(tree.children[0].children[0].event.additional_record), 1)
        self.assertEqual(len(tree.children[0].children[1].event.additional_record), 1)
        # check level3
        self.assertEqual(len(tree.children[0].children[0].children), 2)
        self.assertEqual(len(tree.children[0].children[1].children), 1)
        self.assertEqual(tree.children[0].children[0].children[0].event.struct_type, "task1")
        self.assertEqual(tree.children[0].children[0].children[1].event.struct_type, "node3")
        self.assertEqual(tree.children[0].children[1].children[0].event.struct_type, "task2")
        self.assertEqual(len(tree.children[0].children[0].children[0].event.additional_record), 1)
        self.assertEqual(len(tree.children[0].children[0].children[1].event.additional_record), 1)
        self.assertEqual(len(tree.children[0].children[1].children[0].event.additional_record), 1)

        # 超出递归深度
        tree = chain.build_tree(root_node, 25)


if __name__ == '__main__':
    unittest.main()
