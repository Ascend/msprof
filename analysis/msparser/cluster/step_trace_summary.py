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
import logging
import os

from common_func.common import print_msg
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_query_data import QueryArgumentCheck
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceViewModel


class StepTraceSummary:
    """
    The class for querying step trace summary data.
    """

    FILE_NAME = os.path.basename(__file__)
    HEADERS = [
        "ID",
        "Model ID",
        "Iteration ID",
        "Iteration Time",
        "FP to BP Time",
        "Iteration Interval",
        "Iteration Refresh",
        "Iteration Start",
        "FP Start",
        "BP End",
        "Iteration End",
    ]
    ID_NUM_FOR_ALL_DEVICES = -1
    ID_NUM_FOR_ALL_ITERATIONS = -1
    NUMBER_0F_DECIMAL_PLACE = 2

    def __init__(self: any, params: dict) -> None:
        self.collection_path = params.get("collection_path")
        self.npu_id = params.get("npu_id")
        self.model_id = params.get("model_id")
        self.iteration_id = params.get("iteration_id")
        self.all_devices = False
        self.cluster_info_model = ClusterInfoViewModel(self.collection_path)
        self.cluster_step_trace_model = ClusterStepTraceViewModel(self.collection_path)

    def process(self: any) -> None:
        QueryArgumentCheck.check_arguments_valid(self.npu_id, self.model_id, self.iteration_id)
        self._check_query_all_devices()
        self._check_iteration_id_valid()
        data_collection = self._query_summary_data()
        if data_collection:
            self._storage_summary_data(data_collection)
        else:
            logging.error("Query step trace data failed.")
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': 'Query step trace data failed.', 'data': ''}))

    def _storage_summary_data(self: any, data: list) -> None:
        output_file_name = "step_trace_{}_{}_{}.json".format(self.npu_id, self.model_id, self.iteration_id)
        output_file_path = PathManager.get_query_result_path(self.collection_path, output_file_name)
        result = create_json(output_file_path, StepTraceSummary.HEADERS, data, save_old_file=False)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            logging.error("Save step trace data failed.")
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': 'Save step trace data failed', 'data': ''}))

    def _query_summary_data(self: any) -> list:
        data = []
        if not self._check_step_trace_db():
            logging.error("Step trace database file does not exist. Please check the input dir.")
            return data
        rank_or_device_ids = self._get_rank_or_device_ids()
        if not rank_or_device_ids:
            logging.error("Get rank id or device id info failed.")
            return data
        return self._query_data_in_db(rank_or_device_ids)

    def _check_step_trace_db(self: any) -> bool:
        db_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        return os.path.exists(db_file)

    def _check_query_all_devices(self: any) -> None:
        self.all_devices = self.npu_id == StepTraceSummary.ID_NUM_FOR_ALL_DEVICES

    def _check_iteration_id_valid(self: any) -> None:
        if self.iteration_id is None:
            self.iteration_id = StepTraceSummary.ID_NUM_FOR_ALL_ITERATIONS
        if self.all_devices and self.iteration_id == StepTraceSummary.ID_NUM_FOR_ALL_ITERATIONS:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': 'For querying all devices data, you should input a valid iteration id.',
                        'data': '',
                    }
                )
            )
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not self.all_devices and self.iteration_id != StepTraceSummary.ID_NUM_FOR_ALL_ITERATIONS:
            print_msg(
                json.dumps(
                    {
                        'status': NumberConstant.ERROR,
                        'info': 'For querying single device data, you should not input a iteration id.',
                        'data': '',
                    }
                )
            )
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)

    def _query_data_in_db(self: any, rank_or_device_ids: set) -> list:
        data_collection = []
        with self.cluster_step_trace_model as model:
            rank_or_device_ids_to_query = rank_or_device_ids if self.all_devices else set([self.npu_id])
            for rank_or_device_id in rank_or_device_ids_to_query:
                table = DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_or_device_id)
                if not model.judge_table_exist(table):
                    logging.error("The %s table doesn't exist.", table)
                    continue
                sql = self._sql_for_query_all_iteration(table, rank_or_device_id)
                if self.all_devices:
                    sql = sql + f" and iteration_id={self.iteration_id}"
                data = model.get_sql_data(sql)
                if not data:
                    logging.error("The query data in %s table doesn't exist.", table)
                    continue
                data_collection.extend(data)
        return data_collection

    def _get_rank_or_device_ids(self: any) -> set:
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            return set()
        with self.cluster_info_model as model:
            if not model.check_table():
                return set()
            return model.get_rank_or_device_ids()

    def _sql_for_query_all_iteration(self: any, table_name: str, rank_or_device_id: int) -> str:
        sql = (
            "select {0}, (case when model_id={1} then 'N/A' else model_id end), "
            "iteration_id, "
            "(case when iteration_time={2} then 'N/A' else round(iteration_time, {3}) end), "
            "(case when fp_bp_time={2} then 'N/A' else round(fp_bp_time, {3}) end), "
            "(case when data_aug_bound={2} then 'N/A' else round(data_aug_bound, {3}) end), "
            "(case when bp_end={2} then 'N/A' else round(iteration_end - bp_end, {3}) end), "
            "(case when iteration_time={2} or iteration_end={2} then 'N/A' else "
            "round(iteration_end - iteration_time, {3}) end), "
            "(case when fp_start={2} then 'N/A' else round(fp_start, {3}) end), "
            "(case when bp_end={2} then 'N/A' else round(bp_end, {3}) end), "
            "(case when iteration_end={2} then 'N/A' else round(iteration_end, {3}) end) "
            "from {4} where model_id={5}".format(
                rank_or_device_id,
                NumberConstant.DEFAULT_MODEL_ID,
                NumberConstant.NULL_NUMBER,
                StepTraceSummary.NUMBER_0F_DECIMAL_PLACE,
                table_name,
                self.model_id,
            )
        )
        return sql
