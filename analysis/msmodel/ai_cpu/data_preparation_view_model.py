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
from profiling_bean.db_dto.data_queue_dto import DataQueueDto
from profiling_bean.db_dto.host_queue_dto import HostQueueDto


class DataPreparationViewModel(ViewModel):
    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS, [])

    def get_host_queue(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_QUEUE):
            return []
        sql = (
            "select index_id, sum(case when type=1 then extra_info else 0 end) as queue_capacity,"
            "sum(case when type=1 then value else 0 end) as queue_size,"
            "sum(case when type=1 then mode else 0 end) as mode,"
            "sum(case when type=0 and extra_info=0 then value else 0 end) as get_time,"
            "sum(case when type=0 and extra_info=1 then value else 0 end) as send_time,"
            "sum(case when type=0 and extra_info=2 then value else 0 end) as total_time "
            "from {0} group by index_id order by index_id".format(DBNameConstant.TABLE_HOST_QUEUE)
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HostQueueDto)

    def get_host_queue_mode(self: any) -> int:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HOST_QUEUE):
            return Constant.DEFAULT_INVALID_VALUE
        sql = "select * from {0} limit 1".format(DBNameConstant.TABLE_HOST_QUEUE)
        data = DBManager.fetch_one_data(self.cur, sql, dto_class=HostQueueDto)
        if not data:
            return Constant.DEFAULT_INVALID_VALUE
        return data.mode

    def get_data_queue(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_DATA_QUEUE, dto_class=DataQueueDto)
