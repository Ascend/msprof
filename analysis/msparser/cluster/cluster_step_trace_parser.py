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

import logging
import os
from itertools import chain

from common_func.common import error
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from common_func.profiling_scene import ProfilingScene
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceModel
from msparser.interface.iparser import IParser
from profiling_bean.db_dto.step_trace_dto import TrainingTraceDto


class ClusterStepTraceParser(IParser):
    """
    Step trace data parser for cluster scene.
    """

    FILE_NAME = os.path.basename(__file__)
    MODEL_ID_INDEX = 1
    MODEL_ID_IN_GE = 1
    MODEL_ID_NOT_IN_GE = 0

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = collection_path
        self.id_with_project_path_map = {}
        self.id_with_table_map = {}
        self.id_with_all_reduce_map = {}
        self.cluster_model = None

    @staticmethod
    def _fetch_data_from_database(db_info: tuple, sql: str, dto_class=None) -> list:
        db_dir, db_name, db_table = db_info[:]
        data = []
        db_path = PathManager.get_db_path(db_dir, db_name)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The connect to %s is failed.", db_name)
            return data
        if not DBManager.judge_table_exist(curs, db_table):
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The %s table doesn't exist.", db_table)
            return data
        data = DBManager.fetch_all_data(curs, sql, dto_class=dto_class)
        DBManager.destroy_db_connect(conn, curs)
        return data

    @staticmethod
    def _sql_for_step_trace() -> str:
        model_id_condition = "model_id != {}".format(NumberConstant.INVALID_MODEL_ID)
        if ProfilingScene().is_step_export():
            model_id_condition = "model_id = {}".format(NumberConstant.INVALID_MODEL_ID)
        sql = (
            "select device_id, "
            "model_id, "
            "iteration_id, "
            "FP_start as fp_start, "
            "BP_end as bp_end, "
            "iteration_end, "
            "(case when iteration_time={0} then {0} else iteration_time*{1} end) as iteration_time, "
            "(case when fp_bp_time={0} then {0} else fp_bp_time*{1} end) as fp_bp_time, "
            "(case when grad_refresh_bound={0} then {0} else grad_refresh_bound*{1} end) as grad_refresh_bound, "
            "(case when data_aug_bound={0} then {0} else data_aug_bound*{1} end) as data_aug_bound "
            "from {2} where {3}".format(
                NumberConstant.NULL_NUMBER,
                StepTraceConstant.syscnt_to_micro(),
                DBNameConstant.TABLE_TRAINING_TRACE,
                model_id_condition,
            )
        )
        return sql

    @staticmethod
    def _append_ge_model_tag(step_trace_data: list, model_ids: list) -> list:
        result = []
        for item in step_trace_data:
            item = list(item)
            if item[ClusterStepTraceParser.MODEL_ID_INDEX] in model_ids:
                item.append(ClusterStepTraceParser.MODEL_ID_IN_GE)
            else:
                item.append(ClusterStepTraceParser.MODEL_ID_NOT_IN_GE)
            result.append(item)
        return result

    @staticmethod
    def _get_data_list_from_dto(step_trace_data: list) -> list:
        for idx, single_dto in enumerate(step_trace_data):
            step_trace_data[idx] = (
                single_dto.device_id,
                single_dto.model_id,
                single_dto.iteration_id,
                ClusterStepTraceParser._get_syscnt_time(single_dto.fp_start),
                ClusterStepTraceParser._get_syscnt_time(single_dto.bp_end),
                ClusterStepTraceParser._get_syscnt_time(single_dto.iteration_end),
                single_dto.iteration_time,
                single_dto.fp_bp_time,
                single_dto.grad_refresh_bound,
                single_dto.data_aug_bound,
            )
        return step_trace_data

    @staticmethod
    def _get_syscnt_time(value):
        if value == NumberConstant.NULL_NUMBER:
            return value
        return InfoConfReader().time_from_syscnt(value, NumberConstant.MICRO_SECOND)

    def ms_run(self: any) -> None:
        logging.info("Start to parse cluster step_trace data!")
        if not self._check_collection_path_valid():
            logging.error("The input dir doesn't have cluster database, please check.")
            error(ClusterStepTraceParser.FILE_NAME, "The input dir doesn't have cluster database, please check.")
            return
        self.parse()
        logging.info("Start to save cluster step trace data to db!")
        self.save()
        logging.info("Query cluster step trace data finished!")

    def parse(self: any) -> None:
        if not self._collect_project_paths():
            error(ClusterStepTraceParser.FILE_NAME, "The cluster step trace parsing is failed.")

    def save(self: any) -> None:
        tables = self._generate_cluster_table_name()
        if not tables:
            logging.error("The step trace source database or table is not found.")
            return
        with ClusterStepTraceModel(self.collection_path, tables) as cluster_model:
            cluster_model.create_table()
            self._collect_and_save_step_trace_data(cluster_model)

    def _check_collection_path_valid(self: any) -> bool:
        db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        return os.path.exists(db_path)

    def _generate_cluster_table_name(self: any) -> list:
        for rank_id in list(self.id_with_project_path_map.keys()):
            path = self.id_with_project_path_map.get(rank_id)
            device_path = os.path.join(self.collection_path, path)
            trace_db_path = PathManager.get_db_path(device_path, DBNameConstant.DB_TRACE)
            if not DBManager.check_tables_in_db(trace_db_path, DBNameConstant.TABLE_TRAINING_TRACE):
                self.id_with_project_path_map.pop(rank_id)
                continue
            self.id_with_table_map.setdefault(rank_id, DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id))
            self.id_with_all_reduce_map.setdefault(rank_id, DBNameConstant.TABLE_CLUSTER_ALL_REDUCE.format(rank_id))
        return list(self.id_with_table_map.values()) + list(self.id_with_all_reduce_map.values())

    def _collect_project_paths(self: any) -> bool:
        rank_db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        conn, curs = DBManager.check_connect_db_path(rank_db_path)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The connect to cluster rank database is failed.")
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_CLUSTER_RANK):
            logging.error("The cluster rank table doesn't exist.")
            DBManager.destroy_db_connect(conn, curs)
            return False
        sql = "select case when rank_id='N/A' then device_id else rank_id end as rank_id, dir_name from {}".format(
            DBNameConstant.TABLE_CLUSTER_RANK
        )
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        if not data:
            logging.error("The query cluster rank info is invalid.")
            return False
        self.id_with_project_path_map = dict(data)
        return True

    def _collect_and_save_step_trace_data(self: any, cluster_model: any) -> None:
        for rank_id, path in self.id_with_project_path_map.items():
            project_path = os.path.join(self.collection_path, path)
            if not DataCheckManager.contain_info_json_data(project_path):
                continue
            InfoConfReader().load_info(project_path)
            if InfoConfReader().is_host_profiling():
                continue

            logging.debug(
                "Start to process the table of step trace,table_name: %s", self.id_with_table_map.get(rank_id)
            )
            step_trace_data = self._collect_step_trace_data(project_path)
            if not step_trace_data:
                continue
            cluster_model.insert_data_to_db(self.id_with_table_map.get(rank_id), step_trace_data)

            logging.debug(
                "Start to process the table of all reduce, table_name: %s", self.id_with_all_reduce_map.get(rank_id)
            )
            all_reduce_data = self._collect_all_reduce_data(project_path)
            if not all_reduce_data:
                continue
            cluster_model.insert_data_to_db(self.id_with_all_reduce_map.get(rank_id), all_reduce_data)

    def _collect_all_reduce_data(self: any, project_path: str) -> list:
        sql = (
            "select device_id,model_id,index_id,iteration_end,start*{syscnt_to_micro},end*{syscnt_to_micro} "
            "from {0}".format(DBNameConstant.TABLE_ALL_REDUCE, syscnt_to_micro=StepTraceConstant.syscnt_to_micro())
        )
        return self._fetch_data_from_database(
            (project_path, DBNameConstant.DB_TRACE, DBNameConstant.TABLE_ALL_REDUCE), sql
        )

    def _collect_step_trace_data(self: any, project_path: str) -> list:
        sql_for_ge_model_ids = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        model_ids = self._fetch_data_from_database(
            (project_path, DBNameConstant.DB_GE_INFO, DBNameConstant.TABLE_GE_TASK), sql_for_ge_model_ids
        )
        model_ids = list(chain.from_iterable(model_ids))
        sql_for_step_trace = self._sql_for_step_trace()
        step_trace_data = self._fetch_data_from_database(
            (project_path, DBNameConstant.DB_TRACE, DBNameConstant.TABLE_TRAINING_TRACE),
            sql_for_step_trace,
            dto_class=TrainingTraceDto,
        )
        step_trace_data = self._get_data_list_from_dto(step_trace_data)
        if not step_trace_data:
            logging.error("Can't query step trace data.")
            return step_trace_data
        step_trace_data = self._append_ge_model_tag(step_trace_data, model_ids)
        return step_trace_data
