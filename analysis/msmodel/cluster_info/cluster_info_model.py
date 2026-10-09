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

from itertools import chain

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto


class ClusterInfoModel(ParserModel):
    """
    class used to operate cluster db
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_RANK, [DBNameConstant.TABLE_CLUSTER_RANK])

    def flush(self: any, data_list: list) -> None:
        """
        flush data to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_CLUSTER_RANK, data_list)


class ClusterInfoViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_CLUSTER_RANK, [DBNameConstant.TABLE_CLUSTER_RANK])

    def get_all_rank_id(self: any) -> set:
        sql = f"select distinct rank_id from {DBNameConstant.TABLE_CLUSTER_RANK}"
        return set(chain.from_iterable(DBManager.fetch_all_data(self.cur, sql)))

    def get_all_cluster_rank_info(self: any) -> list:
        sql = f"select * from {DBNameConstant.TABLE_CLUSTER_RANK}"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=ClusterRankDto)

    def get_device_and_rank_ids(self: any) -> list:
        sql = "select device_id, rank_id from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_rank_or_device_ids(self: any) -> set:
        sql = f"select case when rank_id='N/A' then device_id else rank_id end from {DBNameConstant.TABLE_CLUSTER_RANK}"
        return set(chain.from_iterable(DBManager.fetch_all_data(self.cur, sql)))

    def get_dir_name(self: any, rank_or_device_id: int) -> set:
        sql = (
            f"select dir_name from {DBNameConstant.TABLE_CLUSTER_RANK} "
            f"where (rank_id='N/A' and device_id={rank_or_device_id}) or rank_id={rank_or_device_id} "
        )
        return DBManager.fetch_all_data(self.cur, sql)

    def get_info_based_on_rank_or_device_id(self: any, rank_id: int) -> any:
        sql = f"select * from {DBNameConstant.TABLE_CLUSTER_RANK} where rank_id= ? or device_id= ?"
        return DBManager.fetch_one_data(self.cur, sql, (rank_id, rank_id), ClusterRankDto)

    def get_all_rank_id_and_dirnames(self: any) -> list:
        sql = (
            f"select case when rank_id='N/A' then device_id else rank_id end, dir_name "
            f"from {DBNameConstant.TABLE_CLUSTER_RANK}"
        )
        return DBManager.fetch_all_data(self.cur, sql)
