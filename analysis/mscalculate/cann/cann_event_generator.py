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

# pylint: disable=duplicate-code

import logging
from typing import List

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_object import HighPerfDict
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.event import Event
from mscalculate.cann.event_queue import EventQueue
from msmodel.add_info.ctx_id_model import CtxIdModel
from msmodel.add_info.fusion_add_info_model import FusionAddInfoModel
from msmodel.add_info.graph_add_info_model import GraphAddInfoModel
from msmodel.add_info.hccl_info_model import HcclInfoModel
from msmodel.add_info.tensor_add_info_model import TensorAddInfoModel
from msmodel.api.api_data_model import ApiDataModel
from msmodel.compact_info.hccl_op_info_model import HcclOpInfoModel
from msmodel.compact_info.memcpy_info_model import MemcpyInfoModel
from msmodel.compact_info.node_basic_info_model import NodeBasicInfoModel
from msmodel.compact_info.node_attr_info_model import NodeAttrInfoModel
from msmodel.compact_info.task_track_model import TaskTrackModel
from msmodel.event.event_data_model import EventDataModel
from profiling_bean.db_dto.api_data_dto import ApiDataDto, generate_api_data_from_event
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


class CANNThreadDB:
    def __init__(
        self,
        thread_id: int,
        event_q: EventQueue = EventQueue(-1),
        api_db: ApiDataDatabase = ApiDataDatabase(-1),
        record_db: AdditionalRecordDatabase = AdditionalRecordDatabase(-1),
    ):
        self.thread_id = thread_id
        self.event_q = event_q
        self.api_db = api_db
        self.record_db = record_db

    def get_api(self, event: Event) -> ApiDataDto:
        return self.api_db.get(event)

    def get_record(self, event: Event) -> AdditionalRecord:
        return self.record_db.get(event)

    def add_api(self, api_dto: ApiDataDto) -> Event:
        return self.api_db.put(api_dto)

    def get_time_bound(self) -> int:
        return self.event_q.get_max_bound()


