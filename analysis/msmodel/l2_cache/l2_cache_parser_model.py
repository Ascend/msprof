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


class L2CacheParserModel(ParserModel):
    """
    db operator for l2 cache parser
    """

    def __init__(self: any, result_dir: str, tale_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_L2CACHE, tale_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        self.insert_data_to_db(self.table_list[0], data_list)

    def get_class_name(self: any) -> str:
        """
        get class name
        """
        return self.__class__.__name__
