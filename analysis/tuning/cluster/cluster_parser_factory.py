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
from abc import abstractmethod

from common_func.common import print_msg
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from framework.load_info_manager import LoadInfoManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.cluster_info.communication_model import CommunicationModel
from msmodel.stars.op_summary_model import OpSummaryModel
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceViewModel
from msparser.cluster.communication_matrix_parser import CommunicationMatrixParser
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.critical_path_parser import CriticalPathParser
from msparser.cluster.meta_parser import MetaParser
from msparser.cluster.meta_parser import OpTaskBundle, merge_op_task


class ClusterParserFactory:
    """
    parser factory interface for cluster scene
    """

    def __init__(self: any) -> None:
        self.max_iters_model_id = 0
        self.iteration_id = 0
        self.rank_dir_dict = {}
        self.collection_path = None

    @abstractmethod
    def generate_parser(self):
        """
        generate_parse_method
        """
        return MetaParser()

    def get_hccl_ops_by_iter(self, top_hccl_ops: tuple = None) -> None:
        """
        get op events of all rank by iteration start and end time
        """
        with ClusterInfoViewModel(self.collection_path) as _model:
            if not _model.check_db():
                logging.error("Fail to connect %s, hccl parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
                raise ProfException(ProfException.PROF_INVALID_CONNECT_ERROR)
            rank_dirnames = _model.get_all_rank_id_and_dirnames()
        if not rank_dirnames:
            logging.error("no info useful in %s, hccl parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        # Load the info.json to obtain chip_Id
        LoadInfoManager.load_info(os.path.join(self.collection_path, rank_dirnames[0][1]))
        for rank_dir in rank_dirnames:
            if len(rank_dir) < 2:
                logging.error("no info enough in %s, hccl parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
                raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
            self.rank_dir_dict[rank_dir[0]] = rank_dir[1]
        self.get_conditions_from_db(top_hccl_ops)

    def get_conditions_from_db(self, top_hccl_ops: tuple = None) -> None:
        """
        get max iteration model id, iteration start and end time
        """
        for rank_id, dirname in self.rank_dir_dict.items():
            self._check_rank_info(rank_id, dirname)
            rank_path = os.path.join(self.collection_path, dirname)
            iter_start_end = self.get_step_info_from_db(rank_id)
            self.get_hccl_events_from_db(rank_id, rank_path, iter_start_end, top_hccl_ops)

    def get_hccl_events_from_db(
        self: any, rank_id: int, rank_path: str, iter_start_end: list, top_hccl_ops: tuple = None
    ) -> None:
        """
        get op events of all rank by iteration start and end time
        """
        with CommunicationModel(rank_path) as _model:
            conditions = {
                'iter_start': iter_start_end[0][0] * NumberConstant.NS_TO_US,
                'iter_end': iter_start_end[0][1] * NumberConstant.NS_TO_US,
            }
            op_task_map = _model.get_all_events_from_db(conditions, top_hccl_ops)
            if not op_task_map:
                logging.warning("Fail to get no.%d's hccl events, please check hccl.db", rank_id)
                print_msg(
                    f"Fail to get no.{rank_id}'s hccl events, "
                    f"please check hccl.db from {PathManager.get_host_result_dir(rank_path)}"
                )
            op_name_dict = {}
            for op_task in op_task_map.values():
                op_name = op_task.op_name
                merge_op_task(op_name_dict, op_name, op_task)
            self.update_data(op_name_dict, rank_id)

    def get_step_info_from_db(self, rank_id: int) -> list:
        step_trace_table = DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id)
        iter_start_end = [[NumberConstant.DEFAULT_START_TIME, NumberConstant.DEFAULT_END_TIME]]
        with ClusterStepTraceViewModel(self.collection_path) as model:
            if model.judge_table_exist(step_trace_table):
                # find model id that has most iterations
                model_iteration = model.get_model_id_with_iterations(step_trace_table)
                if not model_iteration:
                    logging.error("%s doesn't have model information", step_trace_table)
                    raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)
                model_iteration = sorted(model_iteration, key=lambda x: x[1], reverse=True)
                self.max_iters_model_id = model_iteration[0][0]
                # get iteration end and start
                if self.iteration_id is None or self.iteration_id <= 0:
                    message = "Invalid iteration id!"
                    logging.error(message)
                    raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
                iter_start_end = model.get_iter_start_end(self.iteration_id, self.max_iters_model_id, step_trace_table)
                if not iter_start_end:
                    message = f"Fail to get no.{self.iteration_id} iteration end and start time."
                    logging.error("%s doesn't have %s iteration information", step_trace_table, str(self.iteration_id))
                    raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR, message)
            else:
                logging.debug("%s doesn't exist!", step_trace_table)
        return iter_start_end

    def update_data(self: any, op_name_dict: dict, rank_id: int):
        """implemented by subclass"""
        pass

    def _check_rank_info(self, rank_id: any, dirname: any) -> None:
        if rank_id not in self.rank_dir_dict:
            message = "The query id is wrong. Please enter a valid value."
            logging.error("--id %s is invalid, valid id: %s", str(rank_id), str(list(self.rank_dir_dict.keys())))
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)

        if rank_id is None or dirname is None:
            logging.error("no valid information in %s, hccl parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)

        if rank_id == Constant.DEFAULT_INVALID_VALUE:
            logging.error('Not Device id or rank id!')
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        if rank_id >= NumberConstant.MAX_RANK_NUMS:
            logging.error(
                "Number of ranks is %s !, exceeds the limited upper bound:%s ",
                str(rank_id),
                NumberConstant.MAX_RANK_NUMS,
            )
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)


