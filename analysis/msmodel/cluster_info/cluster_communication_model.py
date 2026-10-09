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

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.collective_communication_dto import CollectiveCommunicationDto


class ClusterCommunicationModel(ViewModel):
    """
    operation of collective communication.
    """

    def __init__(self, params):
        self._collection_path = params["collection_path"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        super().__init__(self._collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE, [])

    def get_cluster_communication(self, rank_id):
        sql = (
            "select {rank_id} as rank_id, t0.fp_bp_time - t0.fp_bp_communication_time as compute_time,"
            "t0.communication_time, "
            "t0.iteration_time - t0.communication_time as stage_time "
            "from (select "
            "(case when tt.fp_bp_time = 0 then tt.iteration_time else tt.fp_bp_time end) as fp_bp_time, "
            "tt.iteration_time, sum(t1.all_reduce_end - t1.all_reduce_start) as communication_time,"
            "sum(case when fp_bp_time > 0 and t1.all_reduce_start > tt.bp_end "
            "then 0 else t1.all_reduce_end - t1.all_reduce_start end) as fp_bp_communication_time "
            "from {1} t1 inner join {0} tt "
            "on t1.model_id = tt.model_id and t1.index_id = tt.iteration_id "
            "and t1.model_id = {2} and t1.index_id = {3} "
            "group by t1.model_id, t1.index_id) t0".format(
                DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id),
                DBNameConstant.TABLE_CLUSTER_ALL_REDUCE.format(rank_id),
                self._model_id,
                self._iteration_id,
                rank_id=rank_id,
            )
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CollectiveCommunicationDto)

    def get_communication_time_ratio(self: any, device_or_rank_id: int):
        sql = (
            "SELECT avg( t.all_reduce_time ) / ( avg( t.all_reduce_time ) + avg( t.fp_bp_time ) ) as ratio "
            "FROM(SELECT a.model_id, a.iteration_id, a.fp_bp_time, sum( b.all_reduce_end - b.all_reduce_start ) "
            "all_reduce_time FROM (SELECT model_id, iteration_id, bp_end, fp_bp_time "
            "FROM {0} WHERE fp_bp_time IS NOT NULL AND fp_bp_time <> 0 ) a "
            "INNER JOIN {1} b ON a.model_id = b.model_id AND a.iteration_id = b.index_id "
            "AND a.bp_end <= b.all_reduce_start AND b.all_reduce_end IS NOT NULL AND b.all_reduce_end <> 0 "
            "GROUP BY a.model_id, a.iteration_id, a.fp_bp_time )t ".format(
                DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(device_or_rank_id),
                DBNameConstant.TABLE_CLUSTER_ALL_REDUCE.format(device_or_rank_id),
            )
        )
        data = DBManager.fetch_all_data(self.cur, sql)
        if not data:
            return Constant.DEFAULT_INVALID_VALUE
        return data[0][0]
