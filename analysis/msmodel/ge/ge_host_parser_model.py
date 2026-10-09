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


class GeHostParserModel(ParserModel):
    """
    Model for Ge Host Parser
    """

    @staticmethod
    def class_name() -> str:
        """
        class name for ge op execute
        """
        return GeHostParserModel.__name__

    def flush(self: any, data_list: list) -> None:
        """
        data flush to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_GE_HOST, data_list)
