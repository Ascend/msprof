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

import os
import logging
from collections import OrderedDict
from dataclasses import dataclass
from typing import List
from typing import Set

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.path_manager import PathManager
from common_func.hccl_info_common import DeviceHcclSource
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from msmodel.hccl.hccl_model import HcclViewModel
from common_func.hash_dict_constant import HashDictData


class HCCLExport:
    """
    hccl export
    """

    HCCL_SORTED_OFFSET = 70000
    INVALID_PLANE = -1
    DEFAULT_PLANE = 0
    INVALID_GROUP = 'N/A'
    INVALID_DATA_SIZE = "Invalid"

    @dataclass
    class HcclGroup:
        group_name: str
        planes: Set[int]
        id: int
        start_index: int
        is_aicpu: bool

    def __init__(self: any, param: dict) -> None:
        self.project_path = param.get(StrConstant.PARAM_RESULT_DIR)
        self.result = []
        self.err_message = {}
        self.iter_range = param.get(StrConstant.PARAM_ITER_ID)
        self.pid_value = InfoConfReader().get_json_pid_data()
        self.hccl_groups = dict()
        self._hash_data = {}

    @staticmethod
    def get_hccl_arg(hccl_task):
        size_str = HCCLExport.INVALID_DATA_SIZE if hccl_task.size >= Constant.UINT32_MAX else str(hccl_task.size)
        return OrderedDict(
            {
                'notify_id': hccl_task.notify_id,
                'duration estimated(us)': hccl_task.duration_estimated,
                'stream id': hccl_task.stream_id,
                'task id': hccl_task.task_id,
                'context id': hccl_task.context_id,
                'task type': hccl_task.hccl_name,
                'src rank': hccl_task.local_rank,
                'dst rank': hccl_task.remote_rank,
                'transport type': hccl_task.transport_type,
                'size(Byte)': size_str,
                'data type': hccl_task.data_type,
                'link type': hccl_task.link_type,
                "bandwidth(GB/s)": hccl_task.bandwidth,
            }
        )

    def get_hccl_timeline_data(self: any) -> list:
        """
        get data for hccl timeline
        """
        hccl_data = []
        mc2_hccl_task = []
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            with HcclViewModel(
                self.project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE]
            ) as hccl_model:
                hccl_data = hccl_model.get_hccl_task()
                mc2_hccl_task = hccl_model.get_kfc_task()
        if not hccl_data and not mc2_hccl_task:
            logging.error(
                "get hccl data failed, may be the hccl file not completed or hccl parser "
                "failed. please check data file."
            )
            return []
        self._hash_data = HashDictData(self.project_path).get_ge_hash_dict()
        self._get_meta_data(hccl_data, mc2_hccl_task)
        if hccl_data:
            self._format_hccl_data(hccl_data, DeviceHcclSource.HCCL.value)
        if mc2_hccl_task:
            self._format_hccl_data(mc2_hccl_task, DeviceHcclSource.MC2.value)
        return self.result

    def _add_hccl_bar(self):
        self.result = TraceViewManager.metadata_event(
            [["process_name", self.pid_value, InfoConfReader().get_json_tid_data(), "Communication"]]
        )

    def _add_group_threads(self, group: HcclGroup, start_sort_index: int, valid_group: bool) -> int:
        """
        add group thread meta data in json
        return: end_sort_index
        """
        if not group.planes:
            return start_sort_index
        index_now = start_sort_index
        # update start index
        group.start_index = start_sort_index
        group_name = self._hash_data.get(group.group_name, group.group_name)
        if group.is_aicpu:
            thread_name = f"Group {group_name} Aicpu Communication" if valid_group else "Aicpu Communication"
        else:
            thread_name = f"Group {group_name} Communication" if valid_group else "Communication"
        self.result.extend(TraceViewManager.metadata_event([["thread_name", self.pid_value, index_now, thread_name]]))
        self.result.extend(
            TraceViewManager.metadata_event([["thread_sort_index", self.pid_value, index_now, index_now]])
        )

        plane_infos = []
        plane_sort_indexes = []
        for plane in group.planes:
            index_now = start_sort_index + plane + 1
            plane_infos.append(["thread_name", self.pid_value, index_now, f"Plane {plane}"])
            plane_sort_indexes.append(["thread_sort_index", self.pid_value, index_now, index_now])
        self.result.extend(TraceViewManager.metadata_event(plane_infos))
        self.result.extend(TraceViewManager.metadata_event(plane_sort_indexes))
        index_now += 1
        return index_now

    def _init_hccl_group(self, hccl_data: List[HcclTask], kfc_data: List[HcclTask]) -> dict:
        name_planes_table: OrderedDict[str, dict] = OrderedDict()
        hccl_groups = dict()
        for data in hccl_data:
            # L0 scene or something get error
            if data.plane_id == self.INVALID_PLANE:
                data = data.replace(plane_id=self.DEFAULT_PLANE)
            name_planes_table.setdefault(data.group_name, {}).setdefault("hccl", set()).add(data.plane_id)
        for data in kfc_data:
            if data.plane_id == self.INVALID_PLANE:
                data = data.replace(plane_id=self.DEFAULT_PLANE)
            name_planes_table.setdefault(data.group_name, {}).setdefault("aicpu", set()).add(data.plane_id)
        for _id, (group_name, planes) in enumerate(name_planes_table.items()):
            planes_hccl = planes.get("hccl", set())
            planes_aicpu = planes.get("aicpu", set())
            hccl_group = self.HcclGroup(group_name, planes_hccl, _id, -1, False)
            hccl_group_aicpu = self.HcclGroup(group_name, planes_aicpu, _id, -1, True)
            hccl_groups[group_name] = [hccl_group, hccl_group_aicpu]
        return hccl_groups

    def _get_meta_data(self: any, hccl_data: List[HcclTask], kfc_data: List[HcclTask]) -> None:
        self.hccl_groups = self._init_hccl_group(hccl_data, kfc_data)

        self._add_hccl_bar()

        index_now = 0
        for groups in self.hccl_groups.values():
            for group in groups:
                index_now = self._add_group_threads(group, index_now, group.group_name != self.INVALID_GROUP)

    def _format_hccl_data(self: any, hccl_data: list, group_type: int = 0) -> None:
        _hccl_format_data = self._format_hccl_communication_data(hccl_data, group_type)
        _hccl_format_op_data = self._format_hccl_op_data(group_type)
        self.result.extend(
            TraceViewManager.time_graph_trace(
                TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, _hccl_format_data + _hccl_format_op_data
            )
        )

    def _format_hccl_op_data(self, group_type: int = 0):
        if not os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            return []
        with HcclViewModel(
            self.project_path,
            DBNameConstant.DB_HCCL_SINGLE_DEVICE,
            [DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE],
        ) as hccl_model:
            if group_type == DeviceHcclSource.HCCL.value:
                hccl_op_data_from_group = hccl_model.get_hccl_op_data_by_group()
                hccl_op_info_from_table = hccl_model.get_hccl_op_info_from_table()
            else:
                hccl_op_data_from_group = hccl_model.get_kfc_op_data_by_group()
                hccl_op_info_from_table = hccl_model.get_hccl_op_info_from_table(DBNameConstant.TABLE_KFC_OP)
            hccl_format_op_data = [None] * len(hccl_op_data_from_group)
            for idx, hccl_op in enumerate(hccl_op_data_from_group):
                if hccl_op.group_name not in self.hccl_groups:
                    logging.error("The group name %s not exists", hccl_op.group_name)
                    continue
                hccl_op_info = hccl_op_info_from_table.get(hccl_op.connection_id, HcclOps())
                args = {
                    "rank_size": hccl_op_info.rank_size,
                    "connection_id": hccl_op.connection_id,
                    "model id": hccl_op.model_id,
                    "data_type": hccl_op_info.data_type,
                    "alg_type": hccl_op_info.alg_type,
                    "count": hccl_op_info.count,
                }

                hccl_format_op_data[idx] = [
                    hccl_op.op_name,
                    self.pid_value,
                    self.hccl_groups.get(hccl_op.group_name)[group_type].start_index,
                    InfoConfReader().trans_into_local_time(raw_timestamp=hccl_op.timestamp),
                    hccl_op.duration / NumberConstant.NS_TO_US,
                    args,
                ]
        return hccl_format_op_data

    def _format_hccl_communication_data(self, hccl_data: List[HcclTask], group_type: int = 0):
        # check level0 by sample.json
        if not hccl_data or InfoConfReader().is_level0():
            return []
        _hccl_format_data = [0] * len(hccl_data)
        index = 0
        for _, _hccl_data in enumerate(hccl_data):
            hccl_args = HCCLExport.get_hccl_arg(_hccl_data)
            hccl_args["model id"] = _hccl_data.model_id
            if _hccl_data.group_name not in self.hccl_groups:
                logging.error("The group name %s not exists: group idx: %d", _hccl_data.group_name, group_type)
                continue
            if _hccl_data.plane_id == self.INVALID_PLANE:
                continue
            thread_id = self.hccl_groups.get(_hccl_data.group_name)[group_type].start_index + _hccl_data.plane_id + 1
            _hccl_data_piece = [
                _hccl_data.hccl_name,
                self.pid_value,
                thread_id,
                InfoConfReader().trans_into_local_time(raw_timestamp=_hccl_data.timestamp),
                _hccl_data.duration / NumberConstant.NS_TO_US,
                hccl_args,
            ]
            _hccl_format_data[index] = _hccl_data_piece
            index += 1
        return _hccl_format_data[:index]