class CANNEventGenerator:
    """
    generate events for cann analysis
    """

    def __init__(self, project_path):
        self._project_path = project_path
        self.event_queues: HighPerfDict[int, EventQueue] = HighPerfDict()
        self.api_databases: HighPerfDict[int, ApiDataDatabase] = HighPerfDict()
        self.record_databases: HighPerfDict[int, AdditionalRecordDatabase] = HighPerfDict()

        self.api_data_model = ApiDataModel(self._project_path)
        self.event_data_model = EventDataModel(self._project_path)
        self.node_basic_info_model = NodeBasicInfoModel(self._project_path)
        self.node_attr_info_model = NodeAttrInfoModel(self._project_path)
        self.tensor_info_model = TensorAddInfoModel(self._project_path)
        self.task_track_model = TaskTrackModel(self._project_path, [DBNameConstant.TABLE_TASK_TRACK])
        self.mem_copy_model = MemcpyInfoModel(self._project_path)
        self.ctx_id_model = CtxIdModel(self._project_path)
        self.hccl_info_model = HcclInfoModel(self._project_path)
        self.graph_id_map_model = GraphAddInfoModel(self._project_path)
        self.fusion_op_info_model = FusionAddInfoModel(self._project_path)
        self.hccl_op_info_model = HcclOpInfoModel(self._project_path)

    @staticmethod
    def is_kernel_api(api_data_dto: ApiDataDto) -> bool:
        if api_data_dto.struct_type == "StreamSyncTaskFinish":
            return False
        if api_data_dto.start <= 0 or api_data_dto.start == api_data_dto.end:
            return False
        return True

    def record_additional_info(self, infos):
        for info in infos:
            record = AdditionalRecord(info, info.timestamp, info.struct_type)
            thread = info.thread_id
            event = self.record_databases.set_default_call_obj_later(thread, AdditionalRecordDatabase, thread).put(
                record
            )
            self.event_queues.set_default_call_obj_later(event.thread_id, EventQueue, event.thread_id).add(event)

    def collect_time_period_data(self):
        api_data, event_data = [], []
        if not self.api_data_model.check_db():
            return api_data, event_data
        with self.api_data_model as api_model:
            api_data = api_model.get_all_data(DBNameConstant.TABLE_API_DATA, ApiDataDto)

        if not self.event_data_model.check_db():
            return api_data, event_data
        with self.event_data_model as event_model:
            event_data = event_model.get_all_data(DBNameConstant.TABLE_EVENT_DATA, EventDataDto)
        return api_data, event_data

    def generate_api_event(self):
        api_data, event_data = self.collect_time_period_data()
        for api_data_dto in api_data:
            # special api start equal end, abandon
            if not self.is_kernel_api(api_data_dto):
                continue
            event = self.api_databases.set_default_call_obj_later(
                api_data_dto.thread_id, ApiDataDatabase, api_data_dto.thread_id
            ).put(api_data_dto)
            self.event_queues.set_default_call_obj_later(api_data_dto.thread_id, EventQueue, event.thread_id).add(event)

        event_logger = dict()
        for event_data_dto in event_data:
            # 先匹配，再生成event
            friend_timestamp = event_logger.get(
                (
                    event_data_dto.level,
                    event_data_dto.thread_id,
                    event_data_dto.struct_type,
                    event_data_dto.request_id,
                    event_data_dto.item_id,
                ),
                None,
            )
            if not friend_timestamp:
                event_logger[
                    (
                        event_data_dto.level,
                        event_data_dto.thread_id,
                        event_data_dto.struct_type,
                        event_data_dto.request_id,
                        event_data_dto.item_id,
                    )
                ] = event_data_dto.timestamp
                continue
            # represent 2 event with an api
            equal_api = generate_api_data_from_event(friend_timestamp, event_data_dto)
            event = self.api_databases.set_default_call_obj_later(
                equal_api.thread_id, ApiDataDatabase, equal_api.thread_id
            ).put(equal_api)
            self.event_queues.set_default_call_obj_later(event.thread_id, EventQueue, event.thread_id).add(event)
            # for sub graph scene
            event_logger.pop(
                (
                    event_data_dto.level,
                    event_data_dto.thread_id,
                    event_data_dto.struct_type,
                    event_data_dto.request_id,
                    event_data_dto.item_id,
                )
            )
        if event_logger:
            for key in event_logger:
                logging.error(
                    "No matched event found for (level, thread_id, struct_type, request_id, item_id): %s", str(key)
                )

    def generate_node_basic_info_event(self):
        if not self.node_basic_info_model.check_db():
            return
        with self.node_basic_info_model as node_basic_model:
            node_basic_infos = node_basic_model.get_all_data(DBNameConstant.TABLE_NODE_BASIC_INFO, NodeBasicInfoDto)
            self.record_additional_info(node_basic_infos)

    def generate_node_attr_info_event(self):
        if not self.node_attr_info_model.check_db():
            return
        with self.node_attr_info_model as node_attr_model:
            node_attr_infos = node_attr_model.get_all_data(DBNameConstant.TABLE_NODE_ATTR_INFO, NodeAttrInfoDto)
            self.record_additional_info(node_attr_infos)

    def generate_tensor_info_event(self):
        if not self.tensor_info_model.check_db():
            return
        with self.tensor_info_model as tensor_info_model:
            tensor_infos = tensor_info_model.get_all_data(DBNameConstant.TABLE_TENSOR_ADD_INFO, TensorInfoDto)
            self.record_additional_info(tensor_infos)

    def generate_task_track_event(self):
        if not self.task_track_model.check_db():
            return
        with self.task_track_model as task_track_model:
            task_tracks = task_track_model.get_all_data(DBNameConstant.TABLE_TASK_TRACK, TaskTrackDto)
            self.record_additional_info(task_tracks)

    def generate_mem_copy_info_event(self):
        if not self.mem_copy_model.check_db():
            return
        with self.mem_copy_model as mem_copy_model:
            mem_copy_infos = mem_copy_model.get_all_data(DBNameConstant.TABLE_MEMCPY_INFO, MemCopyInfoDto)
            self.record_additional_info(mem_copy_infos)

    def generate_ctx_id_event(self):
        if not self.ctx_id_model.check_db():
            return
        with self.ctx_id_model as ctx_id_model:
            ctx_ids = ctx_id_model.get_all_data(DBNameConstant.TABLE_CTX_ID_ADD_INFO, CtxIdDto)
            self.record_additional_info(ctx_ids)

    def generate_hccl_info_event(self):
        if not self.hccl_info_model.check_db():
            return
        with self.hccl_info_model as hccl_info_model:
            hccl_infos = hccl_info_model.get_all_data(DBNameConstant.TABLE_HCCL_INFO_DATA, HCCLInfoDto)
            self.record_additional_info(hccl_infos)

    def generate_graph_id_map_event(self):
        if not self.graph_id_map_model.check_db():
            return
        with self.graph_id_map_model as graph_id_model:
            graph_id_map_infos = graph_id_model.get_all_data(DBNameConstant.TABLE_GRAPH_ADD_INFO, GraphIdMapDto)
            self.record_additional_info(graph_id_map_infos)

    def generate_fusion_op_info_event(self):
        if not self.fusion_op_info_model.check_db():
            return
        with self.fusion_op_info_model as fusion_op_model:
            fusion_op_infos = fusion_op_model.get_all_data(DBNameConstant.TABLE_FUSION_ADD_INFO, FusionOpInfoDto)
            self.record_additional_info(fusion_op_infos)

    def generate_hccl_op_info_event(self):
        if not self.hccl_op_info_model.check_db():
            return
        with self.hccl_op_info_model as hccl_op_info_model:
            hccl_op_infos = hccl_op_info_model.get_all_data(DBNameConstant.TABLE_HCCL_OP_INFO, HCCLOpInfoDto)
            self.record_additional_info(hccl_op_infos)

    def lock_queues(self):
        for queue in self.event_queues.values():
            queue.lock()

    def generate_storages(self) -> List[CANNThreadDB]:
        storages = []
        threads = list(self.event_queues.keys())
        for thread in threads:
            storages.append(
                CANNThreadDB(
                    thread,
                    self.event_queues.get(thread),
                    self.api_databases.get(thread, ApiDataDatabase(thread)),
                    self.record_databases.get(thread, AdditionalRecordDatabase(thread)),
                )
            )
        return storages

    def run(self) -> List[CANNThreadDB]:
        self.generate_api_event()

        self.generate_node_basic_info_event()
        self.generate_node_attr_info_event()
        self.generate_tensor_info_event()
        self.generate_task_track_event()
        self.generate_mem_copy_info_event()
        self.generate_graph_id_map_event()
        self.generate_fusion_op_info_event()
        self.generate_ctx_id_event()
        self.generate_hccl_op_info_event()
        self.generate_hccl_info_event()

        self.lock_queues()
        return self.generate_storages()
