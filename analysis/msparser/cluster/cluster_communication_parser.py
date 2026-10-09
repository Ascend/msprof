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

import json
import os

from common_func.common import print_msg
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_query_data import QueryArgumentCheck
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_communication_model import ClusterCommunicationModel
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel


class ClusterCommunicationParser:
    """
    collective communication data parser
    """

    HEADERS = ['Rank ID', 'Computation Time', 'Communication Time', 'Stage Time']
    CLUSTER_ALL_DEVICE_SCENE = -1

    def __init__(self: any, params: dict) -> None:
        self._collection_path = params.get("collection_path")
        self._npu_id = params.get("npu_id")
        self._model_id = params.get("model_id")
        self._iteration_id = params.get("iteration_id")
        self._data_collection = []
        self._communication_model = ClusterCommunicationModel(params)
        self._cluster_info_model = ClusterInfoViewModel(self._collection_path)

    def process(self: any) -> None:
        QueryArgumentCheck.check_arguments_valid(self._npu_id, self._model_id, self._iteration_id)
        if not self._is_cluster_all_device_scene():
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': "The collective communication data only supports exporting data by all devices, "
                        "and the '--id' parameter needs to be set to (-1).",
                        'data': '',
                    }
                )
            )
            return
        self._get_communication_data()
        self._storage_summary_data()

    def _get_communication_data(self: any) -> None:
        """
        communication contains: rank id, compute_time, communication_time, stage_time
        """
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)):
            return
        with self._communication_model as _model:
            if not _model.check_db():
                return
            with self._cluster_info_model as _c_model:
                if not _c_model.check_db() or not _c_model.check_table():
                    return
                rank_or_device_ids = _c_model.get_rank_or_device_ids()
            if not rank_or_device_ids:
                return
            for rank_or_device_id in rank_or_device_ids:
                communication_data = _model.get_cluster_communication(rank_or_device_id)
                if not communication_data:
                    continue
                self._data_collection.extend(communication_data)

    def _storage_summary_data(self: any) -> None:
        if not self._data_collection:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': f'The collective communication data is not found, '
                        f'maybe you can check whether the data exists in the iteration({self._iteration_id}).',
                        'data': '',
                    }
                )
            )
            return

        communication_data = []
        for data in self._data_collection:
            communication_data.append([data.rank_id, data.compute_time, data.communication_time, data.stage_time])
        output_file_name = "collective_communication_{}_{}_{}.json".format(
            self._npu_id, self._model_id, self._iteration_id
        )
        output_file_path = PathManager.get_query_result_path(self._collection_path, output_file_name)
        result = create_json(output_file_path, self.HEADERS, communication_data, save_old_file=False)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': f'collective communication data generation failed, '
                        f'maybe you can check the directory({self._collection_path}) permissions.',
                        'data': '',
                    }
                )
            )

    def _is_cluster_all_device_scene(self: any) -> bool:
        return self._npu_id == self.CLUSTER_ALL_DEVICE_SCENE
