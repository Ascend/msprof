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
import os
from typing import Optional, Dict, Tuple

from common_func.constant import Constant
from common_func.singleton import singleton
from common_func.path_manager import PathManager
from common_func.db_name_constant import DBNameConstant
from msmodel.add_info.runtime_op_info_model import RuntimeOpInfoViewModel
from msmodel.compact_info.capture_stream_info_model import CaptureStreamInfoViewModel
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto


class CaptureStatus:
    START = 0
    END = 1


@singleton
class RTAddInfoCenter:
    def __init__(self: any, project_path: str) -> None:
        self._op_info_dict = {}
        self._capture_info_list = []
        if os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_RTS_TRACK)):
            self.load_runtime_op_info_data(project_path)
        if os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_STREAM_INFO)):
            self.load_capture_stream_info_data(project_path)
        self._capture_info_time_range_dict = self.build_capture_info_time_range_dict()

    def load_runtime_op_info_data(self: any, project_path: str) -> None:
        """
        load runtime op info data
        """
        try:
            with RuntimeOpInfoViewModel(project_path) as _model:
                self._op_info_dict = _model.get_runtime_op_info_data()
        except Exception as e:
            logging.error("Failed to load runtime op info data: %s", str(e))
            self._op_info_dict = []

    def load_capture_stream_info_data(self: any, project_path: str) -> None:
        """
        load capture stream info data
        """
        try:
            with CaptureStreamInfoViewModel(project_path) as _model:
                self._capture_info_list = _model.get_capture_stream_info_data()
        except Exception as e:
            logging.error("Failed to load capture stream info data: %s", str(e))
            self._capture_info_list = []

    def build_capture_info_time_range_dict(self) -> Dict[Tuple[int, int, int], Tuple[int, int, int]]:
        capture_info_time_range_dict = {}
        for capture_info in self._capture_info_list:
            key = (capture_info.device_id, capture_info.stream_id, capture_info.batch_id)

            current_range = capture_info_time_range_dict.get(key, (0, float("inf"), capture_info.model_id))
            if capture_info.capture_status == CaptureStatus.START:
                capture_info_time_range_dict[key] = (capture_info.timestamp, current_range[1], capture_info.model_id)
            elif capture_info.capture_status == CaptureStatus.END:
                capture_info_time_range_dict[key] = (current_range[0], capture_info.timestamp, capture_info.model_id)
        return capture_info_time_range_dict

    def find_matching_model_id(self, device_id: int, stream_id: int, batch_id: int, timestamp: float) -> Optional[int]:
        search_key = (device_id, stream_id, batch_id)
        if search_key not in self._capture_info_time_range_dict:
            return Constant.GE_OP_MODEL_ID
        start_time, end_time, model_id = self._capture_info_time_range_dict[search_key]
        if start_time <= timestamp <= end_time:
            return model_id
        return Constant.GE_OP_MODEL_ID

    def get_op_info_by_id(
        self: any, device_id: int, stream_id: int, task_id: int, batch_id: int, timestamp: float
    ) -> Tuple[int, RuntimeOpInfoDto]:
        """
        get type hash dict data
        后续需要增加batchId做唯一id关联，batchId应通过capture stream info数据获取
        """
        return self.find_matching_model_id(device_id, stream_id, batch_id, timestamp), self._op_info_dict.get(
            (device_id, stream_id, task_id), RuntimeOpInfoDto()
        )
