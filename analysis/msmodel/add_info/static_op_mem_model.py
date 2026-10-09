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

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class StaticOpMemModel(ParserModel):
    """
    Model for StaticOpMemParser
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_STATIC_OP_MEM, [DBNameConstant.TABLE_STATIC_OP_MEM])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_STATIC_OP_MEM) -> None:
        """
        insert data to table
        :param data_list: static_op_mem data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)
