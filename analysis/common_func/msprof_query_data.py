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

import os

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceModel
from profiling_bean.basic_info.query_data_bean import QueryDataBean
from profiling_bean.db_dto.hwts_rec_dto import HwtsRecDto
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class MsprofQueryData:
    """
    Data splicing for query data, and show in the screen later.
    """

    FILE_NAME = os.path.basename(__file__)
    QUERY_HEADERS = [
        MsProfCommonConstant.JOB_INFO,
        MsProfCommonConstant.DEVICE_ID,
        MsProfCommonConstant.JOB_NAME,
        MsProfCommonConstant.COLLECTION_TIME,
        MsProfCommonConstant.MODEL_ID,
        MsProfCommonConstant.ITERATION_ID,
        MsProfCommonConstant.TOP_TIME_ITERATION,
        MsProfCommonConstant.RANK_ID,
    ]
    QUERY_TOP_ITERATION_NUM = 5

    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.data = []

    @classmethod
    def query_cluster_data(cls: any, collection_path: str, cluster_info_list: list) -> list:
        cluster_query_data = []
        with ClusterStepTraceModel(collection_path, []) as cluster_step_trace:
            for cluster_info in cluster_info_list:
                if cluster_info[3] == Constant.DEFAULT_INVALID_VALUE:
                    rank_or_device_id = cluster_info[1]
                else:
                    rank_or_device_id = cluster_info[3]
                step_table_name = DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_or_device_id)
                model_info_list = cluster_step_trace.get_model_info(step_table_name)
                if not model_info_list:
                    data = [
                        cluster_info[0],
                        cluster_info[1],
                        cluster_info[4],
                        cluster_info[2],
                        'N/A',
                        'N/A',
                        'N/A',
                        cluster_info[3],
                    ]
                    cluster_query_data.append(data)
                    continue
                for model_info in model_info_list:
                    data = [
                        cluster_info[0],
                        cluster_info[1],
                        cluster_info[4],
                        cluster_info[2],
                        model_info[0],
                        model_info[1],
                        model_info[2],
                        cluster_info[3],
                    ]
                    cluster_query_data.append(data)
        return cluster_query_data

    @classmethod
    def get_job_basic_info(cls: any) -> list:
        """
        Get the info under the profiling job data: job info, device id, job name and collect time.
        :return: list of the basic data.
        """
        return InfoConfReader().get_job_basic_info()

    @classmethod
    def _get_iteration_infos(cls: any, curs: any, table_name: str = DBNameConstant.TABLE_STEP_TRACE_DATA) -> list:
        sql = "select model_id, max(index_id) from {} group by model_id".format(table_name)
        iteration_infos = DBManager.fetch_all_data(curs, sql)
        if not iteration_infos or not iteration_infos[0]:
            return []
        return iteration_infos

    @classmethod
    def _get_model_id_set(cls: any, curs_ge: any) -> set:
        sql = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        model_ids = DBManager.fetch_all_data(curs_ge, sql)
        model_ids_set = {model_id[0] for model_id in model_ids}
        return model_ids_set

    @classmethod
    def _update_top_iteration_info(
        cls: any,
        iteration_infos: list,
        model_ids_set: set,
        curs: any,
        table_name: str = DBNameConstant.TABLE_STEP_TRACE_DATA,
    ) -> list:
        if not iteration_infos or not model_ids_set:
            return []

        # get top {QUERY_TOP_ITERATION_NUM} time iterations of every model
        top_sql = (
            "select t.* from ("
            "select index_id, model_id,"
            "(select count(*) + 1 from {0} as t2 where t2.model_id = t1.model_id and "
            "(t2.step_end - t2.step_start) > (t1.step_end - t1.step_start)) as top "
            "from {0} as t1) as t where top <= {1} order by model_id, top".format(
                table_name, cls.QUERY_TOP_ITERATION_NUM
            )
        )
        top_index_ids = DBManager.fetch_all_data(curs, top_sql)
        if not top_index_ids or not top_index_ids[0]:
            return []
        top_index_ids_filtered = list(
            filter(lambda model_iter_info: model_iter_info[1] in model_ids_set, top_index_ids)
        )

        # every iteration_info in format (model_id, max(index_id))
        # top_index_ids_filtered is in format [(index_id, model_id, top),...]
        iteration_info_list = []
        iteration_infos_result = []
        for iteration_info in iteration_infos:
            iteration_info_list = list(iteration_info)
            top_index_id = list(
                # pylint: disable=cell-var-from-loop
                filter(lambda model_iter_info: model_iter_info[1] == iteration_info_list[0], top_index_ids_filtered)
            )
            if not top_index_id:
                iteration_info_list.append(Constant.NA)
            else:
                top_index_ids_info = ",".join((str(id[0]) for id in top_index_id))
                iteration_info_list.append(top_index_ids_info)
            iteration_infos_result.append(iteration_info_list)
        return iteration_infos_result

    def get_job_iteration_info(self: any) -> list:
        """
        Get the iteration info under the profiling job data: model id and index id.
        :return: list of the iteration data.
        """
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_STEP_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
            DBManager.destroy_db_connect(conn, curs)
            return []

        db_path_ge = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        conn_ge, curs_ge = DBManager.check_connect_db_path(db_path_ge)
        if not conn_ge or not curs_ge or not DBManager.judge_table_exist(curs_ge, DBNameConstant.TABLE_GE_TASK):
            model_ids_set = self._get_model_id_set_without_ge(curs)
        else:
            model_ids_set = self._get_model_id_set(curs_ge)

        if not model_ids_set:
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            DBManager.destroy_db_connect(conn, curs)
            return []

        iteration_infos = self._get_iteration_infos(curs)
        # filter model which contains no compute op
        iteration_infos_filtered = list(filter(lambda model_index: model_index[0] in model_ids_set, iteration_infos))
        iteration_infos_result = self._update_top_iteration_info(iteration_infos_filtered, model_ids_set, curs)

        DBManager.destroy_db_connect(conn_ge, curs_ge)
        DBManager.destroy_db_connect(conn, curs)

        return iteration_infos_result

    def get_step_iteration_info(self: any) -> list:
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_STEP_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_STEP_TIME):
            DBManager.destroy_db_connect(conn, curs)
            return []
        iteration_infos = self._get_iteration_infos(curs, DBNameConstant.TABLE_STEP_TIME)
        model_ids_set = {NumberConstant.INVALID_MODEL_ID}
        iteration_infos_result = self._update_top_iteration_info(
            iteration_infos, model_ids_set, curs, DBNameConstant.TABLE_STEP_TIME
        )
        DBManager.destroy_db_connect(conn, curs)
        return iteration_infos_result

    def assembly_job_info(self: any, basic_data: list, iteration_data: list) -> list:
        """
        Splicing data into query data object classes.
        :param basic_data: job info, device id, job name and collect time
        :param iteration_data: model id and index id
        :return: list of query data object
        """
        result = []
        if not iteration_data:
            # Model id and index id should be 'NA' without iteration data
            data = (
                basic_data[:2]
                + [os.path.basename(self.project_path)]
                + [basic_data[2]]
                + [Constant.NA, Constant.NA, Constant.NA]
                + [basic_data[3]]
            )
            query_bean = QueryDataBean(**dict(zip(self.QUERY_HEADERS, data)))
            result.append(query_bean)
            return result

        for _data in iteration_data:
            data = (
                basic_data[:2] + [os.path.basename(self.project_path)] + [basic_data[2]] + list(_data) + [basic_data[3]]
            )
            query_bean = QueryDataBean(**dict(zip(self.QUERY_HEADERS, data)))
            result.append(query_bean)
        return result

    def query_data(self: any) -> list:
        """
        Query data with basic info and iteration info.
        :return: list of query data object
        """
        basic_data = self.get_job_basic_info()
        if not basic_data:
            return []

        iteration_data = self.get_job_iteration_info() + self.get_step_iteration_info()
        return self.assembly_job_info(basic_data, iteration_data)

    def _get_model_id_set_without_ge(self: any, trace_curs: any) -> set:
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_HWTS_REC)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not (conn and curs):
            return set()
        sql = "select iter_id from {} where ai_core_num > 0".format(DBNameConstant.TABLE_HWTS_ITER_SYS)
        iter_data = DBManager.fetch_all_data(curs, sql, dto_class=HwtsRecDto)
        DBManager.destroy_db_connect(conn, curs)
        iter_id_set = set(iter_id.iter_id for iter_id in iter_data)
        filter_sql = "select index_id, model_id, iter_id from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        filter_data = DBManager.fetch_all_data(trace_curs, filter_sql, dto_class=StepTraceDto)
        result = set(data.model_id for data in filter_data if data.iter_id in iter_id_set)
        return result


class QueryArgumentCheck:
    @staticmethod
    def check_arguments_valid(npu_id: int, model_id: int, iteration_id: int) -> None:
        if not QueryArgumentCheck._check_integer_with_min_value(npu_id, min_value=-1):
            raise ProfException(
                ProfException.PROF_INVALID_PARAM_ERROR, "The query id is wrong. Please enter a valid value."
            )
        if not QueryArgumentCheck._check_integer_with_min_value(model_id, min_value=0):
            raise ProfException(
                ProfException.PROF_INVALID_PARAM_ERROR, "The query model id is wrong. Please enter a valid value."
            )
        if not QueryArgumentCheck._check_integer_with_min_value(iteration_id, min_value=1, nullable=True):
            raise ProfException(
                ProfException.PROF_INVALID_PARAM_ERROR, "The query iteration id is wrong. Please enter a valid value."
            )

    @staticmethod
    def _check_integer_with_min_value(arg: any, min_value: int = None, nullable: bool = False) -> bool:
        if nullable and arg is None:
            return True
        if arg is not None and isinstance(arg, int):
            if min_value is not None:
                return arg >= min_value
        return False
