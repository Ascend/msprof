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

import json
import os.path
from collections import OrderedDict

from common_func.common import print_msg
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceViewModel
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto


class MsProfClusterInfo:
    """
    The class for querying cluster info data.
    """

    OUTPUT_FILE_NAME = "cluster_info.json"
    OUTPUT_CLUSTER_INFO_HEADERS = ["Rank Id", "Device Id", "Prof Dir", "Device Dir", "Models"]
    OUTPUT_MODELS_HEADERS = ["Model Id", "Iterations"]
    SINGLE_OP_MODE = ['N/A', 'N/A']

    def __init__(self: any, project_path: str) -> None:
        self.project_path = os.path.realpath(project_path)
        self.cluster_info_model = ClusterInfoViewModel(self.project_path)
        self.cluster_step_trace_model = ClusterStepTraceViewModel(self.project_path)
        self.info_collection = []

    def run(self: any) -> None:
        """
        run cluster info
        :return: None
        """
        self._collect_cluster_info_data(self.project_path)
        if not self.info_collection:
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': "Get the cluster info failed", 'data': ""}))
            return
        output_file_path = PathManager.get_query_result_path(self.project_path, MsProfClusterInfo.OUTPUT_FILE_NAME)
        result = create_json(
            output_file_path, MsProfClusterInfo.OUTPUT_CLUSTER_INFO_HEADERS, self.info_collection, save_old_file=False
        )
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': "Save the cluster info failed", 'data': ""}))

    def _collect_cluster_info_data(self: any, project_path: str) -> None:
        cluster_infos = []
        with self.cluster_info_model as model:
            if model.check_table():
                cluster_infos = model.get_all_cluster_rank_info()
        if not cluster_infos:
            return
        with self.cluster_step_trace_model as model:
            for cluster_info in cluster_infos:
                self._collect_info_for_each_rank(cluster_info, model)

    def _collect_info_for_each_rank(self: any, cluster_info: ClusterRankDto, model: ClusterStepTraceViewModel):
        if cluster_info.rank_id == Constant.DEFAULT_INVALID_VALUE:
            rank_id = cluster_info.device_id
        else:
            rank_id = cluster_info.rank_id
        step_trace_table = DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id)
        prof_dir, device_dir = cluster_info.dir_name.split(os.sep)
        model_list = []
        if model.judge_table_exist(step_trace_table):
            sql_for_total_iterations = "select model_id, max(iteration_id) from {} group by model_id".format(
                step_trace_table
            )
            iteration_data = model.get_sql_data(sql_for_total_iterations)
            if not iteration_data:
                return
            for each in iteration_data:
                iteration_info = ['N/A', each[1]] if each[0] == NumberConstant.INVALID_MODEL_ID else each
                model_list.append(OrderedDict(list(zip(MsProfClusterInfo.OUTPUT_MODELS_HEADERS, iteration_info))))
            self.info_collection.append([rank_id, cluster_info.device_id, prof_dir, device_dir, model_list])
        else:
            model_list.append(
                OrderedDict(list(zip(MsProfClusterInfo.OUTPUT_MODELS_HEADERS, MsProfClusterInfo.SINGLE_OP_MODE)))
            )
            self.info_collection.append([rank_id, cluster_info.device_id, prof_dir, device_dir, model_list])
