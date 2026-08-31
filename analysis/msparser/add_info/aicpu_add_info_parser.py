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
from typing import List

from common_func.db_name_constant import DBNameConstant
from common_func.hash_dict_constant import HashDictData
from common_func.hccl_info_common import (
    DataType,
    DeviceHcclSource,
    LinkType,
    OpType,
    RdmaType,
    RoleType,
    TransPortType,
    trans_enum_name,
)
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.platform.chip_manager import ChipManager
from mscalculate.ascend_task.host_task_collector import HostTaskCollector
from mscalculate.flip.flip_calculator import FlipCalculator
from msmodel.add_info.kfc_info_model import KfcInfoModel
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.ai_cpu.data_preparation_model import DataPreparationModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean, KfcHcclInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip
from profiling_bean.prof_enum.data_tag import DataTag


class _AicpuBatchTaskAdapter:
    """将 mainStreamTask 的 aicpu 侧分组 key 暴露给 FlipCalculator.compute_batch_id：
    stream_id 取落盘 aicpu_stream_id 列值（host_tasks_map remap），timestamp 取 host 时间（对齐 DeviceTaskFlip），
    compute_batch_id 写入的 batch_id 直接落到 bean.aicpu_batch_id
    """

    def __init__(self: any, bean: any, aicpu_stream_id: int) -> None:
        self._bean = bean
        self._aicpu_stream_id = aicpu_stream_id

    @property
    def stream_id(self: any) -> int:
        return self._aicpu_stream_id

    @property
    def task_id(self: any) -> int:
        return self._bean.aicpu_task_id

    @property
    def timestamp(self: any) -> int:
        return InfoConfReader().time_from_syscnt(self._bean.timestamp)

    @property
    def batch_id(self: any) -> int:
        return self._bean.aicpu_batch_id

    @batch_id.setter
    def batch_id(self: any, value: int) -> None:
        self._bean.aicpu_batch_id = value


