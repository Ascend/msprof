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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class ClusterParallelModel(ParserModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_PARALLEL, [])
        self.conn = None
        self.cur = None

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)

    def create_table(self: any, table_name: str) -> None:
        """
        create table
        """
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)

    def init(self: any) -> bool:
        """
        create db and tables
        """
        self.conn, self.cur = DBManager.create_connect_db(PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        return True


class ClusterParallelViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_CLUSTER_PARALLEL, [])

    def get_npu_ids(self: any, table_name: str) -> list:
        result = []
        sql = (
            "SELECT CASE WHEN t.rank_id is null THEN t.device_id ELSE t.rank_id END FROM("
            "SELECT rank_id, device_id FROM {} GROUP BY rank_id, device_id)t".format(table_name)
        )
        data_list = DBManager.fetch_all_data(self.cur, sql)
        for data in data_list:
            result.append(data[0])
        return result

    def get_model_iteration_ids(self: any, table_name: str) -> dict:
        result = {}
        sql = "SELECT model_id, GROUP_CONCAT(distinct iteration_id) FROM {} GROUP BY model_id".format(table_name)
        data_list = DBManager.fetch_all_data(self.cur, sql)
        for data in data_list:
            result[data[0]] = [int(iteration_id) for iteration_id in data[1].split(',')]
        return result

    def get_table_name(self: any) -> str:
        sql = "SELECT name FROM sqlite_master WHERE type='table' AND name like '%Parallel%'"
        data_list = DBManager.fetch_all_data(self.cur, sql)
        if not data_list:
            return Constant.NA
        elif not data_list[0]:
            return Constant.NA
        return data_list[0][0]

    def get_parallel_type(self: any) -> str:
        sql = "select paralleltype from {} limit 1".format(DBNameConstant.TABLE_CLUSTER_PARALLEL_STRATEGY)
        data_list = DBManager.fetch_all_data(self.cur, sql)
        if not data_list:
            return Constant.NA
        elif not data_list[0]:
            return Constant.NA
        return data_list[0][0]

    def get_data_parallel_data(self: any, first_field_name: str, condition: str, query_params: tuple) -> dict:
        sql = (
            "select {0}, computation_time, pure_communication_time, communication_time, "
            "interval_of_communication_time from {1} where {2}".format(
                first_field_name, DBNameConstant.TABLE_CLUSTER_DATA_PARALLEL, condition
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, query_params)

    def get_model_parallel_data(self: any, first_field_name: str, condition: str, query_params: tuple) -> dict:
        sql = "select {0}, computation_time, pure_communication_time from {1} where {2}".format(
            first_field_name, DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL, condition
        )
        return DBManager.fetch_all_data(self.cur, sql, query_params)

    def get_pipeline_parallel_data(self: any, first_field_name: str, condition: str, query_params: tuple) -> dict:
        sql = (
            "select {0}, computation_time, pure_communication_time_only_revice, "
            "pure_communication_time_except_revice, step_time-pure_communication_time stage_time "
            "from {1} where {2}".format(first_field_name, DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL, condition)
        )
        return DBManager.fetch_all_data(self.cur, sql, query_params)

    def get_first_field_name(self: any, params: dict) -> tuple:
        if params["npu_id"] == Constant.DEFAULT_INVALID_VALUE:
            return (self._get_npu_id_name(), "Rank ID")
        else:
            return ("iteration_id", "Iteration ID")

    def get_parallel_condition_and_query_params(self: any, params: dict) -> list:
        if params.get("npu_id") == Constant.DEFAULT_INVALID_VALUE:
            return ["model_id=? and iteration_id=?", (params.get("model_id"), params.get("iteration_id"))]
        else:
            return [
                "{}=? and model_id=?".format(self._get_npu_id_name()),
                (params.get("npu_id"), params.get("model_id")),
            ]

    def get_data_parallel_tuning_data(self: any) -> list:
        sql = (
            "select hccl_op_num, avg(pure_communication_ratio)pure_communication_ratio, "
            "avg(interval_ratio) interval_ratio from (select rank_id, device_id, hccl_op_num, "
            "sum(pure_communication_time)/sum(communication_time) as pure_communication_ratio, "
            "sum(interval_of_communication_time)/sum(interval_of_communication_time+communication_time) "
            "interval_ratio from {} group by rank_id, device_id)t".format(DBNameConstant.TABLE_CLUSTER_DATA_PARALLEL)
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_model_parallel_tuning_data(self: any) -> list:
        sql = (
            "select avg(ratio) avg_ratio from (select rank_id, device_id, "
            "sum(pure_communication_time)/(sum(pure_communication_time)+sum(computation_time)) ratio "
            "from {} group by rank_id, device_id)t".format(DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL)
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_pipeline_parallel_tuning_data(self: any) -> list:
        avg_stage_time = self._get_avg_stage_time()
        if avg_stage_time == Constant.DEFAULT_INVALID_VALUE:
            return []
        sql = (
            "SELECT avg( t.ratio ) avg_ratio, avg( t.ratio1 ) avg_ratio1, "
            "sum(case when t.stage_time >= {0} * 0.8 AND t.stage_time <= {0} * 1.2 THEN 0 ELSE 1 END ) num "
            "FROM( SELECT rank_id, device_id, "
            "sum(pure_communication_time_only_revice) / sum(pure_communication_time+computation_time) ratio, "
            "sum(pure_communication_time_except_revice) / sum(pure_communication_time+computation_time) ratio1, "
            "sum(step_time - pure_communication_time) stage_time "
            "FROM {1} GROUP BY rank_id, device_id) t".format(
                avg_stage_time, DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL
            )
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def _get_npu_id_name(self: any) -> str:
        sql = "select rank_id from {} where rank_id is not null".format(self.get_table_name())
        if DBManager.fetch_all_data(self.cur, sql):
            return "rank_id"
        else:
            return "device_id"

    def _get_avg_stage_time(self: any) -> float:
        sql = (
            "	SELECT avg( t.stage_time ) avg_stage_time FROM( SELECT rank_id, device_id, "
            "sum(step_time - pure_communication_time) stage_time FROM {} GROUP BY rank_id, device_id)t".format(
                DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL
            )
        )
        data_list = DBManager.fetch_all_data(self.cur, sql)
        if not data_list:
            return Constant.DEFAULT_INVALID_VALUE
        elif not data_list[0]:
            return Constant.DEFAULT_INVALID_VALUE
        return data_list[0][0]
