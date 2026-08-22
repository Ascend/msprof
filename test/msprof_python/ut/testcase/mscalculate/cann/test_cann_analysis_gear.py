#!/usr/bin/env python
# coding=utf-8
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
"""
import collections
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.rt_add_info_center import RTAddInfoCenter
from constant.constant import clear_dt_project
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_analysis_gear import ACLGear
from mscalculate.cann.cann_analysis_gear import ModelGear
from mscalculate.cann.cann_analysis_gear import NodeGear
from mscalculate.cann.cann_analysis_gear import TaskGear
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.cann_event_generator import CANNThreadDB
from mscalculate.cann.event import Event
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.graph_id_map_dto import GraphIdMapDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.hccl_op_info_dto import HCCLOpInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.node_attr_info_dto import NodeAttrInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto

NAMESPACE = 'mscalculate.cann.cann_analysis_gear'


class TestCANNAnalysisGear(unittest.TestCase):
    """
    this class if for st
    """
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_CANNAnalysisGear")
    PROF_HOST_DIR = os.path.join(DIR_PATH, 'PROF1', 'host')
    event_col = collections.namedtuple('Event', 'level, thread_id, start, end, struct_type, item_id')

    @staticmethod
    def create_api_event(event_c, api_db):
        api = ApiDataDto()
        api.level = event_c.level
        api.thread_id = event_c.thread_id
        api.start = event_c.start
        api.end = event_c.end
        api.struct_type = event_c.struct_type
        api.item_id = event_c.item_id
        event = api_db.put(api)
        return event

    @staticmethod
    def create_addition_record(dto, time, record_db, struct_type=""):
        record = AdditionalRecord(dto, time, struct_type)
        event = record_db.put(record)
        return record

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        path = os.path.join(self.PROF_HOST_DIR, 'sqlite')
        os.makedirs(path)
        InfoConfReader()._sample_json = {}

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_acl_gear(self):
        gear = ACLGear(self.PROF_HOST_DIR)

        api_db = ApiDataDatabase(1)
        event = self.create_api_event(self.event_col(Constant.ACL_LEVEL, 1, 0, 100, "", 0), api_db)

        gear.run(event, {})
        gear.flush_data()

    def test_model_gear(self):
        gear = ModelGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db)
        gear.set_db(db)
        event1 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 0, 100, "ModelLoad", 0), api_db)
        event2 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 101, 200, "InputCopy", 0), api_db)
        event3 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 201, 300, "ModelExecute", 0), api_db)
        event4 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 301, 400, "OutputCopy", 0), api_db)

        graph_id_map_dto_1 = GraphIdMapDto()
        graph_id_map_dto_1.struct_type = "graph_id_map"
        graph_id_map_dto_1.model_name = "resnet50"
        graph_id_map_dto_1.timestamp = 50
        event1.additional_record = [
            self.create_addition_record(graph_id_map_dto_1, 50, record_db),
        ]
        graph_id_map_dto_2 = GraphIdMapDto()
        graph_id_map_dto_2.struct_type = "graph_id_map"
        graph_id_map_dto_2.model_name = "resnet50_1"
        graph_id_map_dto_2.timestamp = 250
        event3.additional_record = [
            self.create_addition_record(graph_id_map_dto_2, 50, record_db),
        ]
        gear.run(event1, {})
        gear.run(event2, {})
        gear.run(event3, {})
        gear.run(event4, {})
        gear.flush_data()
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_INFO),
                                          DBNameConstant.TABLE_MODEL_NAME, 'model_name', "resnet50"))
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_INFO),
                                          DBNameConstant.TABLE_MODEL_NAME, 'model_name', "resnet50_1"))

    def test_node_gear(self):
        gear = NodeGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        event3: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 102, 140, "launch", "conv"), api_db)
        event3.additional_record = [
            self.create_addition_record(NodeBasicInfoDto(), 140, record_db),
            self.create_addition_record(TensorInfoDto(), 140, record_db)
        ]
        event4: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 141, 145, "", 1), api_db)
        fusion_op = FusionOpInfoDto()
        fusion_op.op_name = "fusion1"
        event4.additional_record = [self.create_addition_record(fusion_op, 143, record_db)]
        event5: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 146, 160, "launch", "hccl1"), api_db)
        hccl_node_basic_info = NodeBasicInfoDto()
        hccl_node_basic_info.task_type = "HCCL"
        event5.additional_record = [self.create_addition_record(hccl_node_basic_info, 150, record_db)]

        gear.run(event3, {})
        gear.run(event4, {})
        gear.run(event5, {})
        gear.flush_data()

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_HOST_INFO),
                                           DBNameConstant.TABLE_GE_HOST), 2)
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_HOST_INFO),
                                          DBNameConstant.TABLE_GE_HOST, 'end_time', 140))
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_INFO),
                                           DBNameConstant.TABLE_GE_FUSION_OP_INFO), 1)

    def test_task_gear(self):
        '''
        Node(thread 1):                   event3 event6 event8 event11 event14 event17
                                            |      |      |       |      |        |
        Hccl(thread 1):                   event4   |    event9 event12 event15    |
                                            |      |      |       |      |        |
        Runtime(thread 1): event1 event2 event5 event7 event10    |    event16 event18
        Runtime(thread 2):                                     event13
        '''
        InfoConfReader()._sample_json["profLevel"] = "l1"
        gear = TaskGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        event1 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 100, 101, "api1", 0), api_db)
        event2 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 200, 250, "api2", 0), api_db)

        mem_dto = MemCopyInfoDto()
        mem_dto.struct_type = "1"
        mem_dto.memcpy_direction = 2
        task_dto = TaskTrackDto()
        task_dto.struct_type = "1"
        task_dto.task_id = 10
        task_dto.task_type = "KERNEL_AICORE"
        task_dto.timestamp = 12345
        event2.additional_record = [
            self.create_addition_record(mem_dto, 210, record_db),
            self.create_addition_record(task_dto, 220, record_db)
        ]

        event3: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 258, 300, "launch", "hccl1"), api_db)
        event4 = self.create_api_event(self.event_col(Constant.HCCL_LEVEL, 1, 260, 290, "hccl api", 0), api_db)
        hccl_info_dto = HCCLInfoDto()
        hccl_info_dto.plane_id = 8
        event4.additional_record = [self.create_addition_record(hccl_info_dto, 277, record_db)]
        event5 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 264, 280, "api3", 0), api_db)
        task_dto2 = TaskTrackDto()
        task_dto2.task_id = 11
        task_dto2.struct_type = "1"
        task_dto2.task_type = "NOTIFY_RECORD"
        event5.additional_record = [self.create_addition_record(task_dto2, 266, record_db)]

        event6: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 310, 360, "launch", "conv"), api_db)
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "1"
        node_basic_info_dto.task_type = 'AI_CORE'
        node_basic_info_dto.op_name = "1"
        node_basic_info_dto.timestamp = 360
        tensor_info_dto = TensorInfoDto()
        tensor_info_dto.op_name = "1"
        tensor_info_dto.timestamp = 360
        tensor_info_dto.struct_type = '1'
        ctx_id_dto = CtxIdDto()
        ctx_id_dto.ctx_id = "8"
        ctx_id_dto.op_name = "1"
        ctx_id_dto.timestamp = 360
        event6.additional_record = [
            self.create_addition_record(node_basic_info_dto, 360, record_db),
            self.create_addition_record(tensor_info_dto, 360, record_db),
            self.create_addition_record(ctx_id_dto, 360, record_db)
        ]
        event7 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 320, 350, "api4", 0), api_db)
        task_dto3 = TaskTrackDto()
        task_dto3.task_id = 12
        task_dto3.struct_type = "1"
        task_dto3.task_type = "KERNEL_AICORE"
        event7.additional_record = [self.create_addition_record(task_dto3, 333, record_db)]

        event8: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 370, 400, "launch", "reduceTBE"), api_db)
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "1"
        node_basic_info_dto.task_type = 'HCCL'
        node_basic_info_dto.op_name = "1"
        node_basic_info_dto.timestamp = 400
        hccl_op_info = HCCLOpInfoDto(data_type="FP32", alg_type="RING-MESH", count=742)
        event8.additional_record = [
            self.create_addition_record(node_basic_info_dto, 400, record_db),
            self.create_addition_record(hccl_op_info, 400, record_db)
        ]
        event9 = self.create_api_event(self.event_col(Constant.HCCL_LEVEL, 1, 372, 390, "hccl tbe", 0), api_db)
        hccl_info_dto = HCCLInfoDto()
        hccl_info_dto.plane_id = 9
        event9.additional_record = [self.create_addition_record(hccl_info_dto, 380, record_db)]
        event10 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 375, 380, "api5", 0), api_db)
        task_dto4 = TaskTrackDto()
        task_dto4.task_id = 15
        task_dto4.struct_type = "1"
        task_dto4.task_type = "KERNEL_AICORE"
        event10.additional_record = [self.create_addition_record(task_dto4, 377, record_db)]

        event11: Event = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 400, 430, "launch", "hcom_allreduce"), api_db)
        event12 = self.create_api_event(self.event_col(Constant.HCCL_LEVEL, 1, 405, 420, "ReduceTBE", 1), api_db)
        event13 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 2, 410, 415, "api6", 0), api_db)
        task_dto5 = TaskTrackDto()
        task_dto5.struct_type = "1"
        task_dto5.task_id = 16
        task_dto5.task_type = "KERNEL_AICORE"
        task_dto5.timestamp = 415
        event13.additional_record = [
            self.create_addition_record(task_dto5, 415, record_db)
        ]

        # Lcal
        # lccl 纯通信算子
        event14 = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 450, 500, "launch", "LcclAllReduce"), api_db)
        event15 = self.create_api_event(
            self.event_col(Constant.HCCL_LEVEL, 1, 450, 500, "master", "LcclAllReduce"), api_db)
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "LcclAllReduce"
        node_basic_info_dto.task_type = 'AI_CORE'
        node_basic_info_dto.op_name = "LcclAllReduce"
        node_basic_info_dto.timestamp = 500
        hccl_op_info = HCCLOpInfoDto(data_type="FP16", alg_type="NA", count=100)
        event14.additional_record = [
            self.create_addition_record(node_basic_info_dto, 500, record_db),
            self.create_addition_record(hccl_op_info, 451, record_db)
        ]
        event16 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 460, 490, "api7", 0), api_db)
        task_dto4 = TaskTrackDto()
        task_dto4.task_id = 18
        task_dto4.struct_type = "task_track"
        task_dto4.task_type = "KERNEL_AIVEC"
        event16.additional_record = [self.create_addition_record(task_dto4, 470, record_db)]

        # lcoc 通算融合
        event17 = self.create_api_event(
            self.event_col(Constant.NODE_LEVEL, 1, 520, 600, "launch", "LcocMatmulAllReduce"), api_db)
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "LcocMatmulAllReduce"
        node_basic_info_dto.task_type = 'AI_CORE'
        node_basic_info_dto.op_name = "LcocMatmulAllReduce"
        node_basic_info_dto.timestamp = 600
        event17.additional_record = [
            self.create_addition_record(node_basic_info_dto, 600, record_db),
        ]
        event18 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 530, 590, "api8", 0), api_db)
        task_dto5 = TaskTrackDto()
        task_dto5.task_id = 20
        task_dto5.struct_type = "task_track"
        task_dto5.task_type = "KERNEL_AIVEC"
        event18.additional_record = [self.create_addition_record(task_dto5, 550, record_db)]

        RTAddInfoCenter("./test")

        gear.run(event1, {})
        gear.run(event2, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: Event.invalid_event(),
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.run(event5, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event3,
                          Constant.HCCL_LEVEL: event4})
        gear.run(event7, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event6,
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.run(event10, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event8,
                           Constant.HCCL_LEVEL: event9})
        gear.run(event13, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: Event.invalid_event(),
                           Constant.HCCL_LEVEL: event12})
        gear.run(event16, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event14,
                           Constant.HCCL_LEVEL: event15})
        gear.run(event18, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event17,
                           Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.flush_data()

        # 根据level判定后， reduce_tbe（多线程下发）根据nodeBasicInfo判定level的方案失效 无法进入l0流程
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TASK), 4)
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                          DBNameConstant.TABLE_GE_TASK, 'op_type', "1"))

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                          DBNameConstant.TABLE_GE_TASK, 'context_id', 8))

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                           DBNameConstant.TABLE_HCCL_TASK), 4)

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                          DBNameConstant.TABLE_HCCL_TASK, 'plane_id', 8))

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                          DBNameConstant.TABLE_HCCL_OP, 'data_type', "FP32"))
        del InfoConfReader()._sample_json["profLevel"]

    def test_task_gear_should_return_when_invalid_node_event(self):
        gear = TaskGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        event1 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 100, 101, "api", 0), api_db)
        gear.run(event1, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: Event.invalid_event(),
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.flush_data()
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TASK), 0)

    def test_task_gear_should_save_one_op_when_one_traditional_mode_node_event_FROM_PROF_LEVEL0(self):
        InfoConfReader()._sample_json["profLevel"] = "l0"
        gear = TaskGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        event1 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 120, 130, "api", 0), api_db)
        task_dto = TaskTrackDto()
        task_dto.task_id = 15
        task_dto.struct_type = "1"
        task_dto.task_type = "KERNEL_AICORE"
        event1.additional_record = [self.create_addition_record(task_dto, 125, record_db)]
        event2: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 110, 140, "launch", "op"), api_db)
        tensor_info_dto = TensorInfoDto()
        tensor_info_dto.tensor_num = 2
        tensor_info_dto.input_formats = "NCHW"
        tensor_info_dto.input_data_types = "FLOAT16"
        tensor_info_dto.input_shapes = '"1,2"'
        tensor_info_dto.output_formats = "ND"
        tensor_info_dto.output_data_types = "FLOAT32"
        tensor_info_dto.output_shapes = '"3,4"'
        event2.additional_record = [self.create_addition_record(tensor_info_dto, 126, record_db)]

        RTAddInfoCenter("./test")

        gear.run(event1, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event2,
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.flush_data()
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TASK), 1)
        ge_info_db = PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO)
        self.assertTrue(DBManager.check_item_in_table(
            ge_info_db, DBNameConstant.TABLE_GE_TASK, 'tensor_num', 2))
        self.assertTrue(DBManager.check_item_in_table(
            ge_info_db, DBNameConstant.TABLE_GE_TASK, 'input_shapes', '"1,2"'))
        self.assertTrue(DBManager.check_item_in_table(
            ge_info_db, DBNameConstant.TABLE_GE_TASK, 'output_shapes', '"3,4"'))
        del InfoConfReader()._sample_json["profLevel"]

    def test_task_gear_should_save_runtime_tensor_info_for_aclgraph_in_prof_level0(self):
        gear = TaskGear(self.PROF_HOST_DIR)
        task_dto = TaskTrackDto()
        task_dto.device_id = 0
        task_dto.stream_id = 1
        task_dto.task_id = 2
        task_dto.batch_id = 0
        task_dto.thread_id = 3
        task_dto.timestamp = 100
        op_info = RuntimeOpInfoDto(
            is_valid=True,
            model_id=4,
            op_name="aclgraph_op",
            tensor_num=2,
            input_formats="NCHW",
            input_data_types="FLOAT16",
            input_shapes='"1,2"',
            output_formats="ND",
            output_data_types="FLOAT32",
            output_shapes='"3,4"',
        )
        rt_add_info_center = RTAddInfoCenter("./test")

        with mock.patch.object(rt_add_info_center, "get_op_info_by_id", return_value=(4, op_info)):
            gear.add_kernel_task_only_task_track(task_dto, True)

        self.assertEqual(gear.task_info[0][13:20], [2, "NCHW", "FLOAT16", '"1,2"', "ND", "FLOAT32", '"3,4"'])

    def test_task_gear_should_save_one_op_when_one_traditional_mode_node_event_FROM_PROF_LEVEL1(self):
        gear = TaskGear(self.PROF_HOST_DIR)
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        event1 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 120, 130, "api", 0), api_db)
        task_dto = TaskTrackDto()
        task_dto.task_id = 15
        task_dto.struct_type = "1"
        task_dto.task_type = "KERNEL_AICORE"
        event1.additional_record = [self.create_addition_record(task_dto, 125, record_db)]
        event2: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 110, 140, "launch", "op"), api_db)
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "1"
        node_basic_info_dto.task_type = 'AI_CORE'
        node_basic_info_dto.op_name = "1"
        node_basic_info_dto.timestamp = 140
        tensor_info_dto = TensorInfoDto()
        tensor_info_dto.op_name = "1"
        tensor_info_dto.timestamp = 140
        tensor_info_dto.struct_type = '1'
        node_attr_info_dto = NodeAttrInfoDto()
        node_attr_info_dto.op_name = "1"
        node_attr_info_dto.timestamp = 140
        node_attr_info_dto.struct_type = '9'
        event2.additional_record = [
            self.create_addition_record(node_basic_info_dto, 140, record_db),
            self.create_addition_record(tensor_info_dto, 140, record_db),
            self.create_addition_record(node_attr_info_dto, 140, record_db)
        ]

        RTAddInfoCenter("./test")

        gear.run(event1, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event2,
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.flush_data()
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TASK), 1)


class TestTaskGear(TestCANNAnalysisGear):
    def test_get_context_ids_should_return_invalid_ids_when_invalid_node_and_invalid_hccl(self):
        gear = TaskGear("")
        call_stack = {Constant.NODE_LEVEL: Event.invalid_event(), Constant.HCCL_LEVEL: Event.invalid_event()}
        ids = gear.get_context_ids(call_stack, "KERNEL_MIX_AIC")
        self.assertEqual(ids, f"{str(NumberConstant.DEFAULT_GE_CONTEXT_ID)},0")

    def test_get_context_ids_should_return_invalid_ids_when_no_context_ids(self):
        gear = TaskGear("")
        node_event = Event.invalid_event()
        node_event.struct_type = "1"
        call_stack = {Constant.NODE_LEVEL: node_event, Constant.HCCL_LEVEL: Event.invalid_event()}
        ids = gear.get_context_ids(call_stack, "KERNEL_AICORE")
        self.assertEqual(ids, str(NumberConstant.DEFAULT_GE_CONTEXT_ID))

    def test_get_context_ids_should_return_valid_ids_when_node_contains_single_context_ids(self):
        gear = TaskGear("")
        node_event = Event.invalid_event()
        node_event.struct_type = "1"
        context_id_info1 = CtxIdDto()
        context_id_info1.ctx_id = "1"
        addition_info1 = AdditionalRecord(context_id_info1)
        context_id_info2 = CtxIdDto()
        context_id_info2.ctx_id = "2"
        addition_info2 = AdditionalRecord(context_id_info2)
        node_event.additional_record = [addition_info1, addition_info2]
        call_stack = {Constant.NODE_LEVEL: node_event, Constant.HCCL_LEVEL: Event.invalid_event()}
        ids = gear.get_context_ids(call_stack, "KERNEL_AICORE")
        self.assertEqual(ids, f"1,2,{str(NumberConstant.DEFAULT_GE_CONTEXT_ID)}")

    def test_get_context_ids_should_return_valid_ids_when_node_contains_mutil_context_ids(self):
        gear = TaskGear("")
        node_event = Event.invalid_event()
        node_event.struct_type = "1"
        context_id_info1 = CtxIdDto()
        context_id_info1.ctx_id = "1,2"
        addition_info1 = AdditionalRecord(context_id_info1)
        context_id_info2 = CtxIdDto()
        context_id_info2.ctx_id = "3,4"
        addition_info2 = AdditionalRecord(context_id_info2)
        node_event.additional_record = [addition_info1, addition_info2]
        call_stack = {Constant.NODE_LEVEL: node_event, Constant.HCCL_LEVEL: Event.invalid_event()}
        ids = gear.get_context_ids(call_stack, "KERNEL_AICORE")
        self.assertEqual(ids, f"1,2,3,4,{str(NumberConstant.DEFAULT_GE_CONTEXT_ID)}")

    def test_get_context_ids_should_return_valid_ids_when_node_contains_mutil_context_ids_hccl_contains_context(self):
        gear = TaskGear("")
        node_event = Event.invalid_event()
        node_event.struct_type = "1"
        context_id_info1 = CtxIdDto()
        context_id_info1.ctx_id = "1,2"
        addition_info1 = AdditionalRecord(context_id_info1)
        context_id_info2 = CtxIdDto()
        context_id_info2.ctx_id = "3,4"
        addition_info2 = AdditionalRecord(context_id_info2)
        node_event.additional_record = [addition_info1, addition_info2]

        hccl_event = Event.invalid_event()
        hccl_event.struct_type = "1"
        context_id_info3 = CtxIdDto()
        context_id_info3.ctx_id = "5,8"
        addition_info3 = AdditionalRecord(context_id_info3)
        hccl_event.additional_record = [addition_info3]

        call_stack = {Constant.NODE_LEVEL: node_event, Constant.HCCL_LEVEL: hccl_event}
        ids = gear.get_context_ids(call_stack, "KERNEL_AICORE")
        self.assertEqual(ids, f"1,2,3,4,5,6,7,8,{str(NumberConstant.DEFAULT_GE_CONTEXT_ID)}")

    def test_get_context_ids_should_return_empty_when_hccl_context_ids_length_is_not_2(self):
        gear = TaskGear("")
        hccl_event = Event.invalid_event()
        hccl_event.struct_type = "1"
        context_id_info3 = CtxIdDto()
        context_id_info3.ctx_id = "5,8,10"
        addition_info3 = AdditionalRecord(context_id_info3)
        hccl_event.additional_record = [addition_info3]
        ids = gear.get_context_ids_in_hccl(hccl_event)
        self.assertEqual(ids, [])

    def test_get_hccl_descs_should_return_default_hccl_desc_when_no_context_id_no_hccl_info(self):
        gear = TaskGear("")
        hccl_event = Event.invalid_event()
        descs = gear.get_hccl_descs(hccl_event)
        self.assertEqual(len(descs), 1)
        self.assertEqual(descs.get(NumberConstant.DEFAULT_GE_CONTEXT_ID).ctx_info.ctx_id,
                         str(NumberConstant.DEFAULT_GE_CONTEXT_ID))

    def test_get_hccl_descs_should_return_1_descs_when_traditional_task(self):
        gear = TaskGear("")
        hccl_event = Event.invalid_event()
        hccl_info = HCCLInfoDto()
        addition_info = AdditionalRecord(hccl_info)
        hccl_event.additional_record = [addition_info]

        descs = gear.get_hccl_descs(hccl_event)
        self.assertEqual(len(descs), 1)

    def test_get_hccl_descs_should_return_3_descs_when_ffts_task_3_context_id_and_3_hccl_info(self):
        gear = TaskGear("")
        hccl_event = Event.invalid_event()
        hccl_event.struct_type = "1"
        hccl_info1 = HCCLInfoDto()
        hccl_info1.context_id = 1
        hccl_info2 = HCCLInfoDto()
        hccl_info2.context_id = 2
        hccl_info3 = HCCLInfoDto()
        hccl_info3.context_id = 3
        context_id_info3 = CtxIdDto()
        context_id_info3.ctx_id = "1,3"
        hccl_event.additional_record = [
            AdditionalRecord(hccl_info1), AdditionalRecord(hccl_info2),
            AdditionalRecord(hccl_info3), AdditionalRecord(context_id_info3),
            AdditionalRecord("1")
        ]

        descs = gear.get_hccl_descs(hccl_event)
        self.assertEqual(len(descs), 3)
        self.assertEqual(descs.get(1).hccl_info.context_id, 1)
        self.assertEqual(descs.get(1).ctx_info.ctx_id, "1")
        self.assertEqual(descs.get(2).hccl_info.context_id, 2)
        self.assertEqual(descs.get(2).ctx_info.ctx_id, "2")
        self.assertEqual(descs.get(3).hccl_info.context_id, 3)
        self.assertEqual(descs.get(3).ctx_info.ctx_id, "3")

    def test_add_hccl_task_should_add_1_task_when_no_hccl_descs(self):
        gear = TaskGear("")
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        hccl_event = Event.invalid_event()
        model_event = Event.invalid_event()
        task_track_dto: TaskTrackDto = TaskTrackDto()
        call_stack = {Constant.MODEL_LEVEL: model_event, Constant.NODE_LEVEL: Event.invalid_event()}

        RTAddInfoCenter("./test")

        gear.add_hccl_task(call_stack, hccl_event, task_track_dto)
        self.assertEqual(len(gear.hccl_task_info), 1)

    def test_add_hccl_task_should_add_3_task_when_get_3_hccl_descs(self):
        gear = TaskGear("")
        api_db = ApiDataDatabase(1)
        record_db = AdditionalRecordDatabase(1)
        db = CANNThreadDB(1, api_db=api_db, record_db=record_db)
        gear.set_db(db)
        hccl_event = Event.invalid_event()
        model_event = Event.invalid_event()
        task_track_dto: TaskTrackDto = TaskTrackDto()
        call_stack = {Constant.MODEL_LEVEL: model_event, Constant.NODE_LEVEL: Event.invalid_event()}
        hccl_descs = collections.OrderedDict([("1", gear.HcclDesc()), ("2", gear.HcclDesc()), ("3", gear.HcclDesc())])

        RTAddInfoCenter("./test")

        with mock.patch(NAMESPACE + '.TaskGear.get_hccl_descs', return_value=hccl_descs):
            gear.add_hccl_task(call_stack, hccl_event, task_track_dto)
            self.assertEqual(len(gear.hccl_task_info), 3)

    def test_is_kernel_task_should_return_false_when_invalid_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        self.assertFalse(gear.is_kernel_task(task_track, True))

    def test_is_kernel_task_should_return_ture_when_kernel_prefix_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        task_track.task_type = "KERNEL_AICPU"
        self.assertTrue(gear.is_kernel_task(task_track, True))

    def test_is_kernel_task_should_return_ture_when_stars_common_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        task_track.task_type = "STARS_COMMON"
        self.assertTrue(gear.is_kernel_task(task_track, True))

    def test_is_kernel_task_should_return_false_when_hccl_ffts_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        task_track.task_type = "FFTS_PLUS"
        self.assertFalse(gear.is_kernel_task(task_track, False))

    def test_is_kernel_task_should_return_true_when_not_hccl_ffts_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        task_track.task_type = "FFTS_PLUS"
        self.assertTrue(gear.is_kernel_task(task_track, True))

    def test_is_kernel_task_should_return_false_when_other_dto(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        task_track.task_type = "MM"
        self.assertFalse(gear.is_kernel_task(task_track, True))

    def test_is_hccl_task_should_return_false_when_hccl_invalid(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        hccl_event = Event.invalid_event()
        self.assertFalse(gear.is_hccl_task(hccl_event, task_track))

    def test_is_hccl_task_should_return_false_when_task_track_invalid(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = None
        hccl_event = Event.invalid_event()
        hccl_event.struct_type = "2"
        self.assertFalse(gear.is_hccl_task(hccl_event, task_track))

    def test_is_hccl_task_should_return_true_when_correct_call_stack(self):
        gear = TaskGear("")
        task_track = TaskTrackDto()
        task_track.struct_type = "1"
        hccl_event = Event.invalid_event()
        hccl_event.struct_type = "2"
        self.assertTrue(gear.is_hccl_task(hccl_event, task_track))


if __name__ == '__main__':
    unittest.main()
