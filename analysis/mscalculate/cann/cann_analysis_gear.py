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
# pylint: skip-file

import logging
from abc import abstractmethod
from collections import OrderedDict
from typing import List
from typing import Union

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.ge_enum_constant import GeTaskType
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_object import HighPerfDict
from common_func.platform.chip_manager import ChipManager
from common_func.rt_add_info_center import RTAddInfoCenter
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_event_generator import CANNThreadDB
from mscalculate.cann.event import Event
from msmodel.ge.ge_host_parser_model import GeHostParserModel
from msmodel.ge.ge_info_model import GeModel
from msmodel.ge.ge_model_load_model import GeFusionModel
from msmodel.hccl.hccl_host_model import HCCLHostModel
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.graph_id_map_dto import GraphIdMapDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.hccl_op_info_dto import HCCLOpInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.node_attr_info_dto import NodeAttrInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto


class CANNGear:
    """
    Gears, the basic components in the analysis chain of CANN software stacks,
    each instance processing data in one layer of CANN.
    1. Associate upper-layer and same-layer data.
    2. Generate the table structure.
    """

    INVALID_LEVEL = -1

    def __init__(self, project_path):
        self._project_path = project_path
        self.cann_level = self.INVALID_LEVEL
        self.db: CANNThreadDB = None

    @abstractmethod
    def run(self, event: Event, call_stack: dict):
        raise Exception("To be implement")

    @abstractmethod
    def flush_data(self):
        raise Exception("To be implement")

    def set_db(self, db: CANNThreadDB):
        self.db = db


class RootGear(CANNGear):
    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.ROOT_LEVEL

    def run(self, event: Event, call_stack: dict):
        pass

    def flush_data(self):
        pass


class ACLGear(CANNGear):
    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.ACL_LEVEL

    def run(self, event: Event, call_stack: dict):
        pass

    def flush_data(self):
        pass


class ModelGear(CANNGear):
    INVALID_MODEL_NAME = "NA"
    INVALID_REQUEST_ID = "NA"
    MODEL_TIME_STAGE = 3

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.MODEL_LEVEL
        self.model_id_name_table = {}
        self.model_name_data = []

    @classmethod
    def get_graph_id_map_dto(cls, event: Event) -> GraphIdMapDto:
        for record in event.additional_record:
            if isinstance(record.dto, GraphIdMapDto):
                return record.dto
        return GraphIdMapDto()

    def add_model_name(self, data: ApiDataDto, event: Event):
        graph_id_map_dto: GraphIdMapDto = self.get_graph_id_map_dto(event)
        if graph_id_map_dto.struct_type:
            model_name = graph_id_map_dto.model_name
            self.model_name_data.append([data.item_id, model_name])

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = self.db.get_api(event)
        # these two table can be merged in the feature
        if dto.struct_type == "ModelLoad" or dto.struct_type == "ModelExecute":
            self.add_model_name(dto, event)
        if not dto.struct_type:
            # lose some data
            for record in event.additional_record:
                if isinstance(record, GraphIdMapDto):
                    logging.warning(
                        "Lose model info for graph_id: %s, model_name: %s", record.graph_id, record.model_name
                    )

    def save_model_name(self: any) -> None:
        if not self.model_name_data:
            return
        model = GeFusionModel(self._project_path, [DBNameConstant.TABLE_MODEL_NAME])
        model.init()
        model.drop_table(DBNameConstant.TABLE_MODEL_NAME)
        model.create_table()
        model.flush_all({DBNameConstant.TABLE_MODEL_NAME: self.model_name_data})
        model.finalize()

    def flush_data(self):
        self.save_model_name()


