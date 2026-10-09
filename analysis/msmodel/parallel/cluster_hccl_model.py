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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.hccl_operator_dto import HCCLOperatorDto


class ClusterHCCLModel(ParserModel):
    """
    class used to operate cluster_hccl db
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_HCCL, [DBNameConstant.TABLE_HCCL_OPERATOR_EXE])

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)


class ClusterHCCLViewModel(ViewModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_HCCL, [])

    def get_hccl_op_data(self: any) -> list:
        sql = "select * from HcclOperatorExe where start_time<>{0} and end_time<>{0} order by start_time".format(
            NumberConstant.INVALID_OP_EXE_TIME
        )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HCCLOperatorDto)
