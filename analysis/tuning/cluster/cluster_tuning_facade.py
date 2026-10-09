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
import logging
import os
from enum import IntEnum

from common_func.common import print_msg
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_query_data import QueryArgumentCheck
from common_func.msvp_common import create_json_for_dict
from common_func.path_manager import PathManager
from tuning.cluster.cluster_calculator_factory import MatrixCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowLinkCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowRankCalculatorFactory
from tuning.cluster.cluster_parser_factory import ClusterCommunicationParserFactory
from tuning.cluster.cluster_parser_factory import CommunicationMatrixParserFactory
from tuning.cluster.cluster_parser_factory import CriticalPathAnalysisParserFactory


class QueryDataType(IntEnum):
    RUN_ALL_TUNING = -1
    CLUSTER_COMMUNICATION = 6
    COMMUNICATION_MATRIX = 7
    CLUSTER_COMMUNICATION_CRITICAL_PATH = 9
    COMMUNICATION_MATRIX_CRITICAL_PATH = 10


class ClusterTuningFacade:
    """
    interface for tuning presentation and tuning suggestions
    """

    FILE_NAME = os.path.basename(__file__)
    CLUSTER_ALL_DEVICE_SCENE = -1

    def __init__(self: any, params: dict) -> None:
        self.args = params
        self._collection_path = os.path.realpath(params.get("collection_path", ''))
        self._npu_id = params.get("npu_id", -2)
        self._model_id = params.get("model_id", 0)
        self._iteration_id = params.get("iteration_id", -1)
        self.data_type = params.get("data_type", -1)

    def process(self: any) -> None:
        self._check_params_valid()
        self.run()

    def run(self: any) -> None:
        # export command entry
        if self.data_type == QueryDataType.RUN_ALL_TUNING:
            self.cluster_communication()
            self.communication_matrix()
        # query command entry
        elif self.data_type in (QueryDataType.CLUSTER_COMMUNICATION, QueryDataType.CLUSTER_COMMUNICATION_CRITICAL_PATH):
            enable_critical_path = self.data_type == QueryDataType.CLUSTER_COMMUNICATION_CRITICAL_PATH
            self.cluster_communication(print_flag=False, enable_critical_path=enable_critical_path)
        else:
            enable_critical_path = self.data_type == QueryDataType.COMMUNICATION_MATRIX_CRITICAL_PATH
            self.communication_matrix(print_flag=False, enable_critical_path=enable_critical_path)

    def cluster_communication(self: any, print_flag=True, enable_critical_path=False) -> None:
        """
        cluster communication parse and calculate
        """
        logging.info('start to parse cluster communication information!')
        # Enabling Critical Path Analysis
        top_hccl_ops = self.critical_path_analysis() if enable_critical_path else None
        parser_factory = ClusterCommunicationParserFactory(self.args)
        communication_parser = parser_factory.generate_parser(top_hccl_ops)
        logging.info('start to parse hccl events')
        op_info = communication_parser.run()
        logging.info('start to give suggestions according to rules')
        slow_rank_calculator = SlowRankCalculatorFactory(op_info).generate_calculator()
        slow_rank_calculator.run()
        slow_rank_calculator.add_suggestions(op_info)
        slow_link_calculator = SlowLinkCalculatorFactory(op_info).generate_calculator()
        slow_link_calculator.run()
        slow_link_calculator.add_suggestions(op_info)
        out_file_name = "communication_cpa_{}_{}_{}.json" if enable_critical_path else "communication_{}_{}_{}.json"
        output_file_name = out_file_name.format(self._npu_id, parser_factory.max_iters_model_id, self._iteration_id)
        if print_flag:
            print_msg(
                StrConstant.SUGGESTION
                + ': '
                + op_info.get(StrConstant.TOTAL, {}).get(StrConstant.SLOW_RANK_SUGGESTION, '')
            )
        self.dump_dict_to_json(output_file_name, op_info)

    def communication_matrix(self: any, print_flag=True, enable_critical_path=False) -> None:
        """
        communication matrix parse and calculate
        """
        logging.info('start to parse communication matrix information!')
        top_hccl_ops = self.critical_path_analysis() if enable_critical_path else None
        parser_factory = CommunicationMatrixParserFactory(self.args)
        matrix_parser = parser_factory.generate_parser(top_hccl_ops)
        logging.info('start to parse hccl events')
        op_info = matrix_parser.run()
        logging.info('start to give suggestions according to rules')
        matrix_calculator = MatrixCalculatorFactory(op_info).generate_calculator()
        matrix_calculator.run()
        matrix_calculator.add_suggestions(op_info)
        out_file_name = "matrix_cpa_{}_{}_{}.json" if enable_critical_path else "matrix_{}_{}_{}.json"
        output_file_name = out_file_name.format(self._npu_id, parser_factory.max_iters_model_id, self._iteration_id)
        if print_flag:
            matrix_calculator.print_suggestion(op_info)
        self.dump_dict_to_json(output_file_name, op_info)

    def critical_path_analysis(self):
        logging.info("Enabling Critical Path Analysis in CommunicationMatrixParserFactory!")
        critical_path_parser_factory = CriticalPathAnalysisParserFactory(self.args)
        critical_path_parser = critical_path_parser_factory.generate_parser()
        top_hccl_ops = critical_path_parser.run()
        return top_hccl_ops

    def dump_dict_to_json(self: any, output_file_name: str, dict_result: dict):
        output_file_path = PathManager.get_query_result_path(self._collection_path, output_file_name)
        result = create_json_for_dict(output_file_path, dict_result)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': f'communication data generation failed, '
                        f'maybe you can check the directory({self._collection_path}) permissions.',
                        'data': '',
                    }
                )
            )

    def _check_params_valid(self: any) -> None:
        enable_critical_path = self.data_type in (
            QueryDataType.COMMUNICATION_MATRIX_CRITICAL_PATH,
            QueryDataType.CLUSTER_COMMUNICATION_CRITICAL_PATH,
        )
        if not self._is_cluster_all_device_scene() and not enable_critical_path:
            self._npu_id = -1
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.WARN,
                        'info': "and the '--id' parameter has been set to (-1) automatically."
                        "The collective communication data only supports exporting data by all devices, ",
                        'data': '',
                    }
                )
            )
        self._check_data_type_valid()
        QueryArgumentCheck.check_arguments_valid(self._npu_id, self._model_id, self._iteration_id)
        if not self._check_collection_dir_valid():
            raise ProfException(
                ProfException.PROF_CLUSTER_DIR_ERROR,
                "To query cluster or summary data, please execute import --cluster first",
            )

    def _check_collection_dir_valid(self: any) -> bool:
        return os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_RANK))

    def _check_data_type_valid(self: any) -> None:
        if self.data_type not in QueryDataType.__members__.values():
            raise ProfException(
                ProfException.PROF_INVALID_PARAM_ERROR, "The query data type is wrong. Please enter a valid value."
            )

    def _is_cluster_all_device_scene(self: any) -> bool:
        return self._npu_id == self.CLUSTER_ALL_DEVICE_SCENE