class NodeGear(CANNGear):
    GE_STEP_INFO_API_TYPE = "step_info"

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.NODE_LEVEL
        self.node_name_type_table = {}
        self.ge_host_info = []
        self.hccl_op_info = []
        self.fusion_op_info = []

    @staticmethod
    def get_op_type_by_addition(additional_records: List[AdditionalRecord]):
        if not additional_records:
            return ""

        # static shape or dynamic shape
        for record in additional_records:
            if isinstance(record.dto, NodeBasicInfoDto):
                return record.dto.op_type
        return ""

    def record_fusion_op_info(self, dto: FusionOpInfoDto, call_stack: dict):
        model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
        model_dto: ApiDataDto = self.db.get_api(model_event)
        self.fusion_op_info.append(
            [
                model_dto.item_id,
                dto.op_name,
                dto.fusion_op_num,
                dto.fusion_op_names,
                dto.memory_input,
                dto.memory_output,
                dto.memory_weight,
                dto.memory_workspace,
                dto.memory_total,
            ]
        )

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = self.db.get_api(event)

        if not dto.struct_type:
            for record in event.additional_record:
                if isinstance(record.dto, FusionOpInfoDto):
                    self.record_fusion_op_info(record.dto, call_stack)
            return

        if dto.struct_type != self.GE_STEP_INFO_API_TYPE:
            # add ge host info
            op_type = self.get_op_type_by_addition(event.additional_record)
            if op_type:
                # record
                self.node_name_type_table[dto.item_id] = op_type
            self.ge_host_info.append([dto.thread_id, op_type, dto.struct_type, dto.start, dto.end, dto.item_id])

    def update_node_name(self):
        op_type_index = 1
        op_name_index = -1
        for ge_host_data in self.ge_host_info:
            ge_host_data[op_type_index] = self.node_name_type_table.get(ge_host_data[op_name_index], "")

    def save_ge_host_info(self):
        if not self.ge_host_info:
            return
        self.update_node_name()
        model = GeHostParserModel(self._project_path, DBNameConstant.DB_GE_HOST_INFO, [DBNameConstant.TABLE_GE_HOST])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_HOST)
        model.create_table()

        model.insert_data_to_db(DBNameConstant.TABLE_GE_HOST, self.ge_host_info)
        model.finalize()

    def save_fusion_op_info(self):
        if not self.fusion_op_info:
            return
        model = GeFusionModel(self._project_path, [DBNameConstant.TABLE_GE_FUSION_OP_INFO])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_FUSION_OP_INFO)
        model.create_table()

        model.insert_data_to_db(DBNameConstant.TABLE_GE_FUSION_OP_INFO, self.fusion_op_info)
        model.finalize()

    def flush_data(self):
        self.save_ge_host_info()
        self.save_fusion_op_info()


