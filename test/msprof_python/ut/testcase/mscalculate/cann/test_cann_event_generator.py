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
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import collections
import unittest
from unittest import mock

from mscalculate.cann.cann_event_generator import CANNEventGenerator
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.event_data_dto import EventDataDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.graph_id_map_dto import GraphIdMapDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.hccl_op_info_dto import HCCLOpInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.node_attr_info_dto import NodeAttrInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto

NAMESPACE = 'mscalculate.cann.cann_event_generator'


class TestClusterLinkCalculate(unittest.TestCase):
    api_col = collections.namedtuple('Api', 'level, thread_id, start, end, struct_type, item_id')
    event_col = collections.namedtuple('Event', 'level, thread_id, timestamp, struct_type, request_id, item_id')

    @staticmethod
    def create_api_dto(api_c):
        api = ApiDataDto()
        api.level = api_c.level
        api.thread_id = api_c.thread_id
        api.start = api_c.start
        api.end = api_c.end
        api.struct_type = api_c.struct_type
        api.item_id = api_c.item_id
        return api

    @staticmethod
    def create_event_dto(event_c):
        event = EventDataDto()
        event.level = event_c.level
        event.thread_id = event_c.thread_id
        event.timestamp = event_c.timestamp
        event.struct_type = event_c.struct_type
        event.request_id = event_c.request_id
        event.item_id = event_c.item_id
        return event

    def test_run(self):
        with mock.patch(NAMESPACE + '.CANNEventGenerator.generate_api_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_node_basic_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_node_attr_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_tensor_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_task_track_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_mem_copy_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_graph_id_map_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_fusion_op_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_ctx_id_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_hccl_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_hccl_op_info_event'):
            ret = CANNEventGenerator(["test"]).run()
            self.assertEqual(len(ret), 0)

    def test_generate_api_event_should_return_5_cann_event_when_3_api_and_4_matched_event_1_mismatched_event(self):
        api1 = self.create_api_dto(self.api_col('model', 0, 10, 100, 'm', 0))
        api2 = self.create_api_dto(self.api_col('node', 0, 12, 90, 'mm', 0))
        api3 = self.create_api_dto(self.api_col('runtime', 0, 19, 80, 'mmm', 1))
        event1 = self.create_event_dto(self.event_col('model', 0, 10, 'm', 0, 0))
        event2 = self.create_event_dto(self.event_col('model', 0, 10, 'mm', 0, 0))
        event3 = self.create_event_dto(self.event_col('node', 1, 13, 'm', 0, 1))
        event4 = self.create_event_dto(self.event_col('node', 1, 13, 'm', 0, 1))
        event5 = self.create_event_dto(self.event_col('model', 0, 10, 'm', 0, 0))
        with mock.patch(NAMESPACE + '.CANNEventGenerator.collect_time_period_data',
                        return_value=([api1, api2, api3], [event1, event2, event3, event4, event5])):
            generator = CANNEventGenerator("test")
            generator.generate_api_event()
            self.assertEqual(generator.event_queues.get(0).size, 4)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_is_kernel_api_when_api_struct_type_invalid_then_return_False(self):
        api = self.create_api_dto(self.api_col('model', 0, 10, 100, 'StreamSyncTaskFinish', 0))
        check = CANNEventGenerator("test")
        self.assertFalse(check.is_kernel_api(api))

    def test_is_kernel_api_when_api_time_invalid_then_return_False(self):
        api1 = self.create_api_dto(self.api_col('model', 0, -1, 100, 'm', 0))
        api2 = self.create_api_dto(self.api_col('model', 0, 100, 100, 'm', 0))
        check = CANNEventGenerator("test")
        self.assertFalse(check.is_kernel_api(api1))
        self.assertFalse(check.is_kernel_api(api2))

    def test_is_kernel_api_when_api_struct_type_invalid_then_return_True(self):
        api = self.create_api_dto(self.api_col('model', 0, 10, 100, 'm', 0))
        check = CANNEventGenerator("test")
        self.assertTrue(check.is_kernel_api(api))

    def test_collect_time_period_data_should_return_0_api_when_check_api_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.ApiDataModel.check_db',
                        return_value=False):
            api, event = generator.collect_time_period_data()
            self.assertEqual(len(api), 0)
            self.assertEqual(len(event), 0)

    def test_collect_time_period_data_should_return_0_event_when_check_event_db_error(self):
        generator = CANNEventGenerator("test")
        dto1 = ApiDataDto()
        dto1.thread_id = 0
        with mock.patch(NAMESPACE + '.ApiDataModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataModel.get_all_data', return_value=[dto1]), \
                mock.patch(NAMESPACE + '.EventDataModel.check_db',
                           return_value=False):
            api, event = generator.collect_time_period_data()
            self.assertEqual(len(api), 1)
            self.assertEqual(len(event), 0)

    def test_collect_time_period_data_should_should_return_1_api_and_1_event_when_1_and_1_in_db(self):
        dto1 = ApiDataDto()
        dto1.thread_id = 0
        dto2 = EventDataDto()
        dto2.thread_id = 1
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.ApiDataModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataModel.get_all_data',
                           return_value=[dto1]), \
                mock.patch(NAMESPACE + '.EventDataModel.check_db',
                           return_value=True), \
                mock.patch(NAMESPACE + '.EventDataModel.get_all_data',
                           return_value=[dto2]):
            api, event = generator.collect_time_period_data()
            self.assertEqual(len(api), 1)
            self.assertEqual(len(event), 1)

    def test_generate_node_basic_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.NodeBasicInfoModel.check_db',
                        return_value=False):
            generator.generate_node_basic_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_node_basic_info_event_should_record_2_event_when_2_node_in_2_thread_in_db(self):
        dto1 = NodeBasicInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = NodeBasicInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.NodeBasicInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.NodeBasicInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_node_basic_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_node_attr_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.NodeAttrInfoModel.check_db',
                        return_value=False):
            generator.generate_node_attr_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_node_attr_info_event_should_record_2_event_when_2_node_in_2_thread_in_db(self):
        dto1 = NodeAttrInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = NodeAttrInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.NodeAttrInfoModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NodeAttrInfoModel.get_all_data', return_value=[dto1, dto2]):
            generator.generate_node_attr_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_tensor_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.TensorAddInfoModel.check_db',
                        return_value=False):
            generator.generate_tensor_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_tensor_info_event_should_record_2_event_when_2_tensor_info_in_2_thread_in_db(self):
        dto1 = TensorInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = TensorInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.TensorAddInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.TensorAddInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_tensor_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_task_track_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.TaskTrackModel.check_db',
                        return_value=False):
            generator.generate_task_track_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_task_track_event_should_record_2_event_when_2_mem_copy_in_2_thread_in_db(self):
        dto1 = TaskTrackDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = TaskTrackDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.TaskTrackModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.TaskTrackModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_task_track_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_mem_copy_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.MemcpyInfoModel.check_db',
                        return_value=False):
            generator.generate_mem_copy_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_mem_copy_info_should_record_2_event_when_2_mem_copy_in_2_thread_in_db(self):
        dto1 = MemCopyInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = MemCopyInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.MemcpyInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.MemcpyInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_mem_copy_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_ctx_id_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.CtxIdModel.check_db',
                        return_value=False):
            generator.generate_ctx_id_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_ctx_id_event_should_record_2_event_when_2_ctx_id_in_2_thread_in_db(self):
        dto1 = CtxIdDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = CtxIdDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.CtxIdModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.CtxIdModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_ctx_id_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_hccl_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.HcclInfoModel.check_db',
                        return_value=False):
            generator.generate_hccl_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_hccl_info_event_should_record_2_event_when_2_hccl_info_in_2_thread_in_db(self):
        dto1 = HCCLInfoDto()
        dto1.thread_id = 0
        dto2 = HCCLInfoDto()
        dto2.thread_id = 1
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.HcclInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.HcclInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_hccl_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_graph_id_map_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.GraphAddInfoModel.check_db',
                        return_value=False):
            generator.generate_graph_id_map_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_graph_id_map_event_should_record_2_event_when_2_graph_id_map_in_2_thread_in_db(self):
        dto1 = GraphIdMapDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = GraphIdMapDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.GraphAddInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.GraphAddInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_graph_id_map_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_fusion_op_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.FusionAddInfoModel.check_db',
                        return_value=False):
            generator.generate_fusion_op_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_fusion_op_info_event_should_record_2_event_when_2_fusion_info_in_2_thread_in_db(self):
        dto1 = FusionOpInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = FusionOpInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.FusionAddInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.FusionAddInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_fusion_op_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)

    def test_generate_hccl_op_info_event_should_record_0_event_when_check_db_error(self):
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.HcclOpInfoModel.check_db',
                        return_value=False):
            generator.generate_hccl_op_info_event()
            self.assertEqual(len(generator.event_queues), 0)

    def test_generate_hccl_op_info_event_should_record_2_event_when_2_node_in_2_thread_in_db(self):
        dto1 = HCCLOpInfoDto()
        dto1.thread_id = 0
        dto1.timestamp = 100
        dto2 = HCCLOpInfoDto()
        dto2.thread_id = 1
        dto2.timestamp = 200
        generator = CANNEventGenerator("test")
        with mock.patch(NAMESPACE + '.HcclOpInfoModel.check_db',
                        return_value=True), \
                mock.patch(NAMESPACE + '.HcclOpInfoModel.get_all_data',
                           return_value=[dto1, dto2]):
            generator.generate_hccl_op_info_event()
            self.assertEqual(len(generator.event_queues), 2)
            self.assertEqual(len(generator.record_databases), 2)
            self.assertEqual(generator.event_queues.get(0).size, 1)
            self.assertEqual(generator.event_queues.get(1).size, 1)


if __name__ == '__main__':
    unittest.main()