class ClusterCommunicationParserFactory(ClusterParserFactory):
    """
    factory which creates cluster communication parser
    provide data preparation for created parser
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, params: dict) -> None:
        super().__init__()
        self.iteration_id = params.get("iteration_id", -1)
        self.max_iters_model_id = NumberConstant.INVALID_MODEL_ID
        self.collection_path = os.path.realpath(params.get("collection_path"))
        self.rank_hccl_data_dict = {}

    def generate_parser(self: any, top_hccl_ops: tuple = None) -> CommunicationParser:
        self.get_hccl_ops_by_iter(top_hccl_ops)
        if not self.rank_hccl_data_dict:
            message = f"fail to get no.{self.iteration_id} iteration hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)

        return CommunicationParser(self.rank_hccl_data_dict)

    def update_data(self: any, op_name_dict: dict, rank_id: int) -> None:
        """
        update self data
        """
        for hccl_name in op_name_dict:
            if hccl_name not in self.rank_hccl_data_dict:
                self.rank_hccl_data_dict[hccl_name] = {}
            self.rank_hccl_data_dict[hccl_name][rank_id] = op_name_dict.get(hccl_name, [])


class CommunicationMatrixParserFactory(ClusterParserFactory):
    """
    factory which creates communication matrix parser
    provide data preparation for created parser
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, params: dict) -> None:
        super().__init__()
        self.iteration_id = params.get("iteration_id", -1)
        self.max_iters_model_id = NumberConstant.INVALID_MODEL_ID
        self.collection_path = os.path.realpath(params.get("collection_path"))
        self.op_hccl_events = {}

    def generate_parser(self: any, top_hccl_ops: tuple = False) -> CommunicationMatrixParser:
        self.get_hccl_ops_by_iter(top_hccl_ops)
        if not self.op_hccl_events:
            message = f"Fail to get no.{self.iteration_id} iteration hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)
        return CommunicationMatrixParser(self.op_hccl_events)

    def update_data(self: any, op_name_dict: dict, rank_id: int) -> None:
        """
        update self data
        """
        for hccl_name, bundle in op_name_dict.items():
            aggregate = self.op_hccl_events.get(hccl_name)
            if aggregate is None:
                aggregate = OpTaskBundle(op_name=bundle.op_name, start=bundle.start, end=bundle.end)
                self.op_hccl_events[hccl_name] = aggregate
            else:
                aggregate.start = min(aggregate.start, bundle.start)
                aggregate.end = max(aggregate.end, bundle.end)
            aggregate.tasks.extend(bundle.tasks)


class CriticalPathAnalysisParserFactory(ClusterParserFactory):
    """
    factory which creates  critical path analysis parser
    provide data preparation for created parser
    """

    def __init__(self: any, params: dict) -> None:
        super().__init__()
        self.iteration_id = params.get("iteration_id", -1)
        self.model_id = params.get('model_id', NumberConstant.INVALID_MODEL_ID)
        self.collection_path = os.path.realpath(params.get("collection_path"))
        self.rank_id = params.get("npu_id", -1)
        self.hccl_op_events = {}
        self.compute_op_events = []

    def generate_parser(self: any) -> CriticalPathParser:
        self.get_hccl_ops_by_iter()
        if not self.hccl_op_events:
            message = f"fail to get no.{self.iteration_id} iteration hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)
        if not self.compute_op_events:
            message = f"fail to get compute ops data of rank {self.rank_id}"
            logging.error("Can't get compute ops events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)
        return CriticalPathParser(self.compute_op_events, self.hccl_op_events)

    def get_conditions_from_db(self, top_hccl_ops: tuple = None) -> None:
        """
        get max iteration model id, iteration start and end time
        """
        if self.rank_id == -1:
            rank_id = list(self.rank_dir_dict.keys())[0]
        else:
            rank_id = self.rank_id
        dir_name = self.rank_dir_dict.get(rank_id)
        self._check_rank_info(rank_id, dir_name)
        rank_path = os.path.join(self.collection_path, dir_name)
        logging.info("Analyzing the critical path of the rank %s, PROF path: %s!", str(rank_id), rank_path)
        iter_start_end = self.get_step_info_from_db(rank_id)
        # Get hccl op info
        self.get_hccl_events_from_db(rank_id, rank_path, iter_start_end)
        # Get compute op info
        sample_config = {'result_dir': rank_path, 'iter_id': self.iteration_id, 'model_id': self.model_id}
        with OpSummaryModel(sample_config) as op_model:
            self.compute_op_events = op_model.get_operator_data_by_task_type()

    def update_data(self: any, op_name_dict: dict, rank_id: int) -> None:
        for hccl_name, bundle in op_name_dict.items():
            # Get mainstream data
            main_events = [event for event in bundle.tasks if event.plane_id == NumberConstant.MAIN_STREAM_THREAD_ID]
            if not main_events:
                logging.error(
                    "Fail to get no.%s rank main events info, critical path parser is interrupted", str(rank_id)
                )
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            self.hccl_op_events[hccl_name] = OpTaskBundle(
                op_name=bundle.op_name, start=bundle.start, end=bundle.end, tasks=main_events
            )