class TaskGear(CANNGear):
    INVALID_STREAM = 65535  # 翻转最大值65535做无效值
    INVALID_ID = -1
    INVALID_DIRECT = -1
    INVALID_CONTEXT_ID = 4294967295
    HCCL_CONTEXT_ID_NUM = 2
    KERNEL_TASK_PREFIX = "KERNEL"
    HCCL_TASK_TYPE = "COMMUNICATION"
    FFTS_PLUS_TASK_TYPE = "FFTS_PLUS"
    KERNEL_FFTS_PLUS_TASK_TYPE = "FFTS_PLUS"
    KERNEL_STARS_COMMON_TASK_TYPE = "STARS_COMMON"
    KERNEL_AICPU = "KERNEL_AICPU"
    KERNEL_AICORE = "KERNEL_AICORE"
    KERNEL_AIVEC = "KERNEL_AIVEC"
    KERNEL_MIX_AIC = "KERNEL_MIX_AIC"
    KERNEL_MIX_AIV = "KERNEL_MIX_AIV"
    CONTEXT_ID_WHITE_LIST = [KERNEL_AICORE, KERNEL_AIVEC, KERNEL_FFTS_PLUS_TASK_TYPE, KERNEL_MIX_AIC, KERNEL_MIX_AIV]
    RTS_TASK_TYPE_MAP = {
        KERNEL_AICORE: GeTaskType.AI_CORE.name,
        KERNEL_AIVEC: GeTaskType.AI_VECTOR_CORE.name,
        KERNEL_MIX_AIC: GeTaskType.MIX_AIC.name,
        KERNEL_MIX_AIV: GeTaskType.MIX_AIV.name,
        KERNEL_AICPU: GeTaskType.AI_CPU.name,
    }

    class RuntimeApi:
        def __init__(self, start, end, struct_type, thread_id):
            self.start = start
            self.end = end
            self.api = struct_type
            self.thread = thread_id
            self.stream_id = TaskGear.INVALID_STREAM
            self.task_id = TaskGear.INVALID_ID
            self.batch_id = TaskGear.INVALID_ID
            self.data_size = 0
            self.memcpy_direction = TaskGear.INVALID_DIRECT

        def to_list(self):
            return [
                self.start,
                self.end,
                self.api,
                self.thread,
                self.stream_id,
                self.task_id,
                self.batch_id,
                self.data_size,
                self.memcpy_direction,
            ]

    class NodeDesc:
        def __init__(
            self,
            node_basic_info=NodeBasicInfoDto(),
            tensor_info=TensorInfoDto(),
            ctx_info=CtxIdDto(),
            node_attr_info=NodeAttrInfoDto(),
        ):
            self.node_basic_info = node_basic_info
            self.tensor_info = tensor_info
            self.ctx_info = ctx_info
            self.node_attr_info = node_attr_info

        @staticmethod
        def get_hash(dto: Union[NodeBasicInfoDto, TensorInfoDto, CtxIdDto, NodeAttrInfoDto, HCCLOpInfoDto]):
            return dto.op_name + "-" + str(int(dto.timestamp))

        def is_invalid(self):
            return self.node_basic_info is not None and self.tensor_info is not None

    class HcclDesc:
        def __init__(self, hccl_info=HCCLInfoDto(), ctx_info=CtxIdDto()):
            self.hccl_info = hccl_info
            self.ctx_info = ctx_info

        @staticmethod
        def get_hash(dto: Union[HCCLInfoDto, CtxIdDto]):
            return dto.context_id

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.TASK_LEVEL
        self.task_info = []
        self.tensor_info = []
        self.hccl_task_info = []
        self.host_tasks = []
        self.mismatch_hccl = 0
        self.hccl_op_info = []
        self.hccl_node_keys = set()
        self.hccl_node_mismatch = 0

    @staticmethod
    def get_task_level_additional_dto(event: Event) -> tuple:
        mem_cpy_info_dto = MemCopyInfoDto()
        task_track_dtos = []
        for record in event.additional_record:
            if isinstance(record.dto, MemCopyInfoDto):
                mem_cpy_info_dto = record.dto
            elif isinstance(record.dto, TaskTrackDto):
                task_track_dtos.append(record.dto)
        return mem_cpy_info_dto, task_track_dtos

    @classmethod
    def get_context_ids_in_node(cls: any, node_event: Event) -> list:
        if node_event.is_invalid():
            return []
        ids = []
        for record in node_event.additional_record:
            if isinstance(record.dto, CtxIdDto):
                ids.append(record.dto.ctx_id)
        return ids

    @classmethod
    def get_context_ids_in_hccl(cls: any, hccl_event: Event) -> list:
        if hccl_event.is_invalid():
            return []
        ids = []
        # context id in hccl level will only report one add info which in the add queue end
        for record in reversed(hccl_event.additional_record):
            if isinstance(record.dto, CtxIdDto):
                ids.extend(record.dto.ctx_id.split(','))
                break
        if ids:
            if len(ids) != cls.HCCL_CONTEXT_ID_NUM:
                logging.error("Illegal context id size, except: %d, found: %d", cls.HCCL_CONTEXT_ID_NUM, len(ids))
                return []
            return [str(_id) for _id in list(range(int(ids[0]), int(ids[1]) + 1))]
        return ids

    @classmethod
    def get_context_ids(cls, call_stack: dict, task_type: str) -> str:
        default_ids = [str(NumberConstant.DEFAULT_GE_CONTEXT_ID)]
        if task_type in [cls.KERNEL_MIX_AIC, cls.KERNEL_MIX_AIV]:
            default_ids.append("0")
        node_context_ids = cls.get_context_ids_in_node(call_stack.get(Constant.NODE_LEVEL))
        hccl_context_ids = cls.get_context_ids_in_hccl(call_stack.get(Constant.HCCL_LEVEL))
        context_ids = list(OrderedDict.fromkeys(node_context_ids + hccl_context_ids + default_ids))

        return ",".join(context_ids)

    def get_node_descs(self, event: Event) -> dict:
        node_descs = HighPerfDict()
        for record in event.additional_record:
            if isinstance(record.dto, NodeBasicInfoDto):
                node_desc = node_descs.set_default_call_obj_later(event.id, self.NodeDesc)
                node_desc.node_basic_info = record.dto
            elif isinstance(record.dto, TensorInfoDto):
                node_desc = node_descs.set_default_call_obj_later(event.id, self.NodeDesc)
                node_desc.tensor_info = record.dto
            elif isinstance(record.dto, CtxIdDto):
                node_desc = node_descs.set_default_call_obj_later(event.id, self.NodeDesc)
                node_desc.ctx_info = record.dto
            elif isinstance(record.dto, NodeAttrInfoDto):
                node_desc = node_descs.set_default_call_obj_later(event.id, self.NodeDesc)
                node_desc.node_attr_info = record.dto
            elif isinstance(record.dto, HCCLOpInfoDto):
                continue
            else:
                logging.error("Unsupported additional type: %s in node level.", type(record.dto))
        return node_descs

    def get_hccl_descs(self, event: Event) -> OrderedDict:
        hccl_descs = HighPerfDict()
        for record in event.additional_record:
            if isinstance(record.dto, HCCLInfoDto):
                hccl_desc = hccl_descs.set_default_call_obj_later(self.HcclDesc.get_hash(record.dto), self.HcclDesc)
                hccl_desc.hccl_info = record.dto
            elif isinstance(record.dto, CtxIdDto):
                context_ids = self.get_context_ids_in_hccl(event)
                for context_id in context_ids:
                    hccl_desc = hccl_descs.set_default_call_obj_later(int(context_id), self.HcclDesc)
                    ctx_dto = CtxIdDto()
                    ctx_dto.ctx_id = context_id
                    hccl_desc.ctx_info = ctx_dto
            else:
                logging.error("Unsupported additional type: %s in hccl level.", type(record.dto))
        if not hccl_descs:
            hccl_descs[NumberConstant.DEFAULT_GE_CONTEXT_ID] = self.HcclDesc()
        return hccl_descs

    def get_model_id(self, rts_trk: TaskTrackDto, model_dto: ApiDataDto):
        if model_dto.item_id != "":
            return model_dto.item_id
        model_id, _ = RTAddInfoCenter().get_op_info_by_id(
            rts_trk.device_id, rts_trk.stream_id, rts_trk.task_id, rts_trk.batch_id, rts_trk.timestamp
        )
        return model_id

    def add_host_task(self, call_stack: dict, task_track_dto: TaskTrackDto):
        model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
        model_dto: ApiDataDto = self.db.get_api(model_event)
        node_event: Event = call_stack.get(Constant.NODE_LEVEL)
        node_dto: ApiDataDto = self.db.get_api(node_event)
        acl_event: Event = call_stack.get(Constant.ACL_LEVEL)
        acl_dto: ApiDataDto = self.db.get_api(acl_event)

        model_id = self.get_model_id(task_track_dto, model_dto)
        request_id = model_dto.request_id if model_dto.request_id is not None else -1
        # 根据task type是否在白名单内对context_ids和connection_id进行处理，以应对Node@Launch下有多个Task的问题
        # 对于在白名单内的task正常生成context_ids，反之使用对应的默认值
        if task_track_dto.task_type in self.CONTEXT_ID_WHITE_LIST and not ChipManager().is_chip_v6():
            context_ids = self.get_context_ids(call_stack, task_track_dto.task_type)
        else:
            context_ids = str(NumberConstant.DEFAULT_GE_CONTEXT_ID)

        connection_id = Constant.DEFAULT_INVALID_VALUE
        if node_dto.connection_id is not None:
            connection_id = node_dto.connection_id
        elif acl_dto.connection_id is not None:
            connection_id = acl_dto.connection_id

        self.host_tasks.append(
            [
                model_id,
                request_id,
                task_track_dto.stream_id,
                task_track_dto.task_id,
                context_ids,
                task_track_dto.batch_id,
                task_track_dto.task_type,
                task_track_dto.kernel_name,
                task_track_dto.device_id,
                task_track_dto.timestamp,
                connection_id,
            ]
        )

    def is_hccl_task(self, hccl_event: Event, task_track_dto: TaskTrackDto):
        if hccl_event.is_invalid():
            return False
        if task_track_dto.struct_type is None:
            return False
        return True

    def add_hccl_task(self, model_event: Event, hccl_event: Event, task_track_dto: TaskTrackDto):
        hccl_descs = self.get_hccl_descs(hccl_event)
        model_dto: ApiDataDto = self.db.get_api(model_event)

        model_id = self.get_model_id(task_track_dto, model_dto)
        request_id = model_dto.request_id if model_dto.request_id is not None else -1

        hccl_tasks = [0] * len(hccl_descs)
        for i, hccl_desc in enumerate(hccl_descs.values()):
            hccl_info_dto = hccl_desc.hccl_info
            context_id = int(hccl_desc.ctx_info.ctx_id)
            is_master = 1 if hccl_event.struct_type == 'master' else 0
            notify_id = hccl_info_dto.notify_id if int(hccl_info_dto.notify_id) != -1 else Constant.NA
            hccl_tasks[i] = [
                model_id,
                request_id,
                hccl_info_dto.op_name,
                hccl_info_dto.group_name,
                hccl_info_dto.plane_id,
                task_track_dto.timestamp,
                hccl_info_dto.duration_estimated,
                task_track_dto.stream_id,
                task_track_dto.task_id,
                context_id,
                task_track_dto.batch_id,
                task_track_dto.device_id,
                is_master,
                hccl_info_dto.local_rank,
                hccl_info_dto.remote_rank,
                hccl_info_dto.transport_type,
                hccl_info_dto.size,
                hccl_info_dto.data_type,
                hccl_info_dto.link_type,
                notify_id,
                hccl_info_dto.rdma_type,
                task_track_dto.thread_id,
                hccl_info_dto.rank_size,
            ]
        self.hccl_task_info.extend(hccl_tasks)

    def get_kfc_connection_id(self: any, node_event: Event):
        if node_event is None:
            return Constant.DEFAULT_INVALID_VALUE
        kfc_node_event = node_event.kfc_node_event
        node_dto: ApiDataDto = self.db.get_api(kfc_node_event)
        connection_id = node_dto.connection_id if node_dto.connection_id is not None else Constant.DEFAULT_INVALID_VALUE
        return connection_id

    def add_hccl_op(self, call_stack: dict, task_track_dto: TaskTrackDto):
        node_event: Event = call_stack.get(Constant.NODE_LEVEL)
        node_dto: ApiDataDto = self.db.get_api(node_event)

        if node_event.is_invalid():
            self.hccl_node_mismatch += 1
            return

        hccl_node_key = node_dto.item_id + str(node_dto.thread_id) + str(node_dto.end)
        if hccl_node_key in self.hccl_node_keys:
            return
        self.hccl_node_keys.add(hccl_node_key)

        model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
        model_dto: ApiDataDto = self.db.get_api(model_event)
        request_id = model_dto.request_id if model_dto.request_id is not None else -1
        model_id = self.get_model_id(task_track_dto, model_dto)
        connection_id = node_dto.connection_id if node_dto.connection_id is not None else Constant.DEFAULT_INVALID_VALUE
        kfc_connection_id = self.get_kfc_connection_id(node_event)

        node_basic_info = [
            task_track_dto.device_id,
            model_id,
            request_id,
            task_track_dto.thread_id,
            node_dto.item_id,
            self.HCCL_TASK_TYPE,
            Constant.NA,
            node_dto.start,
            node_dto.end,
            Constant.NA,
            connection_id,
            kfc_connection_id,
        ]
        hccl_op_info = [
            Constant.DEFAULT_INVALID_VALUE,
            Constant.DEFAULT_INVALID_VALUE,
            Constant.NA,
            Constant.NA,
            Constant.DEFAULT_INVALID_VALUE,
            Constant.NA,
        ]

        if not node_event.additional_record:
            self.hccl_op_info.append(node_basic_info + hccl_op_info)
            return

        has_hccl_op_info = False
        for record in node_event.additional_record:
            if isinstance(record.dto, NodeBasicInfoDto):
                node_basic_info = [
                    task_track_dto.device_id,
                    model_id,
                    request_id,
                    node_dto.thread_id,
                    node_dto.item_id,
                    self.HCCL_TASK_TYPE,
                    record.dto.op_type,
                    node_dto.start,
                    node_dto.end,
                    record.dto.is_dynamic,
                    connection_id,
                    kfc_connection_id,
                ]
            if isinstance(record.dto, HCCLOpInfoDto):
                hccl_op_info = [
                    record.dto.relay,
                    record.dto.retry,
                    record.dto.data_type,
                    record.dto.alg_type,
                    record.dto.count,
                    record.dto.group_name,
                ]
                has_hccl_op_info = True
        if not has_hccl_op_info:
            logging.error("Not report hccl op info for api: %s", node_dto.item_id)
        self.hccl_op_info.append(node_basic_info + hccl_op_info)

    def is_kernel_task(self, task_track_dto: TaskTrackDto, is_not_hccl_task: bool) -> bool:
        if task_track_dto.struct_type is None:
            return False

        # In these scene, tasks are thought to be kernel tasks
        # traditional core task
        if task_track_dto.task_type.startswith(self.KERNEL_TASK_PREFIX):
            return True
        # traditional dsa task
        if task_track_dto.task_type == self.KERNEL_STARS_COMMON_TASK_TYPE:
            return True
        # ffts+ task
        if task_track_dto.task_type == self.KERNEL_FFTS_PLUS_TASK_TYPE:
            return is_not_hccl_task
        return False

    def get_truth_task_type_for_kernel_hccl_task(self, add_dto):
        task_type = Constant.TASK_TYPE_COMMUNICATION
        if add_dto.task_type == self.KERNEL_AICPU:
            # helper场景: HCCL算子运行在AI_CPU上
            task_type = Constant.TASK_TYPE_HCCL_AI_CPU
        if add_dto.task_type == self.KERNEL_AICORE:
            # Reduce tbe
            task_type = Constant.TASK_TYPE_COMMUNICATION
        return task_type

    def add_kernel_task(self, call_stack: dict, add_dto: TaskTrackDto, is_level0: bool):
        node_event: Event = call_stack.get(Constant.NODE_LEVEL)
        node_dto: ApiDataDto = self.db.get_api(node_event)
        hccl_event: Event = call_stack.get(Constant.HCCL_LEVEL)
        hccl_dto: ApiDataDto = self.db.get_api(hccl_event)

        if not node_dto.item_id and not hccl_dto.item_id:
            # this happens when runtime task is not respond to a op; or only runtime task
            self.add_kernel_task_only_task_track(add_dto, is_level0)
            return

        if node_dto.item_id.startswith("Lccl"):
            return

        model_dto: ApiDataDto = self.db.get_api(call_stack.get(Constant.MODEL_LEVEL))
        model_id = self.get_model_id(add_dto, model_dto)
        request_id = model_dto.request_id if model_dto.request_id is not None else -1

        if is_level0:
            # this happens when prof data is collected in level 0,
            # or hccl (reduce TBE op) op which is not same thread with node launch. 此场景在当前分支下失效 待具体数据验证
            self.add_kernel_task_l0(
                [node_dto, self.NodeDesc()], add_dto, [hccl_event, hccl_dto], [model_id, request_id]
            )
            return

        node_descs = self.get_node_descs(node_event)
        if not node_descs:
            logging.error("Can't find node desc for api: %s, timestamp is %d", node_dto.item_id, add_dto.timestamp)
            return

        for node_desc in node_descs.values():
            node_basic_info_dto: NodeBasicInfoDto = node_desc.node_basic_info

            if node_basic_info_dto.task_type is None:
                # this happens when prof data is collected in level 0 with ffts plus
                self.add_kernel_task_l0([node_dto, node_desc], add_dto, [hccl_event, hccl_dto], [model_id, request_id])
                continue
            if node_basic_info_dto.task_type == self.FFTS_PLUS_TASK_TYPE:
                continue

            self.add_kernel_task_l1_or_l2(
                [node_dto, node_desc], add_dto, [hccl_event, hccl_dto], [model_id, request_id]
            )

    def add_kernel_task_l0(self, node_info: list, add_dto: TaskTrackDto, hccl_info: list, model_info: list):
        node_dto = node_info[0]
        node_desc = node_info[1]
        hccl_event = hccl_info[0]
        hccl_dto = hccl_info[1]
        model_id = model_info[0]
        request_id = model_info[1]
        ctx_id_dto: CtxIdDto = node_desc.ctx_info
        cxt_ids = str(ctx_id_dto.ctx_id).split(',')
        op_name = ctx_id_dto.op_name if ctx_id_dto.op_name else node_dto.item_id
        task_type = Constant.NA
        if self.is_hccl_task(hccl_event, add_dto):
            op_name = hccl_dto.item_id
            task_type = self.get_truth_task_type_for_kernel_hccl_task(add_dto)
        for cxt_id in cxt_ids:
            self.task_info.append(
                [
                    model_id,
                    op_name,
                    add_dto.stream_id,
                    add_dto.task_id,
                    0,
                    0,
                    Constant.NA,
                    task_type,
                    Constant.NA,
                    request_id,
                    add_dto.thread_id,
                    add_dto.timestamp,
                    add_dto.batch_id,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    add_dto.device_id,
                    int(cxt_id),
                    Constant.NA,
                    Constant.NA,
                ]
            )

    def add_kernel_task_l1_or_l2(self, node_info: list, add_dto: TaskTrackDto, hccl_info: list, model_info: list):
        # 采集开启l2开关之后，需要在建树的逻辑里面添加attr的相关信息
        node_dto = node_info[0]
        node_desc = node_info[1]
        hccl_event = hccl_info[0]
        hccl_dto = hccl_info[1]
        model_id = model_info[0]
        request_id = model_info[1]
        node_basic_info_dto: NodeBasicInfoDto = node_desc.node_basic_info
        tensor_info_dto: TensorInfoDto = node_desc.tensor_info
        ctx_id_dto: CtxIdDto = node_desc.ctx_info
        node_attr_info: NodeAttrInfoDto = node_desc.node_attr_info
        cxt_ids = str(ctx_id_dto.ctx_id).split(',')
        op_name = ctx_id_dto.op_name if ctx_id_dto.op_name else node_dto.item_id
        task_type = node_basic_info_dto.task_type
        if self.is_hccl_task(hccl_event, add_dto):
            # notice: reduce TBE op
            op_name = hccl_dto.item_id
            task_type = self.get_truth_task_type_for_kernel_hccl_task(add_dto)
        elif (
            task_type in (Constant.TASK_TYPE_COMMUNICATION, Constant.TASK_TYPE_HCCL)
            and add_dto.task_type == self.KERNEL_AICPU
        ):
            # helper场景, HCCL算子运行在AI_CPU上, 但没有HCCL层api
            task_type = Constant.TASK_TYPE_AI_CPU
        for cxt_id in cxt_ids:
            self.task_info.append(
                [
                    model_id,
                    op_name,
                    add_dto.stream_id,
                    add_dto.task_id,
                    node_basic_info_dto.block_num,
                    node_basic_info_dto.mix_block_num,
                    node_basic_info_dto.is_dynamic,
                    task_type,
                    node_basic_info_dto.op_type,
                    request_id,
                    add_dto.thread_id,
                    add_dto.timestamp,
                    add_dto.batch_id,
                    tensor_info_dto.tensor_num,
                    tensor_info_dto.input_formats,
                    tensor_info_dto.input_data_types,
                    tensor_info_dto.input_shapes,
                    tensor_info_dto.output_formats,
                    tensor_info_dto.output_data_types,
                    tensor_info_dto.output_shapes,
                    add_dto.device_id,
                    int(cxt_id),
                    "YES" if node_basic_info_dto.op_flag else "NO",
                    Constant.NA if not node_attr_info.hashid else node_attr_info.hashid,
                ]
            )

    def add_kernel_task_only_task_track(self, rts_trk: TaskTrackDto, is_level0: bool):
        _, op_info = RTAddInfoCenter().get_op_info_by_id(
            rts_trk.device_id, rts_trk.stream_id, rts_trk.task_id, rts_trk.batch_id, rts_trk.timestamp
        )
        if rts_trk.kernel_name == Constant.NA and not op_info.is_valid:
            return

        if op_info.is_valid:
            task_type = Constant.NA if is_level0 else op_info.task_type
            op_type = Constant.NA if is_level0 else op_info.op_type
            op_name = op_info.op_name
        else:
            task_type = Constant.NA if is_level0 else self.RTS_TASK_TYPE_MAP.get(rts_trk.task_type, rts_trk.task_type)
            op_type = Constant.NA if is_level0 else rts_trk.kernel_name
            op_name = rts_trk.kernel_name

        request_id = -1
        context_id = (
            0
            if (task_type in [GeTaskType.MIX_AIC.name, GeTaskType.MIX_AIV.name] and not ChipManager().is_chip_v6())
            else self.INVALID_CONTEXT_ID
        )  # op_flag在level0 或者无补充信息时填NA
        op_flag = Constant.NA if (is_level0 or not op_info.is_valid) else ("YES" if op_info.op_flag else "NO")

        if is_level0:
            self.task_info.append(
                [
                    op_info.model_id,
                    op_name,
                    rts_trk.stream_id,
                    rts_trk.task_id,
                    0,
                    0,
                    Constant.NA,
                    task_type,
                    op_type,
                    request_id,
                    rts_trk.thread_id,
                    rts_trk.timestamp,
                    rts_trk.batch_id,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    rts_trk.device_id,
                    context_id,
                    op_flag,
                    op_info.hash_id,
                ]
            )
        else:
            self.task_info.append(
                [
                    op_info.model_id,
                    op_name,
                    rts_trk.stream_id,
                    rts_trk.task_id,
                    op_info.block_num,
                    op_info.mix_block_num,
                    op_info.is_dynamic,
                    task_type,
                    op_type,
                    request_id,
                    rts_trk.thread_id,
                    rts_trk.timestamp,
                    rts_trk.batch_id,
                    op_info.tensor_num,
                    op_info.input_formats,
                    op_info.input_data_types,
                    op_info.input_shapes,
                    op_info.output_formats,
                    op_info.output_data_types,
                    op_info.output_shapes,
                    rts_trk.device_id,
                    context_id,
                    op_flag,
                    op_info.hash_id,
                ]
            )

    def run(self, event: Event, call_stack: dict):
        # pure runtime api
        if not event.is_invalid() and not event.additional_record:
            return

        is_level0 = InfoConfReader().is_level0()
        mem_cpy_dto, task_track_dtos = self.get_task_level_additional_dto(event)
        for task_track_dto in task_track_dtos:
            if task_track_dto.struct_type is not None:
                self.add_host_task(call_stack, task_track_dto)

            hccl_event: Event = call_stack.get(Constant.HCCL_LEVEL)
            if self.is_hccl_task(hccl_event, task_track_dto):
                self.add_hccl_task(call_stack.get(Constant.MODEL_LEVEL), hccl_event, task_track_dto)
                self.add_hccl_op(call_stack, task_track_dto)
            if self.is_kernel_task(task_track_dto, hccl_event.is_invalid()):
                self.add_kernel_task(call_stack, task_track_dto, is_level0)

    def save_task_info(self):
        if not self.task_info:
            return

        model = GeModel(self._project_path, [DBNameConstant.TABLE_GE_TASK])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_TASK)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_GE_TASK, self.task_info)
        model.finalize()

    def save_hccl_task_info(self):
        if not self.hccl_task_info:
            return
        if self.mismatch_hccl > 0:
            logging.warning("There is %d hccl info lost", self.mismatch_hccl)
        model = HCCLHostModel(self._project_path)
        model.init()
        model.drop_table(DBNameConstant.TABLE_HCCL_TASK)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK, self.hccl_task_info)
        model.finalize()

    def save_hccl_op_info(self):
        if not self.hccl_op_info:
            return

        model = HCCLHostModel(self._project_path)
        model.init()
        model.drop_table(DBNameConstant.TABLE_HCCL_OP)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, self.hccl_op_info)
        model.finalize()

    def save_host_tasks(self):
        if not self.host_tasks:
            return
        with RuntimeHostTaskModel(self._project_path) as model:
            model.flush(self.host_tasks)

    def flush_data(self):
        self.save_task_info()
        self.save_hccl_task_info()
        self.save_hccl_op_info()
        self.save_host_tasks()


class HCCLGear(CANNGear):
    """
    hccl op contains several threads, link will represent in hccl_calculator.py
    """

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.HCCL_LEVEL

    def run(self, event: Event, call_stack: dict):
        pass

    def flush_data(self):
        pass
