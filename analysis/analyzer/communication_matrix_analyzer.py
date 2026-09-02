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

# pylint: disable=duplicate-code
# 与communication_analyzer互为分叉代码，analyzer 处理骨架结构相同

import json
import logging
import os

from common_func.common import print_msg
from common_func.common import warn
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import prepare_for_analyze
from common_func.msprof_common import prepare_log
from common_func.msprof_common import get_valid_sub_path
from common_func.msprof_exception import ProfException
from common_func.msvp_common import create_json_for_dict
from common_func.path_manager import PathManager
from framework.load_info_manager import LoadInfoManager
from msmodel.cluster_info.communication_model import CommunicationModel
from msmodel.cluster_info.communication_analyzer_model import CommunicationAnalyzerModel
from msparser.cluster.communication_matrix_parser import CommunicationMatrixParser
from msparser.cluster.meta_parser import merge_op_task


class CommunicationMatrixAnalyzer:
    """
    single NPU communication data analyzer
    """

    FILE_NAME = os.path.basename(__file__)
    HOST_PATH = 'host'

    def __init__(self: any, collection_path: any, export_type: any) -> None:
        self.collection_path = collection_path
        self.hccl_op_data = {}
        self.export_type = export_type

    def process(self):
        """Analyzing Communication Data"""
        self._process_sub_dirs()

    def _process_output(self, output_data: list) -> dict:
        """Delete unnecessary fields in dict"""
        proc_dict = {}
        for data in output_data:
            proc_dict[data.get(StrConstant.OP_NAME)] = {}
            link_info = data.get(StrConstant.LINK_INFO)
            for info in link_info:
                src_rank = info.get(CommunicationMatrixInfo.SRC_RANK)
                dst_rank = info.get(CommunicationMatrixInfo.DST_RANK)
                link_key = f"{src_rank}-{dst_rank}"
                proc_dict[data.get(StrConstant.OP_NAME)][link_key] = {
                    CommunicationMatrixInfo.TRANSPORT_TYPE: info.get(CommunicationMatrixInfo.TRANSPORT_TYPE),
                    CommunicationMatrixInfo.TRANSIT_SIZE_MB: info.get(CommunicationMatrixInfo.TRANSIT_SIZE_MB),
                    CommunicationMatrixInfo.TRANSIT_TIME_MS: info.get(CommunicationMatrixInfo.TRANSIT_TIME_MS),
                    CommunicationMatrixInfo.BANDWIDTH_GB_S: info.get(CommunicationMatrixInfo.BANDWIDTH_GB_S),
                }
        return proc_dict

    def _get_hccl_data_from_db(self: any, rank_path: str) -> None:
        """
        get op events of all rank by iteration start and end time
        """
        with CommunicationModel(rank_path) as _model:
            start_ts, _ = InfoConfReader().get_collect_time()
            start_time_raw_timestamp = InfoConfReader().trans_from_local_time_into_dev_raw_time(start_ts)
            conditions = {'iter_start': start_time_raw_timestamp, 'iter_end': NumberConstant.DEFAULT_END_TIME}
            op_task_map = _model.get_all_events_from_db(conditions)
            if not op_task_map:
                print_msg(f"Fail to get hccl events, please check hccl_single_device.db from {rank_path}")
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            for op_task in op_task_map.values():
                # get group_name from hccl_single_device.db
                op_name = op_task.op_name + "@" + op_task.group_name
                merge_op_task(self.hccl_op_data, op_name, op_task)

    def _generate_output(self, rank_path: str) -> None:
        self._get_hccl_data_from_db(rank_path)
        if not self.hccl_op_data:
            message = "fail to get hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)

        communication_matrix_parser = CommunicationMatrixParser(self.hccl_op_data)
        process_data = self._process_output(communication_matrix_parser.run())

        if self.export_type == 'text':
            self._dump_dict_to_json(process_data)
        self._dump_dict_to_db(process_data)

    def _dump_dict_to_db(self, output_result: dict) -> object:
        output_file_path = PathManager.get_analyze_dir(self.collection_path)
        with CommunicationAnalyzerModel(output_file_path, [DBNameConstant.TABLE_COMM_ANALYZER_MATRIX]) as _model:
            _model.flush_communication_data_to_db(output_result)

    def _dump_dict_to_json(self, output_result: dict):
        output_file_name = "communication_matrix.json"
        output_file_path = PathManager.get_analyze_result_path(self.collection_path, output_file_name)
        result = create_json_for_dict(output_file_path, output_result)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': f'communication matrix data generation failed, '
                        f'maybe you can check the directory({self.collection_path}) permissions.',
                        'data': '',
                    }
                )
            )

    def _process_sub_dirs(self: any, sub_path: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if sub_path:
            collect_path = os.path.join(self.collection_path, sub_path)
        sub_dirs = sorted(get_path_dir(collect_path), reverse=True)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir in (StrConstant.TIMELINE_PATH, self.HOST_PATH):
                continue

            sub_path = get_valid_sub_path(collect_path, sub_dir, False)
            if DataCheckManager.contain_info_json_data(sub_path):
                LoadInfoManager.load_info(sub_path)
                self._communication_matrix_analyze(sub_path)
            elif sub_path and is_cluster:
                warn(
                    self.FILE_NAME,
                    'Invalid parsing dir("%s"), -dir must be profiling data dir '
                    'such as PROF_XXX_XXX_XXX' % collect_path,
                )
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)

    def _communication_matrix_analyze(self, sub_path: str) -> None:
        """communication matrix analyzer"""
        if not os.path.exists(PathManager.get_db_path(sub_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            prepare_log(PathManager.get_analyze_dir(os.path.join(sub_path, '..')))
            logging.warning(
                'There is no hccl data to analyze communication matrix. '
                'Please export first or check whether single device.'
            )
            return
        prepare_for_analyze(os.path.join(sub_path, '..'))
        self._generate_output(sub_path)
