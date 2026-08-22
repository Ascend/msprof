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
# 与communication_matrix_analyzer互为分叉代码，analyzer 处理骨架结构相同

import json
import logging
import os

from common_func.common import print_msg
from common_func.common import warn
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import OpBandWidthType
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
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.meta_parser import merge_op_task


class CommunicationAnalyzer:
    """
    single NPU communication data analyzer
    """

    FILE_NAME = os.path.basename(__file__)
    HOST_PATH = 'host'

    def __init__(self: any, collection_path: any, export_type: any) -> None:
        self.collection_path = collection_path
        self.rank_hccl_data_dict = {}
        self.export_type = export_type

    @staticmethod
    def _process_dict(dict_data: dict) -> dict:
        """Delete unnecessary fields in dict"""
        proc_dict = {}
        for op_name, op_data in dict_data.items():
            proc_dict[op_name] = op_data.get(Constant.DEFAULT_INVALID_VALUE)
            for transport_type in [
                StrConstant.RDMA,
                StrConstant.SDMA,
                StrConstant.PCIE,
                StrConstant.HCCS,
                StrConstant.SIO,
            ]:
                proc_dict[op_name].get(StrConstant.COMMUNICATION_BANDWIDTH_INFO).get(transport_type).pop(
                    OpBandWidthType.BANDWIDTH_UTILIZATION
                )
        return proc_dict

    def process(self):
        """Analyzing Communication Data"""
        self._process_sub_dirs()

    def _get_hccl_data_from_db(self: any, rank_path: str):
        """
        get op events of all rank by iteration start and end time
        """
        with CommunicationModel(rank_path) as _model:
            start_ts, _ = InfoConfReader().get_collect_time()
            start_time_raw_timestamp = InfoConfReader().trans_from_local_time_into_dev_raw_time(start_ts)
            conditions = {'iter_start': start_time_raw_timestamp, 'iter_end': NumberConstant.DEFAULT_END_TIME}
            op_task_map = _model.get_all_events_from_db(conditions)
            if not op_task_map:
                print_msg(
                    f"Fail to get hccl events, please check hccl.db from {PathManager.get_host_result_dir(rank_path)}"
                )
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            op_name_dict = {}
            for op_task in op_task_map.values():
                # get group_name from hccl.db
                op_name = op_task.op_name + "@" + op_task.group_name
                merge_op_task(op_name_dict, op_name, op_task)
            self._update_data(op_name_dict, Constant.DEFAULT_INVALID_VALUE)

    def _update_data(self: any, op_name_dict: dict, rank_id: int) -> None:
        """
        update self data
        """
        for hccl_name in op_name_dict:
            if hccl_name not in self.rank_hccl_data_dict:
                self.rank_hccl_data_dict[hccl_name] = {}
            self.rank_hccl_data_dict[hccl_name][rank_id] = op_name_dict.get(hccl_name, [])

    def _generate_parser(self: any, rank_path) -> CommunicationParser:
        self._get_hccl_data_from_db(rank_path)

        if not self.rank_hccl_data_dict:
            message = "fail to get hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)

        return CommunicationParser(self.rank_hccl_data_dict)

    def _generate_output(self, rank_path):
        communication_parser = self._generate_parser(rank_path)
        process_data = self._process_dict(communication_parser.run())

        if self.export_type == 'text':
            self._dump_dict_to_json(process_data)
        self._dump_dict_to_db(process_data)

    def _dump_dict_to_db(self, output_result: dict):
        output_file_path = PathManager.get_analyze_dir(self.collection_path)
        with CommunicationAnalyzerModel(
            output_file_path, [DBNameConstant.TABLE_COMM_ANALYZER_TIME, DBNameConstant.TABLE_COMM_ANALYZER_BAND]
        ) as _model:
            _model.flush_communication_data_to_db(output_result)

    def _dump_dict_to_json(self, output_result: dict):
        output_file_name = "communication.json"
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
                        'info': f'communication data generation failed, '
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
            if sub_dir not in (StrConstant.TIMELINE_PATH, self.HOST_PATH):
                sub_path = get_valid_sub_path(collect_path, sub_dir, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    LoadInfoManager.load_info(sub_path)
                    self._communication_analyze(sub_path)
                elif sub_path and is_cluster:
                    warn(
                        self.FILE_NAME,
                        'Invalid parsing dir("%s"), -dir must be profiling data dir '
                        'such as PROF_XXX_XXX_XXX' % collect_path,
                    )
                else:
                    self._process_sub_dirs(sub_dir, is_cluster=True)

    def _communication_analyze(self, sub_path: str) -> None:
        # communication analyzer
        if not os.path.exists(PathManager.get_db_path(sub_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            prepare_log(PathManager.get_analyze_dir(os.path.join(sub_path, '..')))
            logging.warning(
                'There is no hccl data to analyze communication. Please export first or check whether single device.'
            )
            return
        LoadInfoManager.load_info(sub_path)
        prepare_for_analyze(os.path.join(sub_path, '..'))
        self._generate_output(sub_path)