class AicpuAddInfoParser(DataParser, MsMultiProcess):
    """
    aicpu data parser
    """

    NONE_NODE_NAME = ''
    INVALID_CONTEXT_ID = 4294967295

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)  # pylint: disable=E1003
        self._file_list = file_list
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.hash_data = {}
        self.host_tasks_map = {}
        self.is_chip_v6 = ChipManager().is_chip_v6()
        self.unique_id_map = dict()
        self._aicpu_data = {
            AicpuAddInfoBean.AICPU_NODE: [],
            AicpuAddInfoBean.AICPU_DP: [],
            AicpuAddInfoBean.AICPU_MODEL: [],
            AicpuAddInfoBean.AICPU_MI: [],
            AicpuAddInfoBean.KFC_COMM_TURN: [],
            AicpuAddInfoBean.KFC_COMPUTE_TURN: [],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
            AicpuAddInfoBean.HCCL_OP_INFO: [],
            AicpuAddInfoBean.AICPU_FLIP_TASK: [],
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: [],
        }
        self._get_data_save_func = {
            AicpuAddInfoBean.AICPU_NODE: self.save_aicpu_node_data,
            AicpuAddInfoBean.AICPU_DP: self.save_aicpu_dp_data,
            AicpuAddInfoBean.AICPU_MODEL: self.save_aicpu_model_data,
            AicpuAddInfoBean.AICPU_MI: self.save_aicpu_mi_data,
            AicpuAddInfoBean.KFC_COMM_TURN: self.save_kfc_comm_turn_data,
            AicpuAddInfoBean.KFC_COMPUTE_TURN: self.save_kfc_compute_turn_data,
            AicpuAddInfoBean.KFC_HCCL_INFO: self.save_kfc_hccl_info_data,
            AicpuAddInfoBean.HCCL_OP_INFO: self.save_hccl_op_info_data,
            AicpuAddInfoBean.AICPU_FLIP_TASK: self.save_aicpu_flip_task_data,
            AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK: self.save_aicpu_master_stream_hccl_task_data,
        }

    def save_aicpu_node_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                self._get_stream_id_from_host(aicpu_info),
                aicpu_info.data.task_id,
                aicpu_info.data.ai_cpu_task_start_time,
                aicpu_info.data.ai_cpu_task_end_time,
                AicpuAddInfoParser.NONE_NODE_NAME,
                aicpu_info.data.compute_time,
                aicpu_info.data.memory_copy_time,
                aicpu_info.data.ai_cpu_task_time,
                aicpu_info.data.dispatch_time,
                aicpu_info.data.total_time,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU]) as model:
            model.flush(result, DBNameConstant.TABLE_AI_CPU)

    def save_aicpu_dp_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                InfoConfReader().trans_into_local_time(float(aicpu_info.timestamp)),
                aicpu_info.data.action,
                aicpu_info.data.source,
                aicpu_info.data.buffer_size,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU_DP]) as model:
            model.flush(result, DBNameConstant.TABLE_AI_CPU_DP)

    def save_aicpu_model_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                aicpu_info.data.index_id,
                aicpu_info.data.model_id,
                aicpu_info.timestamp,
                aicpu_info.data.tag_id,
                aicpu_info.data.event_id,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with TsTrackModel(
            self.project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_MODEL_WITH_Q]
        ) as model:
            model.create_table(DBNameConstant.TABLE_MODEL_WITH_Q)
            model.flush(DBNameConstant.TABLE_MODEL_WITH_Q, result)

    def save_aicpu_mi_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                aicpu_info.data.node_name,
                aicpu_info.data.queue_size,
                aicpu_info.data.start_time,
                aicpu_info.data.end_time,
                aicpu_info.data.duration,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with DataPreparationModel(self.project_path, [DBNameConstant.TABLE_DATA_QUEUE]) as model:
            model.flush(result)

    def save_kfc_comm_turn_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                aicpu_info.data.device_id,
                self._get_stream_id_from_host(aicpu_info),
                aicpu_info.data.task_id,
                aicpu_info.data.comm_turn,
                aicpu_info.data.current_turn,
                aicpu_info.data.server_start_time,
                aicpu_info.data.wait_msg_start_time,
                aicpu_info.data.kfc_alg_exe_start_time,
                aicpu_info.data.send_task_start_time,
                aicpu_info.data.send_sqe_finish_time,
                aicpu_info.data.rtsq_exe_end_time,
                aicpu_info.data.server_end_time,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_COMM_TURN]) as model:
            model.flush(result, DBNameConstant.TABLE_KFC_COMM_TURN)

    def save_kfc_compute_turn_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                aicpu_info.data.device_id,
                self._get_stream_id_from_host(aicpu_info),
                aicpu_info.data.task_id,
                aicpu_info.data.compute_turn,
                aicpu_info.data.current_turn,
                aicpu_info.data.wait_compute_start_time,
                aicpu_info.data.compute_start_time,
                aicpu_info.data.compute_exe_end_time,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_COMPUTE_TURN]) as model:
            model.flush(result, DBNameConstant.TABLE_KFC_COMPUTE_TURN)

    def save_aicpu_flip_task_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                self._get_stream_id_from_host(aicpu_info),
                InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
                aicpu_info.data.task_id,
                aicpu_info.data.flip_num,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_AICPU_TASK_FLIP]) as model:
            model.flush(result, DBNameConstant.TABLE_AICPU_TASK_FLIP)

    def save_aicpu_master_stream_hccl_task_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
                self.host_tasks_map.get(aicpu_info.data.aicpu_task_id, aicpu_info.data.aicpu_stream_id),
                aicpu_info.data.aicpu_task_id,
                aicpu_info.data.aicpu_batch_id,
                self.unique_id_map.get(aicpu_info.data.task_id, aicpu_info.data.stream_id),
                aicpu_info.data.task_id,
                aicpu_info.data.batch_id,
                aicpu_info.data.type,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_AICPU_MASTER_STREAM_HCCL_TASK]) as model:
            model.flush(result, DBNameConstant.TABLE_AICPU_MASTER_STREAM_HCCL_TASK)

    def save_hccl_op_info_data(self: any, aicpu_info_list: List[AicpuAddInfoBean]):
        result = [
            [
                InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
                aicpu_info.data.relay,
                aicpu_info.data.retry,
                trans_enum_name(DataType, aicpu_info.data.data_type),
                self.hash_data.get(aicpu_info.data.alg_type, aicpu_info.data.alg_type),
                aicpu_info.data.count,
                aicpu_info.data.group_name,
                self._get_stream_id_from_host(aicpu_info),
                aicpu_info.data.task_id,
                aicpu_info.data.rank_size,
                DeviceHcclSource.HCCL.value,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_DEVICE_HCCL_OP_INFO]) as model:
            model.flush(result, DBNameConstant.TABLE_DEVICE_HCCL_OP_INFO)

    def save_kfc_hccl_info_data(self: any, aicpu_info_list: List[KfcHcclInfoBean]):
        result = [
            [
                InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
                self.hash_data.get(aicpu_info.item_id, aicpu_info.item_id),
                aicpu_info.ccl_tag,
                aicpu_info.group_name,
                aicpu_info.local_rank,
                aicpu_info.remote_rank,
                aicpu_info.rank_size,
                aicpu_info.work_flow_mode,
                aicpu_info.plane_id,
                self.INVALID_CONTEXT_ID,
                aicpu_info.notify_id,
                aicpu_info.stage,
                trans_enum_name(RoleType, aicpu_info.role),
                aicpu_info.duration_estimated,
                aicpu_info.src_addr,
                aicpu_info.dst_addr,
                aicpu_info.data_size,
                trans_enum_name(OpType, aicpu_info.op_type),
                trans_enum_name(DataType, aicpu_info.data_type),
                trans_enum_name(LinkType, aicpu_info.link_type),
                trans_enum_name(TransPortType, aicpu_info.transport_type),
                trans_enum_name(RdmaType, aicpu_info.rdma_type),
                aicpu_info.stream_id,
                aicpu_info.task_id,
                aicpu_info.batch_id,
            ]
            for aicpu_info in aicpu_info_list
        ]
        with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_INFO]) as model:
            model.flush(result, DBNameConstant.TABLE_KFC_INFO)

    def parse(self: any) -> None:
        """
        parse ai cpu
        """
        aicpu_files = self._file_list.get(DataTag.AICPU_ADD_INFO, [])
        aicpu_info = self.parse_bean_data(
            aicpu_files, StructFmt.AI_CPU_ADD_FMT_SIZE, AicpuAddInfoBean, check_func=self.check_magic_num
        )
        self.hash_data = HashDictData(self._project_path).get_ge_hash_dict()
        self.set_aicpu_data(aicpu_info)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        for struct_type, aicpu_data in self._aicpu_data.items():
            func = self._get_data_save_func.get(struct_type)
            if not func:
                logging.error("The aicpu type %d is invalid.", struct_type)
                continue
            if not aicpu_data:
                continue
            func(aicpu_data)

    def ms_run(self: any) -> None:
        """
        parse and save ge fusion data
        :return:
        """
        if not self._file_list.get(DataTag.AICPU_ADD_INFO, []):
            return
        logging.info("start parsing aicpu data, files: %s", str(self._file_list.get(DataTag.AICPU_ADD_INFO)))
        self.parse()
        self._pre_process_host_info()
        self._compute_batch_id()
        self._compute_aicpu_batch_id()
        self.save()

    def set_aicpu_data(self: any, aicpu_data: list) -> None:
        for aicpu_info in aicpu_data:
            struct_type = int(aicpu_info.struct_type)
            if struct_type == AicpuAddInfoBean.AICPU_NODE and (
                aicpu_info.data.ai_cpu_task_start_time == 0 or aicpu_info.data.ai_cpu_task_end_time == 0
            ):
                continue
            if struct_type == AicpuAddInfoBean.KFC_HCCL_INFO:
                self._aicpu_data.get(struct_type).extend(self._pre_process_kfc_info(aicpu_info))
            else:
                self._aicpu_data.get(struct_type).append(aicpu_info)

    def _pre_process_host_info(self: any) -> None:
        device_id = InfoConfReader().get_device_id()
        host_collector = HostTaskCollector(self._project_path)
        self.host_tasks_map = host_collector.get_host_task_stream_table(device_id) if self.is_chip_v6 else {}

    def _get_stream_id_from_host(self: any, aicpu_info: AicpuAddInfoBean) -> int:
        """
        获取处理后的 stream_id
        :param aicpu_info: AicpuAddInfoBean 对象
        :return: 处理后的 stream_id
        """
        return self.host_tasks_map.get(aicpu_info.data.task_id, aicpu_info.data.stream_id)

    def _pre_process_kfc_info(self: any, aicpu_info: AicpuAddInfoBean) -> list:
        result_list = []
        for kfc_hccl_info in [aicpu_info.data.first_hccl_info, aicpu_info.data.second_hccl_info]:
            if kfc_hccl_info.group_name == "0":
                continue
            if self.is_chip_v6:
                self.unique_id_map[kfc_hccl_info.task_id] = kfc_hccl_info.stream_id
            result_list.append(kfc_hccl_info)
        return result_list

    def _compute_batch_id(self: any) -> None:
        """用aicpu flip task预计算kfc_info和mainStreamTask的batchId，避免后续再从DB读取计算"""
        flip_task_list = self._aicpu_data.get(AicpuAddInfoBean.AICPU_FLIP_TASK, [])
        if not flip_task_list:
            return

        # 将flip数据转为TaskFlip（FlipCalculator原生接受的类型）
        flip_items = [
            TaskFlip(
                timestamp=flip.timestamp,
                stream_id=flip.data.stream_id,
                task_id=flip.data.task_id,
                flip_num=flip.data.flip_num,
            )
            for flip in flip_task_list
        ]

        # KfcHcclInfoBean已有timestamp/stream_id/task_id/batch_id，可直接传入
        kfc_info_list = self._aicpu_data.get(AicpuAddInfoBean.KFC_HCCL_INFO, [])
        if kfc_info_list:
            FlipCalculator.compute_batch_id(kfc_info_list, flip_items, is_flip_num=False)

        # 对mainStreamTask也同样预计算batchId
        master_stream_list = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK, [])
        if master_stream_list:
            FlipCalculator.compute_batch_id([item.data for item in master_stream_list], flip_items, is_flip_num=False)

    def _compute_aicpu_batch_id(self: any) -> None:
        """用 step_trace.db DeviceTaskFlip(device flip) 预计算 mainStreamTask 的 aicpuBatchId。
        分组 key 对齐落盘 aicpu_stream_id 列值（host_tasks_map remap），timestamp 对齐 DeviceTaskFlip（host 时间），
        与 kfcModel 原先 set_device_batch_id 的逻辑一致，计算后随 aicpu_batch_id 列一并落盘
        """
        master_stream_list = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MASTER_STREAM_HCCL_TASK, [])
        if not master_stream_list:
            return
        with TsTrackModel(
            self.project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]
        ) as model:
            task_flip = model.get_task_flip_data()
        if not task_flip:
            return
        task_data = [
            _AicpuBatchTaskAdapter(
                item.data, self.host_tasks_map.get(item.data.aicpu_task_id, item.data.aicpu_stream_id)
            )
            for item in master_stream_list
        ]
        FlipCalculator.compute_batch_id(task_data, task_flip, is_flip_num=False)
